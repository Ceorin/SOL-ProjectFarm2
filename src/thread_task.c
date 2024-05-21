#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "thread_task.h"
#include "utils.h"
#include "sumfun.h"
#include "collector.h"

// outside it'll be defined as the amount of character; stringsize is to quickly account for the termination char.
#define _STRINGSIZE 1+_DEF_PATHNAME_MAX_SIZE

// encapsulating the stack data in case I need to change it for the future
struct stackData {
    char filename[_STRINGSIZE];
} typedef stackData;

stackData *tasks_stack = NULL;
unsigned long next; // next item to take, will be initialized to -1.
unsigned long maxsize; // size of the stack after initialization

// TODO - requests to remove threads that are waiting.
unsigned long requested_terminations; 
__sig_atomic_t canAdd; // will only be modified by close_fileStack, and only to 0.
__sig_atomic_t freed;

unsigned long waiting_deletion;

static pthread_mutex_t mutex_stack = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t can_push = PTHREAD_COND_INITIALIZER;
static pthread_cond_t can_pop = PTHREAD_COND_INITIALIZER;
static pthread_cond_t can_remove = PTHREAD_COND_INITIALIZER;

/* Initializes the stack for the threads with a size qlen.
    return -1 if the stacks already exists or fails if malloc fails. */
int init_fileStack (size_t qlen)  {
    pthread_mutex_lock(&mutex_stack);
    if (tasks_stack != NULL) {
        // errore, esiste già.
        pthread_mutex_unlock(&mutex_stack);
        return -1;
    }
    DEBUG_PRINT(fprintf(stdout, "Creating stack with size of %ld\n", qlen);)
    maxsize = qlen;
    test_error(tasks_stack = (stackData*) malloc (sizeof(stackData)*qlen), NULL, "Initializing task stack");

    next = 0;
    requested_terminations = 0;
    waiting_deletion = 0;
    canAdd = 1;
    freed = 0;
    pthread_mutex_unlock(&mutex_stack);
    return 0; // success
}

// returns -2 if the stack is not initialized
// returns -1 if it's closed
// returns 0 if it's open and ready
int is_open () {
    pthread_mutex_lock(&mutex_stack); // tasks_stack is a shared resource
    if (tasks_stack == NULL) {
        pthread_mutex_unlock(&mutex_stack);
        return -2;
    }
    pthread_mutex_unlock(&mutex_stack);

    if (!canAdd)
        return -1;
    else
        return 0;
}

// Closes the task queue
void close_fileStack () {
    pthread_mutex_lock(&mutex_stack);
    canAdd = 0;
    pthread_cond_signal(&can_push); // tell everyone who's waiting that they cannot add any more.
    pthread_mutex_unlock(&mutex_stack);
}

void destroy_mutex(size_t);
// Waits for there to be no tasks, then requests to terminate a number of threads equal to the argument and frees the stack.
// returns -1 and fails if the queue is still open.
int delete_fileStack (size_t thread_num) {
    DEBUG_PRINT(fprintf(stdout, "deleting %ld threads\n", thread_num);)
    if (canAdd)
        return -1;
    pthread_mutex_lock(&mutex_stack);
    while (next>0) {
        DEBUG_PRINT(fprintf(stdout, "waiting for stack to empty\n");)
        struct timespec badExit;
        test_error(-1, clock_gettime(CLOCK_REALTIME, &badExit), "Creating safety clock");
        badExit.tv_sec +=5;
        if (ETIMEDOUT == pthread_cond_timedwait(&can_push, &mutex_stack, &badExit)) {
            // TODO - TIMEDWAIT! What if noone is working but the stack is still there?
            DEBUG_PRINT(perror("destroy_filestack:"));
            free(tasks_stack);   
            pthread_cond_destroy(&can_remove);
            pthread_cond_destroy(&can_pop);
            pthread_cond_destroy(&can_push);
            pthread_mutex_destroy(&mutex_stack);
            return -5;
        }
        DEBUG_PRINT(perror("timedwait?"));
    }
    requested_terminations+=thread_num;
    free(tasks_stack);
    freed = 1;
    pthread_cond_broadcast (&can_pop);
    pthread_mutex_unlock(&mutex_stack);
    // if freed is 1, then the last thread will delete the mutex variables
    
    pthread_mutex_lock(&mutex_stack);
    while (requested_terminations>0) {
        struct timespec badExit;
        test_error(-1, clock_gettime(CLOCK_REALTIME, &badExit), "Creating safety clock");
        badExit.tv_sec +=10;
        if (ETIMEDOUT == pthread_cond_timedwait(&can_remove, &mutex_stack, &badExit)) {
            // TODO - TIMEDWAIT! What if noone is working but the stack is still there?
            DEBUG_PRINT(perror("destroy_filestack:"));
            break;
        }
        DEBUG_PRINT(perror("timedwait?"));
    }
    pthread_mutex_unlock(&mutex_stack);

    pthread_cond_destroy(&can_remove);
    pthread_cond_destroy(&can_pop);
    pthread_cond_destroy(&can_push);
    pthread_mutex_destroy(&mutex_stack);
    return 0;
}

/* requests to work on the file with path filename. 
    Return -1 if filename is null, -2 if filename is too long.*/
int add_request (char* name) {
    if (name == NULL)
        return -1;
    if (strlen(name) > _DEF_PATHNAME_MAX_SIZE) // filename too long
        return -2;
    if (!canAdd)
        return -3;
    
    pthread_mutex_lock(&mutex_stack);
    // DEBUG
    //fprintf(stderr, "adding %s, next: %ld, size: %ld\n", name, next, maxsize);
    while (next >= maxsize && canAdd) { // TODO must be interruptable by closing signal: it's the same thread that does this!
        DEBUG_PRINT(fprintf(stdout, "waiting for stack not to be full\n");)
        pthread_cond_wait(&can_push, &mutex_stack);
    }
    if (!canAdd) {
        pthread_mutex_unlock(&mutex_stack);
        return -3;
    } else {
        strncpy(tasks_stack[next].filename, name, _STRINGSIZE);
        next++;
        pthread_cond_signal(&can_pop);
        pthread_mutex_unlock(&mutex_stack);
        // DEBUG
        // fprintf(stderr, "added %s\n", name); 
        return 0;
    }
}

// Gets an item to process from the stack
// returns 0 on success
// returns 1 and ref_result is null, if it received a request to terminate
// if it returns -1, there's an error soemwhere.
// returns -2 an EINVAL happens while waiting (should not have been called)
int get_request (char *ref_result) {
    if (ref_result==NULL)
        return -1;

    pthread_mutex_lock(&mutex_stack);
    while (next==0 && requested_terminations==0) {
        DEBUG_PRINT(fprintf(stdout, "waiting for stack to fill\n");)
        pthread_cond_wait(&can_pop, &mutex_stack);
    } 
    if (requested_terminations>0) { // if it exits from here will free from the exit function of the thread
        if (next > 0 && freed) { // we need to finish consuming
            DEBUG_PRINT(fprintf(stdout, "We'll exit when we're done consuming");)
        } else {
            ref_result = NULL;
            return 1;
        }
        // releases mutex on cleanup
    }
    
    next--;
    strncpy(ref_result, tasks_stack[next].filename, _STRINGSIZE);
    pthread_cond_signal(&can_push);
    pthread_mutex_unlock(&mutex_stack);
    
    return 0;
}

void delete_request () {
    pthread_mutex_lock(&mutex_stack);
    requested_terminations++;
    // tell thread that are waiting to do something that they can try to delete themselves
    pthread_cond_signal(&can_pop);
    pthread_mutex_unlock(&mutex_stack);
}

/* takes mutex and exits (release mutex on exit); 
increases the count of terminated thread if the threadpool has been destroyed */
static void count_exit (void* socket_fd) {
    if (freed) {
        if (requested_terminations) {
            // TODO send "finished" message through the connection.
            pthread_cond_signal(&can_remove);
        }
    }
    //   fprintf(stdout, "%ld exit\n", requested_terminations);
    requested_terminations--;
    pthread_mutex_unlock(&mutex_stack); // should always have mutex while exiting with this
    if (socket_fd!=NULL && *((int*) socket_fd)!=-1)
        close(*((int*) socket_fd));
}

void wait_delete() { // TODO BETTER
    pthread_mutex_lock(&mutex_stack);
    while (requested_terminations==0) {
        DEBUG_PRINT(fprintf(stdout, "waiting to die\n");)
        pthread_cond_wait(&can_pop, &mutex_stack);
    }
    requested_terminations++; // caught the request for someone else but terminates on its own
    pthread_cond_signal(&can_push);
    // exits with lock and everything, cleanup function will release
}

void* worker_thread(void* arg) { 
    // does it need arguments?
    int socket_fd=-1; //def value
    short check_close;
    pthread_cleanup_push(&count_exit, (void*)&socket_fd);
    char filename[_STRINGSIZE];
    check_close = 0;
    result_value myTemp;
    FILE *inp;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, _DEF_SOCKET_NAME, UNIX_SOCKPATH_MAX);
    sa.sun_family = AF_UNIX;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    while (connect (socket_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        if (errno == ENOENT) {
            DEBUG_PRINT(fprintf(stderr, "Socket not found");)
            nanosleep(&(struct timespec){{0}, {10E6}}, NULL); // sleeps 1 millisecond
        } else {
            DEBUG_PRINT(perror("Connecting to server");)
            check_close = 2; // no need to unlock at exit
            break;
        }
        if (freed) { // if I fail to catch it once, it only goes from 0 to 1 once, so I'll take it the next cycle anyway (one more try to connect doesn't hurt anyway)
            DEBUG_PRINT(fprintf(stdout, "Thread didn't do anything\n");)
            check_close = 2;
            break;
        }
    }
/*
    if (check_close == 1) {
        wait_delete()
    }*/

    while (!check_close) {
        check_close = get_request(filename);
        if (check_close == 0) { // return 0, good result
            // filename taken?
            DEBUG_PRINT(fprintf(stdout, "Hi, thread got string %s!\n", filename);)
            inp = fopen(filename, "r");
            if (inp == NULL) // should not get here! MasterWorker betrayed us!
                perror("opening file in thread");
            else { // can do the task
                myTemp = sum_fun_file(filename, inp);
                DEBUG_PRINT(fprintf(stdout,"THREAD: %s : %lld\n", myTemp.name, myTemp.sumvalue);)

                char buf[300];
                strncpy(buf, myTemp.name, sizeof(myTemp.name));
                memcpy(buf+sizeof(myTemp.name), &(myTemp.sumvalue), sizeof(myTemp.sumvalue));

                int last_write, bytes_written = 0, bytes_to_write = sizeof(myTemp);
                while (bytes_to_write > 0) {
                    last_write = write (socket_fd, buf+bytes_written, bytes_to_write);

                    if (last_write <0) {
                        if (errno == EPIPE) {
                            DEBUG_PRINT(printf("Collector finished too quick?"));
                        } else {
                            perror ("THREAD Writing");
                        }
                        break;
                    }
                    bytes_written+= last_write;
                    bytes_to_write -= last_write;
                }
                if (bytes_to_write != 0) {
                    DEBUG_PRINT( perror ("THREAD write not finished properly");)
                }
                // consumed inp properly
                fclose (inp);
            } 

        } else if (check_close == 1)  { // return 1, "you need to stop"
            DEBUG_PRINT(fprintf(stdout,"Thread closing, terminations?:s%ld\n", requested_terminations);)

            if (freed)
                strncpy(myTemp.name, _LAST_TERMINATION_NAME, sizeof(myTemp.name));
            else
                strncpy(myTemp.name, _TERMINATION_RESULT_NAME, sizeof(myTemp.name));            
            myTemp.sumvalue = 0;

            char buf[300];
            strncpy(buf, myTemp.name, sizeof(myTemp.name));
            memcpy(buf+sizeof(myTemp.name), &(myTemp.sumvalue), sizeof(myTemp.sumvalue));

            int last_write, bytes_written = 0, bytes_to_write = sizeof(myTemp);
            while (bytes_to_write > 0) {
                last_write = write (socket_fd, buf+bytes_written, bytes_to_write);

                if (last_write <0) {
                    if (errno == EPIPE) { // multiple threads can send the ending signal, collector might have closed the socket already!
                        DEBUG_PRINT(printf("Collector has closed");)
                    } else {
                        DEBUG_PRINT(perror("THREAD writing");)
                    }
                    break;
                }
                bytes_written+= last_write;
                bytes_to_write -= last_write;
            }
            if (bytes_to_write != 0 && errno != EPIPE) {
                perror ("THREAD write not finished properly");
            }

            close(socket_fd);
            socket_fd = -1;
        } else {
            // errore?
            fprintf(stderr, "huh?\n");
        }
    }
    
    DEBUG_PRINT(fprintf (stdout, "Thread exited with status %d\n", check_close);)
    pthread_cleanup_pop((check_close == 2) ? 0 : 1);
    return 0;
}
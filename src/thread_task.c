
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "thread_task.h"
#include "utils.h"
#include <sched.h>

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
short freed;


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
    fprintf(stderr, "Creating stack with size of %ld\n", qlen);
    maxsize = qlen;
    test_error(tasks_stack = (stackData*) malloc (sizeof(stackData)*qlen), NULL, "Initializing task stack");

    next = 0;
    requested_terminations = 0;
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
    fprintf(stdout, "deleting %ld threads\n", thread_num);
    if (canAdd)
        return -1;
    pthread_mutex_lock(&mutex_stack);
    while (next>0) {
        fprintf(stdout, "waiting for stack to empty\n");
        pthread_cond_wait(&can_push, &mutex_stack);
    }
    requested_terminations+=thread_num;
    free(tasks_stack);
    freed = 1;
    pthread_cond_broadcast (&can_pop);
    pthread_mutex_unlock(&mutex_stack);
    // if freed is 1, then the last thread will delete the mutex variables
    
    pthread_mutex_lock(&mutex_stack);
    while (requested_terminations>0)
        pthread_cond_wait(&can_remove, &mutex_stack);
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
    while (next >= maxsize && canAdd) {
        fprintf(stdout, "waiting for stack not to be full\n");
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

/* Private function for the threads to pop from the stack
    ref_result NEEDS to be a string of defined size, but given it's used only here we know as much.
    probably will tell threads to exit here. (see prototype version)*/
 /* int get_request (char *ref_result) {
    if (ref_result==NULL)
        return -1;
    pthread_mutex_lock(&mutex_stack);

    // DEBUG
    // fprintf(stdout, "popping from next:%ld\n", next);

    while (next==0) {
        fprintf(stdout, "waiting for stack to fill\n");
        pthread_cond_wait(&can_pop, &mutex_stack);
    }
    next--;
    strncpy(ref_result, tasks_stack[next].filename, _STRINGSIZE);
    pthread_cond_signal(&can_push);
    pthread_mutex_unlock(&mutex_stack);
    // DEBUG
    // fprintf(stdout, "popped %s\n", ref_result);
    return 0;
} */

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
        fprintf(stdout, "waiting for stack to fill\n");
        pthread_cond_wait(&can_pop, &mutex_stack);
    } 
    if (requested_terminations>0) { // if it exits from here will free from the exit function of the thread
        ref_result = NULL;
        return 1;
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



/* non-delete worker thread
void* worker_thread (void* arg) {
    // does it need arguments?
    if (arg!=NULL)
        free(arg);
    
    char filename[_STRINGSIZE] = "test";
    int retvalue;
    sleep(1);
    while (1) {
        retvalue = get_request(filename);
        if (retvalue == 0) { // return 0, good result
            // TASK
            fprintf(stdout, "Hi, I got string %s!\n", filename);
            // YEE

        } else {
            // errore
            fprintf(stdout, "huh?\n");
        }
    }

} */


/* takes mutex and exits (release mutex on exit); 
increases the count of terminated thread if the threadpool has been destroyed */
static void count_exit (void* arg) {
    if (arg!= NULL)
        free(arg);
    if (freed) {
        if (requested_terminations)
            pthread_cond_signal(&can_remove);
    }
 //   fprintf(stdout, "%ld exit\n", requested_terminations);
    requested_terminations--;
    pthread_mutex_unlock(&mutex_stack); // should always have mutex while exiting with this
}

void* worker_thread(void* arg) { 
    // does it need arguments?
    pthread_cleanup_push(&count_exit, arg);

    char filename[_STRINGSIZE];
    short check_close = 0;
    while (!check_close) {
        check_close = get_request(filename);
        if (check_close == 0) { // return 0, good result
            // TASK
            fprintf(stdout, "Hi, I got string %s!\n", filename);
            // YEE

        } else if (check_close == 1)  { // return 1, "you need to stop"
            fprintf(stdout, "Okay I'll close - %ld\n", requested_terminations);
            fflush(stdout);
        } else if (check_close == 2) {
            // errore
            fprintf(stdout, "huh?\n");
        }
    }

//    fprintf (stdout, "Thread exited with status %d\n", check_close);
    pthread_cleanup_pop(1);
    return 0;
}
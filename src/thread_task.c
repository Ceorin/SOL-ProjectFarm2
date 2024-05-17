#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "thread_task.h"
#include "utils.h"

// outside it'll be defined as the amount of character; stringsize is to quickly account for the termination char.
#define _STRINGSIZE 1+_DEF_PATHNAME_MAX_SIZE

// encapsulating the stack data in case I need to change it for the future
struct  stackData {
    char filename[_STRINGSIZE];
} typedef stackData;

stackData *tasks_stack = NULL;
unsigned long next; // next item to take, will be initialized to -1.
unsigned long maxsize; // size of the stack after initialization

// TODO - requests to remove threads that are waiting.
unsigned long requested_terminations; 

static pthread_mutex_t mutex_stack = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty_stack = PTHREAD_COND_INITIALIZER;
static pthread_cond_t full_stack = PTHREAD_COND_INITIALIZER;

/* Initializes the stack for the threads with a size qlen.
    return -1 if the stacks already exists or fails if malloc fails. */
int init_fileStack (size_t qlen)  {
    if (tasks_stack != NULL) {
        // errore, esiste già.
        return -1;
    }
    maxsize = qlen;
    test_error(tasks_stack = (stackData*) malloc (sizeof(stackData)*qlen), NULL, "Initializing task stack");

    next = -1;
    requested_terminations = 0;
    return 0; // success
}

/* requests to work on the file with path filename. 
    Return -1 if filename is null, -2 if filename is too long.*/
int add_request (char* name) {
    if (name == NULL)
        return -1;
    if (strlen(name) > _DEF_PATHNAME_MAX_SIZE) // filename too long
        return -2;
    
    pthread_mutex_lock(&mutex_stack);

    while (next >= maxsize-1) {
        fprintf(stdout, "waiting for stack not to be full\n");
        pthread_cond_wait(&full_stack, &mutex_stack);
    }
    next++;
    strncpy(tasks_stack[next].filename, name, _STRINGSIZE);
    pthread_cond_signal(&empty_stack);
    pthread_mutex_unlock(&mutex_stack);
    
    return 0;
}

/* Private function for the threads to pop from the stack
    ref_result NEEDS to be a string of defined size, but given it's used only here we know as much.
    probably will tell threads to exit here. (see prototype version)*/
int get_request (char *ref_result) {
    if (ref_result==NULL)
        return -1;
    pthread_mutex_lock(&mutex_stack);
    while (next<0) {
        fprintf(stdout, "waiting for stack to fill\n");
        pthread_cond_wait(&empty_stack, &mutex_stack);
    } 
    strncpy(ref_result, tasks_stack[next].filename, _STRINGSIZE);
    next--;
    pthread_cond_signal(&full_stack);
    pthread_mutex_unlock(&mutex_stack);
    return 0;
}

// probably will tell threads to exit here.
// if it returns 1 and ref_result is null, it'll tell the thread to exit.
int prototype_get_request_with_delete (char *ref_result) {
    if (ref_result==NULL)
        return -1;

    int ret_value;
    pthread_mutex_lock(&mutex_stack);
    while (next<0 && requested_terminations==0) {
        fprintf(stdout, "waiting for stack to fill\n");
        pthread_cond_wait(&empty_stack, &mutex_stack);
    } 
    if (requested_terminations!=0) {
        requested_terminations--;
        ref_result = NULL;
        ret_value = 1;
    } else {
        strncpy(ref_result, tasks_stack[next].filename, _STRINGSIZE);
        next--;
        ret_value = 0;
        pthread_cond_signal(&full_stack);
    }
    pthread_mutex_unlock(&mutex_stack);
    return ret_value;
}

void prototype_delete_request () {
    pthread_mutex_lock(&mutex_stack);
    requested_terminations++;
    // tell thread that are waiting to do something that they can try to delete themselves
    pthread_cond_signal(&empty_stack);
    pthread_mutex_unlock(&mutex_stack);
}




void worker_thread (void* arg) {
    // does it need arguments?
    if (arg!=NULL)
        free(arg);
    
    char filename[_STRINGSIZE];
    int retvalue;
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

}

void prototype_worker_thread(void* arg) { 
    // does it need arguments?
    if (arg!=NULL)
        free(arg);
    
    char filename[_STRINGSIZE];
    short check_close = 0;
    while (!check_close) {
        check_close = prototype_get_request_with_delete(filename);
        if (check_close == 0) { // return 0, good result
            // TASK
            fprintf(stdout, "Hi, I got string %s!\n", filename);
            // YEE

        } else if (check_close == 1)  { // return 1, "you need to stop"
            fprintf(stdout, "Okay I'll close\t");
        } else {
            // errore
            fprintf(stdout, "huh?\n");
        }
    }

    fprintf (stdout, "Thread exited with status %d\n", check_close);

}
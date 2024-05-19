#include "worker_pool.h"
#include "thread_task.h"
#include "utils.h"
#include "signal_utils.h"
// #include "myList.h" not needed if detached threads work
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>


/* INITIAL TEST:
    THREADS WILL BE DETACHED
    current_thread_num will HAVE TO be consistent with the requests to destroy or create threads! */

size_t current_thread_num;

/* Initializes thread pool with a concurrent queeu of size queue_len and a number of initial threads equal to thread_num
    threads will all be the same, deatched, working on the task defined in thread_task.
    returns -1 if the initialization fails, otherwise returns the number of thread created.
    Do note: if the returns is lower than the second parameter, something went wrong.*/
int init_worker_pool(size_t queue_len, size_t thread_num) {
    if (thread_num <1)
        return -1;
    if (queue_len <1)
        return -1;
    // Threads will be detached, as such I can reuse the variable to create
    int init_ret = init_fileStack(queue_len);
    if (init_ret == -1)
        return -1;
    current_thread_num = 0;
    pthread_t temp_Thread;
    
    // funneling all signals to master
    sigset_t master_mask = mask_all();
    for (long int i = 0; i < thread_num; i++) {
        test_error_isNot(0, errno = pthread_create(&(temp_Thread), NULL, &worker_thread, NULL), "Creating worker thread");
        test_error_isNot(0, errno = pthread_detach(temp_Thread), "Detaching thread");
        current_thread_num++;
    }
    return_mask(master_mask);
    // if every thread has been created correctly, ends state with current_thread_num = thread_num.
    return current_thread_num;
}

// Send the argument for a task to the thread pool
// returns -1 if the argument is invalid, -2 if it's too long
int send_request_to_pool(char* filename) {
    return add_request(filename);
}

// tries to add a new executing thread 
// returns a negative value if the thread_pool is closed (-1) or not initialized (-2).
int add_thread() {
    int can_create = is_open();
    if (can_create<0)
        return can_create;  
    pthread_t temp_Thread;
    // funneling all signals to master
    sigset_t master_mask = mask_all();
    test_error_isNot(0, errno = pthread_create(&(temp_Thread), NULL, &worker_thread, NULL), "Creating worker thread");
    test_error_isNot(0, errno = pthread_detach(temp_Thread), "Detaching thread");
    return_mask(master_mask);
    current_thread_num++;
    return 0;
}

// tries to decrease the amount of threads
// will fail if the threadpool is not initialized (-2) or if there is only one thread left (-1)
int delete_thread() {
    if (current_thread_num <2)
        return -1;
    if (is_open() == -2)
        return -1;
    delete_request();
    current_thread_num--;
    return 0;
}

// Destroys the pool.
// returns a negative value if something wrong happened, in which case the state of the pool is undefined.
int destroy_pool() {
    close_fileStack();
    int delete_value = delete_fileStack(current_thread_num);
    DEBUG_PRINT(fprintf(stdout, "Threadpool delete value=%d\n", delete_value);)
    current_thread_num=0;;
    return delete_value;
}

// returns the number of thread currently active in the pool
int pool_size() {
    return current_thread_num;
}
#ifndef FUNCTION_TASK_HEADER
    #define FUNCTION_TASK_HEADER

    #include <stdlib.h>

    int init_fileStack (size_t qlen);

    int add_request(char*);
    void prototype_delete_request();
    void* worker_thread (void*);
    void* prototype_worker_thread(void*);
#endif
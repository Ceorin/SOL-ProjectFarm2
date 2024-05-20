#ifndef FUNCTION_TASK_HEADER
    #define FUNCTION_TASK_HEADER

    #include <stdlib.h>

    #ifndef _DEF_SOCKE_NAME
        #define _DEF_SOCKET_NAME "./tmp/farm2.sck"
    #endif

    int init_fileStack (size_t qlen);
    int is_open();
    void close_fileStack();
    int delete_fileStack (size_t);

    int add_request(char*);
    void delete_request();
    void* worker_thread (void*);
    // void* prototype_worker_thread(void*);
#endif
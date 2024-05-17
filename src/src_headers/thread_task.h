#ifndef FUNCTION_TASK_HEADER
    #define FUNCTION_TASK_HEADER

    #define _DEF_PATHNAME_MAX_SIZE 255
    #include <stdlib.h>

    int init_fileStack (size_t qlen);

    int add_request(char*);
    void prototype_delete_request();
    void* worker_thread (void*);
#endif
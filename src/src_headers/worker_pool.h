#ifndef WORKERS_HEADER
    #define WORKERS_HEADER

    #include <stdlib.h>
    int init_worker_pool(size_t, size_t);
    int send_request_to_pool(char*);
    int add_thread();
    int delete_thread();
    int destroy_pool();


#endif
#ifndef COLLECTOR_PRINT
    #define COLLECTOR_PRINT

    #include "myList.h"
    #include <pthread.h>

    extern pthread_mutex_t mutex_last;
    void* printingthread (void*);
#endif
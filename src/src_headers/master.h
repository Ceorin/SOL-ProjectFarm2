#ifndef MASTER_HEADER
    #define MASTER_HEADER

    #define _DEF_NTHREAD 4
    #define _DEF_QLEN 8
    #define _DEF_DELAY 0
    #define MAX_DELAY 3000 // ms

    void masterThread(int, char**);

#endif
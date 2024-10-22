#ifndef MY_UTILITIES
    #define MY_UTILITIES
    #include <stdio.h>
    #include <stdlib.h>
    #define _DEF_PATHNAME_MAX_SIZE 255
    // Exit macros
    #define test_error(comp, sc, msg) \
        if ((sc) == (comp) ) { perror (msg); exit(EXIT_FAILURE); }
    #define test_error_isNot(comp, sc, msg) \
        if ((sc) != (comp) ) { perror (msg); exit(EXIT_FAILURE); }

    #ifdef DEBUG
        #define DEBUG_PRINT(msg) msg
    #else
        #define DEBUG_PRINT(msg) 
    #endif 
    
    #define UNIX_SOCKPATH_MAX 108

#endif
#ifndef MY_UTILITIES
    #define MY_UTILITIES
    #include <stdio.h>
    #define _DEF_PATHNAME_MAX_SIZE 255
    // Exit macros
    #define test_error(comp, sc, msg) \
        if ((sc) == (comp) ) { perror (msg); exit(EXIT_FAILURE); }
    #define test_error_isNot(comp, sc, msg) \
        if ((sc) != (comp) ) { perror (msg); exit(EXIT_FAILURE); }

#endif
#ifndef SUMFUN
    #define SUMFUN
    #ifndef _DEF_PATHNAME_MAX_SIZE
        #define _DEF_PATHNAME_MAX_SIZE 255
    #endif

    struct result_value {
        char name [1+_DEF_PATHNAME_MAX_SIZE];
        long long int sumvalue;
    } typedef result_value;

    #include <stdio.h>

    result_value sum_fun_fd (char*, int);
    result_value sum_fun_file (char*, FILE*);
#endif
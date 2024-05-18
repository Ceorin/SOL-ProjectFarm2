#ifndef SUMFUN
    #define SUMFUN
    #ifndef _DEF_PATHNAME_MAX_SIZE
        #define _DEF_PATHNAME_MAX_SIZE 255
    #endif
    
    struct result_value {
        char name [1+_DEF_PATHNAME_MAX_SIZE];
        long long int sumvalue;
    } typedef result_value;

    result_value sum_fun (char*, int);
#endif
#ifndef COLLECTOR_HEADER
    #define COLLECTOR_HEADER
    
    #define _DEF_COLLECTOR_PATH "./collector"
    #define _DEF_SOCKET_NAME "tmp/farm2.sck"
    
    /* wille poll an array, the number of requests should be mostly constant after the beginning, 
        so reallocating should not happen too often */
    #define _START_SIZE_POLL 32 
    
    //This name should never be processed as a file thus should never be sent to collector
    #define _TERMINATION_RESULT_NAME "./"  // Could probably simply be empty string

#endif
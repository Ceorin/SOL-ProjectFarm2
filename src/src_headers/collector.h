#ifndef COLLECTOR_HEADER
    #define COLLECTOR_HEADER
    
    #define _DEF_COLLECTOR_PATH "./collector"
    #define _DEF_SOCKET_NAME "./tmp/farm2.sck"
    
    /* wille poll an array, the number of requests should be mostly constant after the beginning, 
        so reallocating should not happen too often */
    #define _START_SIZE_POLL 32 // Currently linear increments
    #define _COLLECTOR_TIMEOUT 5000 // milliseconds

    /* These names should **never** be processed as a file by masterworker, 
        thus should never be sent to collector.
        Therefore we can use them as constant variables to check for stream ends. */ 
    #define _TERMINATION_RESULT_NAME "./"  // Could probably simply be empty string
    #define _LAST_TERMINATION_NAME "../" // as above, as long as they are different from one another

#endif
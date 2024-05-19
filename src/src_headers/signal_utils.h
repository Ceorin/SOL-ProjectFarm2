#ifndef MY_SIGNAL_UTILITIES
    #define MY_SIGNAL_UTILITIES
    #include "signal.h"

    sigset_t mask_all();
    void unmask_all();
    void return_mask(sigset_t);

#endif
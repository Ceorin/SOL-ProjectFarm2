#ifndef HANDLE_SIG_MASTER
    #define HANDLE_SIG_MASTER
    #include <signal.h>
    #include "signal_utils.h"

    extern volatile __sig_atomic_t th_num_modify; // negative if need to remove, positive otherwise
    extern volatile __sig_atomic_t terminate_th_pool; // 0 usually; set to 1 when receiving termination 

    int handle_signals_master();
#endif  
#include "utils.h"
#include "signal_handlers_master.h"
#include <string.h>

volatile __sig_atomic_t th_num_modify = 0; // negative if need to remove, positive otherwise
volatile __sig_atomic_t terminate_th_pool = 0; // 0 usually; set to 1 when receiving termination 


static void handler_USR1 (int signum) {
    DEBUG_PRINT(write(1, "Received SIGUSR1\n", 18);)
    th_num_modify++;
}

static void handler_USR2 (int signum) {
    DEBUG_PRINT(write(1, "Received SIGUSR2\n", 18));
    th_num_modify--;
}

// for SIGHUP, SIGINT, SIGQUIT, SIGTERM
static void handle_interrupt (int signum) {
    DEBUG_PRINT(write(1, "Received interrupt signal\n", 26);)

    terminate_th_pool = 1;
}

// Sets signal handlers for master thread
// return 0 on success, or an error otherwise.
 int handle_signals_master() {
    sigset_t previous_mask;
    struct sigaction handlers; 
    previous_mask = mask_all();
    memset(&handlers, 0, sizeof(handlers));

    handlers.sa_handler = handle_interrupt;
    test_error(sigaction(SIGHUP, &handlers, NULL), -1, "Setting SIGHUP");
    test_error(sigaction(SIGINT, &handlers, NULL), -1, "Setting SIGINT");
    test_error(sigaction(SIGQUIT, &handlers, NULL), -1, "Setting SIGQUIT");
    test_error(sigaction(SIGTERM, &handlers, NULL), -1, "Setting SIGTERM");

    handlers.sa_handler = handler_USR1;
    test_error(sigaction(SIGUSR1, &handlers, NULL), -1, "Setting SIGUSR1");

    handlers.sa_handler = handler_USR2;
    test_error(sigaction(SIGUSR2, &handlers, NULL), -1, "Setting SIGUSR2");

    handlers.sa_handler = SIG_IGN;
    test_error(sigaction(SIGPIPE, &handlers, NULL), -1, "Setting SIGPIPE");
    

    return_mask (previous_mask);
    return 0;
}
#include <stdio.h>
#include <sys/socket.h>
#include "utils.h"
#include "signal_utils.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

 // 0 = mask is not set. by default, it is set by master, thus it considers all signals to be masked before enterign the function
void collector_set_signals(int mask_is_set)  {
    sigset_t mask_q;
    if (!mask_is_set)
        mask_q = mask_all();
        
    struct sigaction sa_ign;
    memset(&sa_ign, 0, sizeof(sa_ign));

    sa_ign.sa_handler = SIG_IGN; 
    test_error(sigaction(SIGPIPE, &sa_ign, NULL), -1, "Setting SIGPIPE to ignore");
    test_error(sigaction(SIGHUP, &sa_ign, NULL), -1, "Setting SIGHUP to ignore");
    test_error(sigaction(SIGINT, &sa_ign, NULL), -1, "Setting SIGINT to ignore");
    test_error(sigaction(SIGQUIT, &sa_ign, NULL), -1, "Setting SIGQUIT to ignore");
    test_error(sigaction(SIGTERM, &sa_ign, NULL), -1, "Setting SIGTERM to ignore");
    test_error(sigaction(SIGUSR1, &sa_ign, NULL), -1, "Setting SIGUSR1 to ignore");
    test_error(sigaction(SIGUSR2, &sa_ign, NULL), -1, "Setting SIGUSR2 to ignore");

    if (!mask_is_set)
        return_mask(mask_q);
    else
        unmask_all();
}

// TODO let collector be run on its own
int main(int argc, char* argv[]) {
    collector_set_signals(1); // default 1 when called from Main, TODO when run on its own.
    
    
    if (argc != 2) {
        fprintf(stderr, "use as %s <socket name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    fprintf (stdout, "Collector creato\n");
    fprintf (stdout, "\narg1: %s\n", argv[1]);
    fflush(stdout);
    sleep(1);
    return 0;
} //?
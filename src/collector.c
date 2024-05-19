#include <stdio.h>
#include <sys/socket.h>
#include "utils.h"
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

// TODO let collector be run on its own
int main(int argc, char* argv[]) {
    // enters with all signals masked
    
    struct sig_action sa_ign;
    sa_ign.sa_handler = SIG_IGN; 
    test_error(sigaction(SIGPIPE, &sa_ign, NULL), -1, "Setting SIGPIPE to ignore");
    test_error(sigaction(SIGHUP, &sa_ign, NULL), -1, "Setting SIGHUP to ignore");
    test_error(sigaction(SIGINT, &sa_ign, NULL), -1, "Setting SIGINT to ignore");
    test_error(sigaction(SIGQUIT, &sa_ign, NULL), -1, "Setting SIGQUIT to ignore");
    test_error(sigaction(SIGTERM, &sa_ign, NULL), -1, "Setting SIGTERM to ignore");
    test_error(sigaction(SIGUSR1, &sa_ign, NULL), -1, "Setting SIGUSR1 to ignore");
    test_error(sigaction(SIGUSR2, &sa_ign, NULL), -1, "Setting SIGUSR2 to ignore");

    // unmasking signals
    sigset_t mask;
    test_error(sigemptyset(&mask), -1, "Creating unmask_all");
    test_error(pthread_sigmask(SIG_SETMASK, &mask, NULL), -1, "Setting unmask_all")
    
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
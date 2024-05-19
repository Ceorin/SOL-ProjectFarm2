#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "collector.h"
#include "master.h"
#include "signal_handlers_master.h"

int main(int argc, char* argv[]) {
    __pid_t collector_id;
    // masking SIGPIPE for master and collector

    struct sigaction sa_ign; 
    sigset_t curmask = mask_all();
    memset(&sa_ign, 0, sizeof(sa_ign));

    sa_ign.sa_handler = SIG_IGN;
    test_error(sigaction(SIGPIPE, &sa_ign, NULL), -1, "Setting pipe to ignore");
    test_error(sigaction(SIGHUP, &sa_ign, NULL), -1, "Setting SIGHUP to ignore");
    test_error(sigaction(SIGINT, &sa_ign, NULL), -1, "Setting SIGINT to ignore");
    test_error(sigaction(SIGQUIT, &sa_ign, NULL), -1, "Setting SIGQUIT to ignore");
    test_error(sigaction(SIGTERM, &sa_ign, NULL), -1, "Setting SIGTERM to ignore");
    test_error(sigaction(SIGUSR1, &sa_ign, NULL), -1, "Setting SIGUSR1 to ignore");
    test_error(sigaction(SIGUSR2, &sa_ign, NULL), -1, "Setting SIGUSR2 to ignore");
    

    
    test_error(collector_id = fork(), -1, "Forking process");
    if (collector_id == 0) { // esegui collector
        execl(_DEF_COLLECTOR_PATH, _DEF_COLLECTOR_PATH, _DEF_SOCKET_NAME, NULL);
        perror("Executing exec for collector");
        exit(EXIT_FAILURE);
    } else {
        // unmasking pipe
        return_mask(curmask);
        DEBUG_PRINT(kill(collector_id, SIGPIPE));
        DEBUG_PRINT(kill(collector_id, SIGINT));
        DEBUG_PRINT("Is collector dead or did it go on?\n");
        masterThread(argc, argv);
        // (2) TODO PIPE -> WORKERS CREATION
       // (3) TODO READING INPUTS
    }
    // Finito di leggere i suoi file e fatte le sue cose, attende la fine dei thread e di collector
    waitpid(collector_id, NULL, 0   );
    // gestione del risultato di collector
    return 0;
}
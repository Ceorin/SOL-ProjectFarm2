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
#include "signal_utils.h"

int main(int argc, char* argv[]) {
    __pid_t collector_id;

    // masking signals for fork
    sigset_t curmask = mask_all();
    
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
        DEBUG_PRINT(printf("Is collector dead or did it go on?\n");)
        masterThread(argc, argv);
        // (2) TODO PIPE -> WORKERS CREATION
       // (3) TODO READING INPUTS
    }
    // Finito di leggere i suoi file e fatte le sue cose, attende la fine dei thread e di collector
    waitpid(collector_id, NULL, 0   );
    // gestione del risultato di collector
    return 0;
}
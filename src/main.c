#include <unistd.h>
#include <stdio.h>
#include "utils.h"
#include "collector.h"
#include "master.h"

#define COLLECTOR_PATH "./collector"
#define SOCKET_NAME "farm2"

int main(int argc, char* argv[]) {
    __pid_t collector_id;
    // (4) TODO SIGNAL HANDLING

    // (1) TODO FORK-EXEC COLLECTOR
    test_error(collector_id = fork(), -1, "Forking process");
    if (collector_id == 0) { // esegui collector
        execl(COLLECTOR_PATH, SOCKET_NAME);
        perror("Executing exec for collector");
        exit(EXIT_FAILURE);
    } else { // esegui MasterWorker
        masterThread();
        // (2) TODO PIPE -> WORKERS CREATION
       // (3) TODO READING INPUTS
    }
    // Finito di leggere i suoi file e fatte le sue cose, attende la fine dei thread e di collector
    waitpid(collector_id);
    // gestione del risultato di collector
    return 0;
}
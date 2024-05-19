#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "utils.h"
#include "signal_utils.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "collector.h"

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

    DEBUG_PRINT(fprintf (stdout, "Collector creato\narg1: %s\n", argv[1]);fflush(stdout);)
    // create list TODO

    // Setup as a server listening
    int collector_fd_server, client_socket;
    struct sockaddr_un collector_socket;

    strncpy(collector_socket.sun_path, argv[1], UNIX_SOCKPATH_MAX);
    collector_socket.sun_family = AF_UNIX;

    unlink(collector_socket.sun_path); // if the socket has not been removed last previously
    errno = 0; // ignoring unlink errors. The notable ones (like access requirements) will be found by socket and bind functions

    // Consider - refactor using SOCK_DGRM? Would remove the need for poll and the limited (albeit big) amount of clients
    test_error(-1, collector_fd_server = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0), "Creating collector socket");
    // socket non-blocking => accepted connnection will be non-blocking
    test_error(-1, bind(collector_fd_server, (struct sockaddr*)&collector_socket, sizeof(collector_socket)), "Binding collector socket");
    test_error(-1, listen(collector_fd_server, SOMAXCONN), "Collector server - listen");

    DEBUG_PRINT(fprintf(stdout, "Collector socket started - listening on %d!\n", collector_fd_server);)

    
    // creating structure for poll
    struct pollfd* pFDs;
    int size = _START_SIZE_POLL; // will be a dinamic array, it should not reallocate often
    int num_FDs = 1; // collector itself
    int tempsize = 0, pollRes = 0;

    test_error(NULL, pFDs = (struct pollfd*) calloc (size, sizeof(struct pollfd)), "allocating poll structure");
    
    memset(pFDs, 0, sizeof(struct pollfd)*size); // calloc should set the pfds to 0, but just in case a differente implementation of calloc is useds

    DEBUG_PRINT(fprintf(stdout, "Created poll fds! Are these 0? %d - %d\n", pFDs[0].fd, pFDs[size-1].fd);)

    sleep(1);
    return 0;
} //?
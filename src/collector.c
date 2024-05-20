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
#include "sumfun.h"
#include "collector_print.h"
#include <pthread.h>


pthread_mutex_t mutex_last = PTHREAD_MUTEX_INITIALIZER;

 // 0 = mask is not set. by default, it is set by master, thus it considers all signals to be masked before enterign the function
void collector_set_signals(int);
void use_client(int);


int main(int argc, char* argv[]) {
    if (argc != 2) { // TODO let collector be run on its own
        fprintf(stderr, "use as %s <socket name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // default 1 when called from Main, TODO when run on its own.
    collector_set_signals(1); 

    DEBUG_PRINT(fprintf (stdout, "Collector creato\narg1: %s\n", argv[1]);fflush(stdout);)
    // create list
    list_t* result_list = empty_List(false);

    // create printing thread

    pthread_t printing_thread;
    test_error_isNot(0, errno = pthread_create(&(printing_thread), NULL, &printingthread, (void*)result_list), "Creating printing thread");
    test_error_isNot(0, errno = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL), "setting thread to be cancelable")
    

    // creating socket
    DEBUG_PRINT(printf("\n***Starting socket test***\n");)

    int listen_sck, client_fd;
    struct sockaddr_un my_address;
    strncpy(my_address.sun_path, _DEF_SOCKET_NAME, UNIX_SOCKPATH_MAX);
    my_address.sun_family = AF_UNIX;

    unlink(my_address.sun_path);
    errno = 0;// errno ignored, bind will trigger the same errors

    test_error(-1, listen_sck = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0), "creating collector socket")
    test_error(-1, bind (listen_sck, (struct sockaddr*) &my_address, sizeof(my_address)),"binding collector socket")
    test_error(-1, listen(listen_sck, SOMAXCONN), "listening started")

    // creating poll
    struct pollfd* pFDs;
    int pollsize = _START_SIZE_POLL; // size of the poll array. Will dynamically expand eventually
    int num_FDs = 1; // numbere of FD to poll at the beginning; index of the first "free" poll-fd
    int poll_res = 0; // number of FD with events
    int tmp_size = 0; // num_FDs to iterate on

    test_error(NULL, pFDs = (struct pollfd*) calloc(pollsize, sizeof(struct pollfd)), "Allocating poll structure" );
    // memset(pFDs, 0, sizeof(pFDs)); not needed, done by calloc

    pFDs[0].fd = listen_sck;
    pFDs[0].events = POLLIN;

    int go = 1, close_requested = 0;
    while (go) {
        DEBUG_PRINT(sleep(1);)   
        DEBUG_PRINT(fprintf(stdout, "Waiting on poll\n");)
        test_error(-1, poll_res = poll(pFDs, num_FDs, 5000), "Poll failed");
        if (poll_res == 0){
            fprintf(stderr, "Timed out!\n");
            break;
        }

        tmp_size = num_FDs;
        for (int i = 0; i<tmp_size; i++) { // checking polliin results
            if (pFDs[i].revents == 0)
                continue; // nothing to do
            
            if (!(pFDs[i].revents & POLLIN)) { // error - idk what to do!
                fprintf(stderr, "Idk what this is: [fd:%d - rq:%d]\n", pFDs[i].fd, pFDs[i].revents);
            }

            if (pFDs[i].fd == listen_sck) { // ready to accept new connections
                DEBUG_PRINT( fprintf(stdout, "***Listen socket reading***\n");)
                do {
                    client_fd = accept(listen_sck, NULL, 0);
                    if (client_fd <0) {
                        if (errno == (EWOULDBLOCK | EAGAIN)) // no read to do
                            errno = 0;
                        else 
                            perror("Accepting clients");
                    } else {
                        DEBUG_PRINT( fprintf(stdout, "Accepted client on fd: %d\n", client_fd);)
                        int can_add = 1;
                        if (num_FDs+1 == pollsize) {
                            DEBUG_PRINT( fprintf(stderr, "Poll is full! Create a bigger one\n");)
                            pFDs = (struct pollfd*) realloc (pFDs, (pollsize+_START_SIZE_POLL) * sizeof(struct pollfd));
                            if (errno == ENOMEM) {
                                perror("Reallocating poll structure");
                                can_add = 0;
                            } else { // set new member of the structure to 0
                                memset(pFDs+_START_SIZE_POLL, 0, _START_SIZE_POLL);
                                pollsize += _START_SIZE_POLL;
                            }
                        } 
                        if (can_add) {
                            pFDs[num_FDs].fd = client_fd;
                            pFDs[num_FDs].events = POLLIN;
                            num_FDs++;
                        }
                    }
                } while (client_fd >= 0);
            } else { // a connection wants to write something
                DEBUG_PRINT( printf("Client%d wants to write\n", pFDs[i].fd);)
                char buf[300];
                int last_read, bytes_read = 0, bytes_to_read = sizeof(result_value);
                while (bytes_to_read > 0) { 
                    last_read = read(pFDs[i].fd, &(buf[bytes_read]), bytes_to_read);
                    if (last_read < 0)
                        perror("reading");
                    bytes_read += last_read;
                    bytes_to_read -= last_read;
                }
                if (bytes_to_read != 0)
                    perror("finishing read");

                // convert buf in result_value
                result_value *wrapper;
                test_error(NULL, wrapper = (result_value*) malloc(sizeof(result_value)), "Creating result_value data");
                strncpy(wrapper->name, buf, sizeof(wrapper->name));
                memcpy(&(wrapper->sumvalue), buf+sizeof(wrapper->name), sizeof(wrapper->sumvalue));
                DEBUG_PRINT (printf("client said: %s : %lld\n", wrapper->name, wrapper->sumvalue);) 
                if (!strncmp(wrapper->name, _LAST_TERMINATION_NAME, sizeof(_LAST_TERMINATION_NAME))) {
                    free(wrapper);
                    pFDs[i].fd = -1;
                    num_FDs--;
                    close_requested = 1;
                } else if (!strncmp(wrapper->name, _TERMINATION_RESULT_NAME, sizeof(_TERMINATION_RESULT_NAME))) {
                    free(wrapper);
                    pFDs[i].fd = -1; // I don't want to write anymore anyway
                    num_FDs--;
                } else {
                    pthread_mutex_lock(&mutex_last);
                    int ret = add_Last(NULL, wrapper, result_list);
                    pthread_mutex_unlock(&mutex_last);
                    if (ret < 0) {
                        free(wrapper);
                        fprintf (stderr, "%d\t", ret);
                        perror("Adding result!");
                        errno = 0;
                    } else {
                        DEBUG_PRINT(fprintf(stdout, "file?: %s\n", wrapper->name);)
                    } 
                } 
            }

        }

        DEBUG_PRINT(
        fprintf(stdout,  "FD inside now:\n");
        for (int i = 0; i<num_FDs; i++) {
            printf("%d\t", pFDs[i].fd);
        })

        // removing closed FD 
        for (int i = 0; i< num_FDs; i++) {
            if (pFDs[i].fd == -1) {
                //fd is closed
                for (int j = i; j < num_FDs; j++) { // move every fd from position [i] to position [i-1]
                    pFDs[j].fd = pFDs[j+1].fd;
                    pFDs[j].events = pFDs[j+1].events;
                }
                i--;
                num_FDs--;
            }
        }

        DEBUG_PRINT(
        fprintf(stdout,  "FD inside after removal:\n");
        for (int i = 0; i<num_FDs; i++) {
            printf("%d\t", pFDs[i].fd);
        }
        fflush(stdout);)
        
        
        if (close_requested) {
            if (num_FDs == 1)
                go = 0;
            else {
                DEBUG_PRINT(printf("Close requested but can't yet\n");)
            }
        }

        if ((num_FDs > 1) && (pollsize > _START_SIZE_POLL) && (pollsize/4 > num_FDs)) {
            pollsize/=2;
            pFDs = (struct pollfd*) realloc (pFDs, pollsize * sizeof(struct pollfd));
            if (errno == ENOMEM) {
                perror("Reducing poll structure size");
            }
        }
    }

    free(pFDs);

    result_value* fake_end;
    test_error(NULL, fake_end = (result_value*) malloc(sizeof(result_value)), "Puttign an end to result_list data");
    strncpy(fake_end->name, _LAST_TERMINATION_NAME, sizeof(fake_end->name));
    fake_end->sumvalue = 0;
    pthread_mutex_lock(&mutex_last);
    int ret = add_Last(NULL, fake_end, result_list);
    if (ret < 0) {
        free(fake_end);
        perror("Closing result list");
        errno = 0;
    }
    pthread_mutex_unlock(&mutex_last);
    // fine test lista
    test_error_isNot(0, pthread_cancel(printing_thread), "Canceling thread");
    test_error_isNot(0, pthread_join(printing_thread, NULL), "Joining back printing thread");
    
    // free list after printing
    size_t check_list_size = result_list->size;
    test_error_isNot(check_list_size, delete_List(&result_list, &free), "Freeing up list space");
    DEBUG_PRINT(fprintf(stdout, "Collector list test concluso\n"));

    pthread_mutex_destroy(&mutex_last);

    DEBUG_PRINT( printf("end collector\n");)
    unlink(my_address.sun_path);

    close(listen_sck);
    exit(EXIT_SUCCESS);
}


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

// function that processes a request (TODO: everything. Will have the values-list as second arg probably)
void use_client(int client_socket) {
    printf("In use_client");
    char myBuffer [1000] = "";
    int err = 0, close = 0;
    fprintf(stderr, "Using client %d\n", client_socket);
    do {
        err = read(client_socket, myBuffer, 5);
        if (err<0) {
            if (errno == EWOULDBLOCK) {
                errno = 0;
                break;
            } else {
                perror("Reading client");
                break;
            }
        }

        if (err > 0) {
            fprintf(stdout, "Collector received %s\n", myBuffer);
            if (myBuffer[0] == 'H')
                close = 1;
        } 
    } while (err > 0 && close == 0);
    fprintf(stdout, "collector done reading -> err:%d\n", err);
}
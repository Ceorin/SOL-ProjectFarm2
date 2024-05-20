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
    // create list TODO
    list_t* result_list = empty_List(false);

    // create printing thread TODO

    pthread_t printing_thread;
    test_error_isNot(0, errno = pthread_create(&(printing_thread), NULL, &printingthread, (void*)result_list), "Creating printing thread");
    test_error_isNot(0, errno = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL), "setting thread to be cancelable")
    
    // create server socket
    /*
    int fd_server; // listening socket
    int fd_client; // accept => worker
    int num_fds; // # fds validi in poll ad inizio e fine ciclo. Durante viene usato invece per tenere l'ultimo indice "aggiornabile";
    int tmp_size; // memorizzazione dei num_fds temporanea
    int poll_result; // return della poll, # di descrittori che hanno eventi

    int readB;
    char buf[100];
    

    */
    // test insert for sorting
    for (int i = 0; i < 20; i++) {
        result_value *wrapper;
        test_error(NULL, wrapper = (result_value*) malloc(sizeof(result_value)), "Creating result value mockup");
        snprintf (wrapper->name, 10, "Ciao%d", i);
        wrapper->sumvalue = (i%2 == 0) ? (i*2) : (i*i)-(i+1);
        pthread_mutex_lock(&mutex_last);
        int ret = add_Last(NULL, wrapper, result_list);
        pthread_mutex_unlock(&mutex_last);
        if (ret < 0) {
            free(wrapper);
            fprintf (stderr, "%d\t", ret);
            perror("Adding mockup!");
            errno = 0;
        } else {
            DEBUG_PRINT(fprintf(stdout, "file?: %s\n", wrapper->name);)
        }
        if (i%8 == 0) {
            //sleep(1);
        } else {
            struct timespec x;
            x.tv_sec=0;
            x.tv_nsec = i*10000000;
            nanosleep(&x, NULL);
        }
    }
    pthread_mutex_lock(&mutex_last);
    char* temp = (char*) malloc (sizeof (char)*10);
    strncpy(temp, "end", sizeof(char)*10);
    int ret = add_Last(NULL, temp, result_list);
    pthread_mutex_unlock(&mutex_last);
    // fine test lista

    test_error_isNot(0, pthread_cancel(printing_thread), "Canceling thread");
    test_error_isNot(0, pthread_join(printing_thread, NULL), "Joining back printing thread");
    size_t check_list_size = result_list->size;
    test_error_isNot(check_list_size, delete_List(&result_list, &free), "Freeing up list space");
    DEBUG_PRINT(fprintf(stdout, "Collector list test concluso\n"));


    // starting socket testing
    printf("\n***Starting socket test***\n");

    int listen_sck, client_fd;
    struct sockaddr_un my_address;
    strncpy(my_address.sun_path, "test_socket.sck", UNIX_SOCKPATH_MAX);
    my_address.sun_family = AF_UNIX;

    unlink(my_address.sun_path);
    errno = 0;// errno ignored, bind will trigger the same errors

    listen_sck = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    perror("created collector socket");
    bind (listen_sck, (struct sockaddr*) &my_address, sizeof(my_address));
    perror("bound collector socket");
    listen(listen_sck, SOMAXCONN);
    perror("listening started");

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
        sleep(1);   
        fprintf(stdout, "Waiting on poll\n");
        test_error(-1, poll_res = poll(pFDs, num_FDs, 5000), "Poll failed");
        if (poll_res == 0){
            fprintf(stdout, "Timed out!\n");
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
                fprintf(stdout, "***Listen socket reading***\n");
                do {
                    client_fd = accept(listen_sck, NULL, 0);
                    if (client_fd <0) {
                        if (errno == (EWOULDBLOCK | EAGAIN)) // no read to do
                            errno = 0;
                        else 
                            perror("Accepting clients");
                    } else {
                        fprintf(stdout, "Accepted client on fd: %d\n", client_fd);
                        if (num_FDs == pollsize) {
                            fprintf(stderr, "Poll is full! Create a bigger one\n");
                        } else {
                            pFDs[num_FDs].fd = client_fd;
                            pFDs[num_FDs].events = POLLIN;
                            num_FDs++;
                        }
                    }
                } while (client_fd >= 0);
            } else { // a connection wants to write something
                printf("Client%d wants to write\n", pFDs[i].fd);
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
                
                result_value test;
                strncpy(test.name, buf, sizeof(test.name));
                memcpy(&(test.sumvalue), buf+sizeof(test.name), sizeof(test.sumvalue));
                printf("client said: %s : %lld\n", test.name, test.sumvalue); 

                if (!strncmp(test.name, "../", sizeof("../"))) {
                    pFDs[i].fd = -1;
                    num_FDs--;
                    close_requested = 1;
                } else if (!strncmp(test.name, "./", sizeof("./"))) {
                    pFDs[i].fd = -1; // I don't want to write anymore anyway
                    num_FDs--;
                }  
            }

        }

        // debug
        fprintf(stdout,  "FD inside now:\n");
        for (int i = 0; i<num_FDs; i++) {
            printf("%d\t", pFDs[i].fd);
        }

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

        // debug
        fprintf(stdout,  "FD inside after removal:\n");
        for (int i = 0; i<num_FDs; i++) {
            printf("%d\t", pFDs[i].fd);
        }
        fflush(stdout);
        
        
        if (close_requested) {
            if (num_FDs == 1)
                go = 0;
            else {
                printf("Close requested but can't yet\n");
            }
        }
    }

    printf("end connection test\n");
    unlink(my_address.sun_path);
    
}
/*
        client_fd = accept(listen_sck, NULL, 0);
        if (client_fd == -1) {
            fprintf(stderr, "client %d\t", client_fd);
            perror("accept");
            go = 0;
        } else {
            int client_go = 1;
            do {
                printf("Client%d wants to write\n", client_fd);
                char buf[300];
                int last_read, bytes_read = 0, bytes_to_read = sizeof(result_value);
                while (bytes_to_read > 0) { 
                    last_read = read(client_fd, &(buf[bytes_read]), bytes_to_read);
                    if (last_read < 0)
                        perror("reading");
                    bytes_read += last_read;
                    bytes_to_read -= last_read;
                }
                if (bytes_to_read != 0)
                    perror("finishing read");
                
                result_value test;
                strncpy(test.name, buf, sizeof(test.name));
                memcpy(&(test.sumvalue), buf+sizeof(test.name), sizeof(test.sumvalue));

                printf("client said: %s : %lld\n", test.name, test.sumvalue); 
                if (!strncmp(test.name, "../", sizeof("./"))) {
                    client_go = go = 0;
                } else if (!strncmp(test.name, "./", sizeof("./"))) {
                    client_go = 0;
                }
                printf("Client has more? %d\n", client_go);
            } while (client_go);
        }
    }*/

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
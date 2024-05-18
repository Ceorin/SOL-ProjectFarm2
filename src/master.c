#include "utils.h"
#include "myList.h"
#include "master.h"
#include "worker_pool.h"

// NOTA - UNA MACRO POSIX CAMBIA IL COMPORTAMENTO DI GETOPT
// usleep is deprecated thus must use nanosleep
// are there better posix sources?
//#define _POSIX_C_SOURCE 199309L 
// ^ Defined with pthread
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <pthread.h>

struct dir_or_file {
    enum {_is_D, _is_F} dor_type;
    char pathname [1+_DEF_PATHNAME_MAX_SIZE];
} typedef maybeFile;

void masterThread(int argc, char** argv) {
    fprintf(stdout, "Parte master in esecuzione!\n");
    fflush(stdout);

    size_t qlen = _DEF_QLEN;
    size_t thread_num = _DEF_NTHREAD;
    struct timespec qdelay;

    errno = 0;
    list_t *maybe_files = empty_List(false); // FALSE = it's not a unique set
    if (maybe_files == NULL) {
        perror("Creating list");
        exit(EXIT_FAILURE);
    }

    
    // getting main options
    char opt;
    while ((opt = getopt(argc, argv, "-n:q:d:t:")) != -1) {
        char* leftovers;
        long int tempNum;

        switch (opt) {  
            case 'n': // set thread_num
                if (!(optarg && *optarg)) 
                    fprintf(stderr, "Error reading -%c!\t", opt);
                else {
                    errno = 0;
                    tempNum = strtol(optarg, &leftovers, 10);
                    if (errno == ERANGE) {
                        fprintf (stderr, "%c: Out of bound\n", opt);
                        break;
                    }

                    if (leftovers[0]!='\0') { //not a number
                        // Checks to see if it's an option (i.e. missing); separated for a better understanding
                        if (tempNum==0) // 0@string or fully a string
                        if ((optarg[0]!='0') && (optarg[1] !='0')) // it's not 0 nor '+0' or '-0'. (other options with 0 as the second character will be strings we don't care about anyway) 
                        if (leftovers[0]== '-') // is it -n -q -t or -d?
                            if (leftovers[1] == 'n' || leftovers[1] == 'q' || leftovers[1] == 't' || leftovers[1] == 'd') {
                                fprintf (stderr, "%c: missing argument! (cannot use %s)\n", opt, optarg);
                                optind -=1; // use this optarg as next opt
                                break;
                            }

                        // It failed one of the checks, it's not an option
                        fprintf(stderr, "%c: %s must be a number\n", opt, optarg);
                        break;
                    }

                    if (tempNum <= 0) {
                        fprintf(stderr, "%c: minimum one thread\n", opt);
                        break;
                    } 
                    // if tempNum > ? upperbound?

                    thread_num = tempNum;
                    // DEBUG
                    fprintf (stdout, "%c set to %ld successfully!\n", opt, thread_num);
                }
                break;
            case 'q': // set qlen
                if (!(optarg && *optarg)) // Se non sono settati bene i parametri
                    fprintf(stderr, "Error reading -%c!\n", opt);
                else {
                    errno = 0;
                    tempNum = strtol(optarg, &leftovers, 10);
                    if (errno == ERANGE) {
                        fprintf (stderr, "%c: Out of bound\n", opt);
                        break;
                    }

                    if (leftovers[0]!='\0') { //not a number
                        // Checks to see if it's an option (i.e. missing); separated for a better understanding
                        if (tempNum==0) // 0@string or fully a string
                        if ((optarg[0]!='0') && (optarg[1] !='0')) // it's not 0 nor '+0' or '-0'. (other options with 0 as the second character will be strings we don't care about anyway) 
                        if (leftovers[0]== '-') // is it -n -q -t or -d?
                            if (leftovers[1] == 'n' || leftovers[1] == 'q' || leftovers[1] == 't' || leftovers[1] == 'd') {
                                fprintf (stderr, "%c: missing argument! (cannot use %s)\n", opt, optarg);
                                optind -=1; // use this optarg as next opt
                                break;
                            }

                        // It failed one of the checks, it's not an option
                        fprintf(stderr, "%c: %s must be a number\n", opt, optarg);
                        break;
                    }

                    if (tempNum <= 0) {
                        fprintf(stderr, "%c: must be a positive number\n", opt);
                        break;
                    } 
                    // if tempNum > ? upperbound?

                    qlen = tempNum;
                    // DEBUG
                    fprintf (stdout, "%c set to %ld successfully!\n", opt, qlen);
                }
                break;
            case 't': // set qdelay
                if (!(optarg && *optarg)) // Se non sono settati bene i parametri
                    fprintf(stderr, "Error reading -%c!\n", opt);
                else {
                    errno = 0;
                    tempNum = strtol(optarg, &leftovers, 10);
                    if (errno == ERANGE) {
                        fprintf (stderr, "%c: Out of bound\n", opt);
                        break;
                    }

                    if (leftovers[0]!='\0') { //not a number
                        // Checks to see if it's an option (i.e. missing); separated for a better understanding
                        if (tempNum==0) // 0@string or fully a string
                        if ((optarg[0]!='0') && (optarg[1] !='0')) // it's not 0 nor '+0' or '-0'. (other options with 0 as the second character will be strings we don't care about anyway) 
                        if (leftovers[0]== '-') // is it -n -q -t or -d?
                            if (leftovers[1] == 'n' || leftovers[1] == 'q' || leftovers[1] == 't' || leftovers[1] == 'd') {
                                fprintf (stderr, "%c: missing argument! (cannot use %s)\n", opt, optarg);
                                optind -=1; // use this optarg as next opt
                                break;
                            }

                        // It failed one of the checks, it's not an option
                        fprintf(stderr, "%c: %s must be a number\n", opt, optarg);
                        break;
                    }
                    
                    if (tempNum <= 0) {
                        fprintf(stderr, "%c: must be a positive number\n", opt);
                        break;
                    } 
                    // a very big delay will end up doing nothing anyway
                    if (tempNum > MAX_DELAY) {
                        fprintf(stderr, "%c: maximum amount of delay set to %d millisecond, such delay will be used\n", opt, MAX_DELAY);
                        tempNum = MAX_DELAY;
                    }

                    qdelay.tv_sec = tempNum/1000;
                    qdelay.tv_nsec = (tempNum%1000) * 1000000;
                    // DEBUG
                    fprintf (stdout, "%c set to [%lds : %ldns] successfully!\n", opt, qdelay.tv_sec, qdelay.tv_nsec);
                }
                break;
            case 'd': // check if optarg is a valid directory, then adds it to the to-do-list 
                if (!(optarg && *optarg)) // Se non sono settati bene i parametri
                    fprintf(stderr, "Error reading -%c!\n", opt);
                else { // TODO creat dir list structure and manage it here
                    if (optarg[0]=='-') { // missing argument?
                    if (optarg[1]=='n' || optarg[1] =='q' || optarg[1] =='t' || optarg[1] == 'd') {
                        fprintf (stderr, "%c: missing argument! (cannot use %s)\n", opt, optarg);
                        optind -=1; // use this optarg as next opt
                        break;
                        }
                    }
                    // check about it being a valid path later
                    maybeFile *wrapper;
                    test_error(NULL, wrapper = (maybeFile*) malloc(sizeof(maybeFile)), "Creating dir or file wrapper");
                    strncpy (wrapper->pathname, optarg, _DEF_PATHNAME_MAX_SIZE);
                    wrapper->dor_type = _is_D;
                    int ret = add_Last(NULL, wrapper, maybe_files); 
                    if (ret < 0) {
                        free(wrapper);
                        fprintf (stderr, "%d\t", ret);
                        perror("Adding a directory");
                        errno = 0;
                    } else {  
                        // DEBUG
                        fprintf(stdout, "%c - dir?: %s\n", opt, optarg);
                    }
                }
                break;
            case 1: // is a file? non-opt argument
                if (!(optarg && *optarg)) // Se non sono settati bene i parametri
                    fprintf(stderr, "Error reading -%c!\n", opt);
                else { // TODO creat dir list structure and manage it here
                    if (optarg[0]=='-') { // Is it an option on its own?
                    if (optarg[1]=='n' || optarg[1] =='q' || optarg[1] =='t' || optarg[1] == 'd') {
                        fprintf (stderr, "%s: missing argument\n", optarg);
                        break;
                        }
                    }
                    // check about it being a valid path later
                    maybeFile *wrapper;
                    test_error(NULL, wrapper = (maybeFile*) malloc(sizeof(maybeFile)), "Creating dir or file wrapper");
                    strncpy (wrapper->pathname, optarg, _DEF_PATHNAME_MAX_SIZE);
                    wrapper->dor_type = _is_F;
                    int ret = add_Last(NULL, wrapper, maybe_files); 
                    if (ret < 0) {
                        free(wrapper);
                        fprintf (stderr, "%d\t", ret);
                        perror("Adding a file!");
                        errno = 0;
                    } else {  
                        // DEBUG
                        fprintf(stdout, "file?: %s\n", optarg);
                    }
                }
                break;
            case ':': // Errors to check..?
            case '?':
            default:
                if (!(optarg && *optarg))
                    fprintf(stderr, "%c not recognized\n", optopt);
                else
                    fprintf(stderr, "%c %s not recognized\n", optopt, optarg);
                break;
        }
    }

    // extra debug or valid?
    if (optind < argc) {
        fprintf (stderr, "Argument not recognized: %s\n", argv[optind]);
    }
    /* Old version - posix changes getopt behaviour so that it ends on the first non-option while missing options after that.
    for (int i = optind; i<argc; i++) { // check files validity
        fprintf(stdout, "File?: %s\n", argv[i]);
    } */
    
    /* TEST if list has been created properly
    
    fprintf(stdout, "printing list:\n");
    fprintf(stdout, "Size: %d\t Head:%p\t Last:%p\t Unique? %s\n", maybe_files->size, (void*) maybe_files->head, (void*) maybe_files->last, (maybe_files->unique) ? "Yes" : "No");
    maybeFile* test;
    int i = 0;
    while (i < maybe_files->size) {
        test = list_getAt(maybe_files, i, false);
        fprintf(stdout, "Element %d -> value: %s\ttype: %s\n", i, test->pathname, (test->dor_type == _is_D) ? "Dir" : "File");
        i++;
    }
    fprintf(stdout, "\n"); */
    
    
    // TEST DI CREAZIONE THREAD!
    fflush(stdout);
    int created_threads = init_worker_pool(qlen, thread_num);
    if (created_threads < thread_num) {
        fprintf(stderr, "something went wrong, created only %d threads", created_threads);
    }
    fprintf (stdout, "Created thread pool");

    // sending test messages to threads
    for (int i = 0; i < maybe_files->size; i++) {
        maybeFile *test = (maybeFile*) list_getAt(maybe_files, i, false);
        int ret = send_request_to_pool(test->pathname);
        fprintf(stdout, "adding %s, returns %d\n", test->pathname, ret);        
        
        if (i%10 == 0) {
            int trydelete = delete_thread();
            if (trydelete == -2)
                perror("NOT INITIALIZED");
            else if (trydelete == -1)
                fprintf(stderr, "ONLY ONE THREAD");
            else
                fprintf(stderr, "deleted one thread\n");
        }
        if (i%13 == 0) {
            int tryadd = add_thread();
            if (tryadd < 1)
                fprintf (stderr, "cannot add threads\n");
            else   
                fprintf(stderr, "added one thread\n");
        }
    }
    size_t testSize = maybe_files->size;
    test_error_isNot(testSize, delete_List(&maybe_files, &free), "Deleting file and directory list");
    //TODO better
    sleep(1);
    test_error_isNot(0, destroy_pool(), "Deleting threadpool");
}//?
// NOTA - UNA MACRO POSIX CAMBIA IL COMPORTAMENTO DI GETOPT
// usleep is deprecated thus must use nanosleep
// are there better posix sources?
//#define _POSIX_C_SOURCE 199309L 
// ^ Defined with pthread
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <getopt.h>

#include <pthread.h>

#include "utils.h"
#include "myList.h"
#include "master.h"
#include "worker_pool.h"
#include "signal_handlers_master.h"

struct dir_or_file {
    enum {_is_D, _is_F} dor_type;
    char pathname [1+_DEF_PATHNAME_MAX_SIZE];
} typedef maybeFile;

int navigate_and_add(char*, list_t*);
int check_regular_file (char*);
int check_if_dir(char*);


void masterThread(int argc, char** argv) {
    DEBUG_PRINT(printf("Parte master in esecuzione!\n");)
    DEBUG_PRINT(fflush(stdout);)

    DEBUG_PRINT(pid_t me = getpid();) // will use for debugging prints later
    // signal handling
    if (handle_signals_master()) {
        perror("handling master");
    } else {
        DEBUG_PRINT(printf("Signal handler installed?\n");)
        DEBUG_PRINT(kill(me, SIGUSR1);)
        DEBUG_PRINT(printf("Sent usr1!\n");)
        DEBUG_PRINT(kill(me, SIGPIPE);)
        DEBUG_PRINT(printf("Ignored pipe?\n");)
        // DEBUG_PRINT(kill(me, SIGUSR1); kill(me, SIGHUP); kill(me, SIGUSR1); kill(me, SIGUSR2); kill(me, SIGUSR1); kill(me, SIGINT);)
        // DEBUG_PRINT(fprintf(stdout, "Is this 3 : 1?\t %d : %d\n", th_num_modify, terminate_th_pool);)
    }

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
                    DEBUG_PRINT(fprintf(stdout, "%c set to %ld successfully!\n", opt, thread_num);)
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

                    DEBUG_PRINT(fprintf(stdout, "%c set to %ld successfully!\n", opt, qlen);)
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

                    DEBUG_PRINT(fprintf(stdout, "%c set to [%lds : %ldns] successfully!\n", opt, qdelay.tv_sec, qdelay.tv_nsec);)
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
                    errno = 0;
                    int ret = add_Last(NULL, wrapper, maybe_files); 
                    if (ret < 0) {
                        free(wrapper);
                        fprintf (stderr, "%d\t", ret);
                        perror("Adding a directory");
                    } else {  
                        DEBUG_PRINT(fprintf(stdout, "%c - dir?: %s\n", opt, optarg);)
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
                        DEBUG_PRINT(fprintf(stdout, "file?: %s\n", optarg);)
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
    
    int created_threads = init_worker_pool(qlen, thread_num);
    if (created_threads < thread_num) {
        // exit failure?
        fprintf(stderr, "something went wrong, created only %d threads", created_threads);
    }
    DEBUG_PRINT(fprintf(stdout, "Created thread pool"));

    while (maybe_files->size >0) { // sending test messages to threads
        DEBUG_PRINT(fprintf(stdout, "signal vars: %d - %d\n", th_num_modify, terminate_th_pool);)
        if (terminate_th_pool == 1)
            break;
        int reqnum;
        if ((reqnum = th_num_modify) != 0) {
            th_num_modify -= reqnum; // Processed X amount of requests. if a USR signal got received between the if-guard and this command, it'll be processed in the next cycle
            if (reqnum > 0) { // adding reqnum thread (if possible)
                for (int i = 0; i < reqnum; i++) {
                    if (add_thread() <0) {
                        DEBUG_PRINT(fprintf(stdout, "Cannot add a thread\n");)
                    } else {
                        DEBUG_PRINT(fprintf(stdout,"Thread added successfully!\n");)
                    }
                }
            } else { // removing reqnnum threads (if possible)
                for (int i = 0; i > reqnum; i--) {
                    if (delete_thread() <0) {
                        DEBUG_PRINT(fprintf(stdout, "Cannot add a thread\n");)
                    } else {
                        DEBUG_PRINT(fprintf(stdout, "Thread removed successfully!\n");)
                    }
                }
            }
        }
        errno = 0;
        maybeFile *test = (maybeFile*) list_first(maybe_files, true);
        if (test == NULL && errno!=0)
            perror("picking argument from list");
        else {
            if (test->dor_type == _is_F) {
                if (check_regular_file(test->pathname) == 1) {
                    int ret = send_request_to_pool(test->pathname);
                    DEBUG_PRINT((stdout, "adding %s, returns %d\n", test->pathname, ret));       
                } else 
                    fprintf(stderr, "%s is not a valid file!\n", test->pathname); 
            } else if (test->dor_type == _is_D) {
                int ret = navigate_and_add(test->pathname, maybe_files);
                // DEBUG_PRINT(kill(me, SIGUSR2);kill(me, SIGUSR2);kill(me, SIGUSR2);kill(me, SIGUSR2);)
            }
        }
        free(test);
        
    }
    // freeing list space (if testSize > 0 => exited before consuming all items)
    size_t testSize = maybe_files->size;
    test_error_isNot(testSize, delete_List(&maybe_files, &free), "Deleting file and directory list");
    
    // writing number of working threads at exit 
    FILE *remember_to_write_thread_num = fopen("nworkeratexit.txt", "w");
    if (remember_to_write_thread_num == NULL) {
        DEBUG_PRINT (perror ("Cannot create nworkeratexit.txt");)
    } else {
        fprintf(remember_to_write_thread_num, "%d\n", pool_size());
        fclose(remember_to_write_thread_num);
    }

    // whether I exited from finishing the list of items or by terminating the pool, I need to free the memory.
    test_error_isNot(0, destroy_pool(), "Deleting threadpool");
            
    //TODO better
}//?

int navigate_and_add (char* dirname, list_t* list_of_files) {
    DEBUG_PRINT(fprintf(stdout, "%s is dir?\n", dirname);)
    DIR* ectory = opendir(dirname); 
    if (ectory == NULL) {
        char errmsg[300];
        snprintf(errmsg, 1+_DEF_PATHNAME_MAX_SIZE, "Opening dir %s", dirname);
        perror (errmsg);
        return -1;
    }
    struct dirent* current;
    while ((errno = 0, current = readdir(ectory))!=NULL) {
        DEBUG_PRINT(fprintf(stdout, "checking %s/%s\n", dirname, current->d_name);)
        if (!strncmp(current->d_name, ".", 2) || !strncmp(current->d_name, "..", 3))
            continue;
    #ifndef DT_DIR // if DT_DIR is undefined I'll use check functions that use stat and require a path
        char bufname[_DEF_PATHNAME_MAX_SIZE+30];
        snprintf(bufname, 1+_DEF_PATHNAME_MAX_SIZE, "%s/%s", dirname, current->d_name);
    #endif

    #ifdef DT_DIR
        if (current->d_type == DT_DIR) {
    #else
        if (check_if_dir(bufname) == 1) {
    #endif
            maybeFile *wrapper;
            test_error(NULL, wrapper = (maybeFile*) malloc(sizeof(maybeFile)), "Creating dir or file wrapper");
        #ifdef DT_DIR
            if (snprintf(wrapper->pathname, 1+_DEF_PATHNAME_MAX_SIZE, "%s/%s", dirname, current->d_name) > 1+_DEF_PATHNAME_MAX_SIZE)
                fprintf(stderr, "%s/%s is too long\n", dirname, current->d_name);
        #else
            if (snprintf(wrapper->pathname, 1+_DEF_PATHNAME_MAX_SIZE, "%s", bufname) > 1+_DEF_PATHNAME_MAX_SIZE)
                fprintf(stderr, "%s is too long\n", bufname);
        #endif
            else {
                wrapper->dor_type = _is_D;
                errno = 0;
                int ret = add_Last(NULL, wrapper, list_of_files); 
                if (ret < 0) {
                    free(wrapper);
                    fprintf (stderr, "%d\t", ret);
                    perror("Adding a directory");
                } else {
                    DEBUG_PRINT(fprintf(stdout, "From directory %s to %s, added!\n", dirname, current->d_name);)
                }
            }
    #ifdef DT_REG
        } else if (current->d_type == DT_REG || current->d_type == DT_UNKNOWN) { // would leave the unknown check to the file itself
    #else
        } else if (check_regular_file(bufname) == 1) {
    #endif
            maybeFile *wrapper;
            test_error(NULL, wrapper = (maybeFile*) malloc(sizeof(maybeFile)), "Creating dir or file wrapper");
        #ifdef DT_DIR
            if (snprintf(wrapper->pathname, 1+_DEF_PATHNAME_MAX_SIZE, "%s/%s", dirname, current->d_name) > 1+_DEF_PATHNAME_MAX_SIZE) {
                fprintf(stderr, "%s/%s is too long\n", dirname, current->d_name);
        #else
            if (snprintf(wrapper->pathname, 1+_DEF_PATHNAME_MAX_SIZE, "%s", bufname) > 1+_DEF_PATHNAME_MAX_SIZE) {
                fprintf(stderr, "%s is too long\n", bufname);
        #endif
            } else {
                wrapper->dor_type = _is_F;
                errno = 0;
                int ret = add_Last(NULL, wrapper, list_of_files); 
                if (ret < 0) {
                    free(wrapper);
                    fprintf (stderr, "%d\t", ret);
                    perror("Adding a directory");
                } else {
                    DEBUG_PRINT(fprintf(stdout, "From directory %s, added file %s!\n", dirname, current->d_name);)
                }
            }
        }
    }
    if (errno!=0) {
        char errmsg[300];
        snprintf(errmsg, 1+_DEF_PATHNAME_MAX_SIZE, "Reading dir %s", dirname);
        perror (errmsg);
        return -1;
    }
    closedir(ectory);
    return 0;
}


int check_regular_file (char* pathname) {
    struct stat s; 
    if (stat(pathname, &s) == -1) {
        char errmsg[30+_DEF_PATHNAME_MAX_SIZE];
        snprintf(errmsg, 1+_DEF_PATHNAME_MAX_SIZE, "Reading stat - file %s", pathname);
        perror (errmsg);
        return -1;
    } 
    
    if (S_ISREG(s.st_mode))
        return 1;
    else
        return 0;
}

int check_if_dir (char* dirname) {
    struct stat s; 
    if (stat(dirname, &s) == -1) {
        char errmsg[_DEF_PATHNAME_MAX_SIZE+30];
        snprintf(errmsg, 1+_DEF_PATHNAME_MAX_SIZE, "Reading stat - file %s", dirname);
        perror (errmsg);
        return -1;
    }
    
    if (S_ISDIR(s.st_mode))
        return 1;
    else
        return 0;
}
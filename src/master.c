#include "utils.h"
#include "master.h"
#include "worker_pool.h"

// NOTA - UNA MACRO POSIX CAMBIA IL COMPORTAMENTO DI GETOPT
// usleep is deprecated thus must use nanosleep
// are there better posix sources?
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include <stdlib.h>
#include <errno.h>

void masterThread(int argc, char** argv) {
    fprintf(stdout, "Parte master in esecuzione!\n");
    fflush(stdout);

    size_t qlen = _DEF_QLEN;
    size_t thread_num = _DEF_NTHREAD;
    struct timespec qdelay;
    
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
                    // DEBUG
                    fprintf(stdout, "%c - dir?: %s\n", opt, optarg);
                }
                break;
            case 1: // is a file? non-opt argument
                if (!(optarg && *optarg)) // Se non sono settati bene i parametri
                    fprintf(stderr, "Error reading -%c!\n", opt);
                else { // TODO create file list structure and manage it here
                    // DEBUG
                    fprintf(stdout, "file?: %s\n", optarg);
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

}//?
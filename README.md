# SOL-Project-Farm
**Examination project for the Operative Systems course at unipi, 2023-2024**
University project - Client server multi-processed program which computes a function on multiple files

## Description

The project consists of making a multi-process multi-threaded applications denominated **'farm'** in an **UNIX** environment using C language and UNIX functionalities.
In particular, one process, from here onward called **'MasterWorker'**, will read multiple binary files and evaluate a function on each of them concurrently, sending the result to the second process, from here called **'Collector'**.

MasterWorker is a multi-threaded process that manages a thread pool with the single task to evaluate a function on a file received. It can be configured through command line optional arguments, and it will process -d arguments and undefined arguments as file to read.

Collector is a dual-threaded process that creates the connection sockets and act as a server, awaiting results from MasterWorker's threads.
Every second, it will print on the standard output the result it has taken in, in ascending order.

A predefined test is also present in the */test* folder.

## Installation & Run instrunction
The project can be cloned from this github repository or downloaded from it all the same.

It must run on an unix operative system.
To compile it, you can run 'make' in the project root folder from CLI, which will compile all the necessary files, in addition to the predefined test executable.
Object files will be compiled into the */build* directory and 3 executables will be generated in the project folder:
 - **farm**
 - **collector**
 - **generafile**

Collector should **NOT** be executed manually - its behaviour is still undefined in such a scenario.

To run the programm, launch ./farm with the following optional arguments:
 - -n [number of threads]
 - -q [concurrent queue size]
 - -t [produce task delay]
 - -d [DIR1] -d [DIR2]
 - [FILE1, FILE2]

While the program is running, SIGUSR1 and SIGUSR2 will allow respectively to increase or decrease the number of active threads in MasterWorker by one.
Sending either of these 4 signals - SIGHUP, SIGINT, SIGQUIT or SIGTERM - will notify the application to finish processing the active task and terminate.

The makefile can also be used to clean the project folder by running 'make clean'; in addition it is possible to build a mockup debug version of the application by running 'make debug'.

## Version
0.9 - Most core functionalities are up and running. Almost no extra functionality is available.
    Some optimization has been performed but the project requires cleaning and to fix some known bugs.

## Known bugs
**DANGEROUS**
 - There is a possibility for the main program to go into deadlock while closing. Further investigation needed, may require to refactor using non-detached threads.
**light**
 - RELATED TO THE ABOVE - There is a possibility that due to how the program termination can be faulty, the last result (and only the last, from what is known), can go missing.
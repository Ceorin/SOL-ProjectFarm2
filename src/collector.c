#include <stdio.h>
#include <sys/socket.h>
#include "utils.h"
#include <stdlib.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    if (argc != 1) {
        fprintf(stderr, "use as %s <socket name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    fprintf (stdout, "Collector creato\n");
    fprintf (stdout, "\narg1: %s\n", argv[1]);
    fflush(stdout);
    return 0;
} //?
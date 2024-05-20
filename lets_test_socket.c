#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#define SOCKNAME "test_socket.sck"
#define UNIX_PATH_MAX 108
#include <string.h>
#include "sumfun.h"

int main (int argc, char* argv[]) {
    int my_socket;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, "test_socket.sck", UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    my_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    while (connect (my_socket, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        if (errno = ENOENT) {
            fprintf(stderr, "Socket not found");
            sleep(1);
        } else {
            perror("Connecting to server");
            return -1;
        }
    }

    result_value buf;
    strncpy(buf.name, argv[1], 15);
    buf.sumvalue = 100;

    write (my_socket, &buf, sizeof(buf));
    return 0;
}
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

    for (int i = 1; i < argc; i++) {
        result_value temp;
        strncpy(temp.name, argv[i], sizeof(temp.name));
        temp.sumvalue = 100*i;

        char buf[300];
        strncpy(buf, temp.name, sizeof(temp.name));
        memcpy(buf+sizeof(temp.name), &(temp.sumvalue), sizeof(temp.sumvalue));

        int last_write, bytes_written = 0, bytes_to_write = sizeof(temp);
        while (bytes_to_write > 0) {
            last_write = write (my_socket, buf+bytes_written, bytes_to_write);

            if (last_write <0) {
                perror ("Writing");
            }

            bytes_written+= last_write;
            bytes_to_write -= last_write;
        }

        if (bytes_to_write != 0) 
            perror("finishing write");
    }
    return 0;
}
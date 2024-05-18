#include "sumfun.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* function reads from a file descriptor fd ad if sent only long integers
    and returns a result_value with name equal target and value equal to 
    Σ i*long[i], where i is the index of the i-nth long read from the descriptor    
    */
result_value sum_fun (char* target, int fd) {
    result_value result;
    long long sum = 0;
    long int i = 0;
    long int buf;
    ssize_t ret;   
    while (0 < (ret = read(fd, &buf, sizeof(long int)))) {
        sum += (buf * i);
        i++;
    }
    if (ret == -1) {
        // check errrori
    }
    strncpy(result.name, target, _DEF_PATHNAME_MAX_SIZE);
    result.sumvalue = sum;
    return result;
}

/*
int main () {
    int file_desc = open("../test_file", O_RDONLY);
    result_value x = sum_fun("test_file", file_desc);
    fprintf(stdout, "%s : %lld\n", x.name, x.sumvalue);
}*/
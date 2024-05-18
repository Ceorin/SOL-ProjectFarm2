#include "sumfun.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* function reads from a file descriptor fd ad if sent only long integers
    and returns a result_value with name equal target and value equal to 
    Σ i*long[i], where i is the index of the i-nth long read from the descriptor    */
result_value sum_fun (char* target, int fd) {
    result_value result;
    long long sum = 0;
    long int i = 0;
    long int buf;
    ssize_t ret;   
    while (0 < (ret = read(fd, &buf, sizeof(long int)))) {
        if ((char) buf == '\n')
            break;
        sum += (buf * i);
        i++;
    }
    if (ret == -1)
        // check errrori
    strncpy(result.name, target, _DEF_PATHNAME_MAX_SIZE);
    result.sumvalue = sum;
    return result;
}

#define MUTLIPLIER 16
result_value fast_sum_fun (char* target, int fd) {
    result_value result;
    long long sum = 0;
    long int i = 0;
    long int buf[MUTLIPLIER];
    ssize_t ret;   
    while (0 <(ret = read(fd, &buf, MUTLIPLIER*sizeof(long int)))) {
        if (ret%MUTLIPLIER != 0) {
            // errore?
        }
        long int section = ret/MUTLIPLIER;
        for (long int j = 0; j<section; j++)
            sum += (buf[j] * i+j);
        i+=section;
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
    result_value x = sum_fun("test", file_desc);
    fprintf(stdout, "%s : %lld\n", x.name, x.sumvalue);
} */
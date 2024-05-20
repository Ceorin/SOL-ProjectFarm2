#include <time.h>
#include "collector_print.h"
#include "sumfun.h"
#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

int print = 1; // continue printing

node_t* order_list_first(node_t*, node_t*);

/* concurrent access only on size and will ORDER and print up to the (size-1)-nth element 
    (by not touching the last element of the list, by it being a linked list and 
    always adding at the end, we should be able to access no other datum concurrently */
void order_print_result_list(list_t* l) {
    //lock
    pthread_mutex_lock(&mutex_last);
    DEBUG_PRINT(printf("%d\t", l->size));
    node_t* tempLast = l->last;
    unsigned int lines_num = l->size;
    DEBUG_PRINT(printf("%d\t", lines_num));
    pthread_mutex_unlock(&mutex_last); 
    // unlock here?
    l->head = order_list_first(l->head, tempLast); // do I need to generate intermediate result?
    // unlock
    // does not print the last one - during execution it won't have been ordered!
    lines_num = (lines_num == 0) ? 0 : lines_num-1;
    for (unsigned int i = 0; i < lines_num; i++) {
        result_value *temp = (result_value*) list_getAt(l, i, false);
        fprintf(stdout, "%s : %lld\n", temp->name, temp->sumvalue);
    }
}

void last_print (void* arg) {
    order_print_result_list((list_t*) arg);
}

void* printingthread (void* arg) {
    list_t * mylist = (list_t*) arg;
    pthread_cleanup_push(last_print, arg); // will print one last time when canceled
    struct timespec print_time, rem_time;
    print_time.tv_nsec = 0;
    print_time.tv_sec = 1;
    int last_ret = 0;
    while (1) {
        if (last_ret == 0) {
            DEBUG_PRINT( fprintf(stdout, "normal sleep");)
            last_ret = nanosleep(&print_time, &rem_time);
            if (last_ret != 0 && errno != EINTR) {
                break;
            }
        } else {
            DEBUG_PRINT (fprintf(stdout, "interrupted?\n");)
            last_ret = nanosleep(&rem_time, &rem_time);
        }
        DEBUG_PRINT(fprintf(stdout, " * * * PRINT - list_t_size = %d * * *!\n", mylist->size));
        order_print_result_list(mylist);
    }
    pthread_cleanup_pop(1);
    return 0;
}

int compare (node_t *a, node_t *b) {
    if (((result_value*)(a->item))->sumvalue >= ((result_value*)(b->item))->sumvalue)
        return 1;
    else
        return 0;
}

/* TODO implement 
    Sorting algorithm is an iterative MergeSort developed by sir. Simon Tatham
    I used this to optimize for efficiency in space and memory 
    Given I need to sort until a certain element, it behaves similarly to a circular list */ 
node_t* order_list_first(node_t* node_list, node_t* _DONT) {
    int insize, nmerges, psize, qsize, i;
    node_t *p, *q, *e, *tail;
    if (!node_list)
        return NULL;

    insize = 1;

    while (1) {
        p = node_list;
        tail = NULL;

        nmerges = 0;

        while (p) {
            nmerges++;
            q = p;
            psize = 0;
            for (i = 0; i< insize; i++) {
                psize++;
                q = (q->next == _DONT) ? NULL : q->next;
                if (!q)
                    break;
            }
            qsize = insize;

            while ((psize >0) || (qsize > 0 && q)) {
                if (psize == 0) { // just second (q's) part
                    e = q;
                    q = (q->next == _DONT) ? NULL : q->next;
                    qsize--;
                } else if (qsize == 0 || !q) { // just first (p's) part
                    e = p;
                    p = (p->next == _DONT) ? NULL : p->next;
                    psize--;
                } else if (compare(p, q)) { // p >= q
                    e = q;
                    q = (q->next == _DONT) ? NULL : q->next;
                    qsize--;
                } else { // p < q
                    e = p;
                    p = (p->next == _DONT) ? NULL : p->next;
                    psize--;
                }

                if (tail) // we're mid-order
                    tail->next = e;
                else // we just started ordering or we found a new first
                    node_list = e;
                tail = e;
            }
            p = q; // p went [insize] steps into the list and q has too
        }
        tail->next = _DONT; // should always be last member here
        if (nmerges <= 1) // we did only 0 or 1 merge (didn't go through the p^iteration more than once this cycle)
            return node_list;
        // if we don't exit
        insize *= 2;
    }
}
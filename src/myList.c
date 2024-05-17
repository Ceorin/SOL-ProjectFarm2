#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "myList.h"

/*  THIS DATA STRUCTURE CAN BE USED AS A LINKED LIST, A QUEUE, A STACK.
    It stores generic item as void pointers.
    (therefore cast and use different types at your own risk)

    The data structure can also be used as a set, not allowing elements with the same key.
    However, it is also possible to ignore key entirely and set it to NULL if unique is 
     set to false!

    All the functions might set errno in case of errors, some of which you might want to 
     handle (for example the case of adding a duplicate key into an unique list).         
     
    P.S. A main to test exists at the bottom of the file, although it is wrapped as a comment. */
struct node {
    node_t* next;
    char* key;
    void* item;
};


// Initializes an empty list
// returns NULL if malloc failes, or a pointer to an empty list_t otherwise
list_t* empty_List (bool unique) {
    list_t* newList = (list_t *) malloc (sizeof(list_t));
    if (newList == NULL) // error
        return NULL;

    newList->head = NULL;
    newList->last = NULL;
    newList->size = 0;
    newList->unique = unique;

    return newList;
}

// Returns a node_t ptr.
// This is an internal function and shouldn't be used outside of this file.
node_t * create_Node (char* key, void* val) {
    node_t* newNode = (node_t*) malloc (sizeof(node_t));
    if (newNode == NULL) // error
        return NULL;

    newNode->next = NULL;
    if (key == NULL)
        newNode->key = NULL;
    else {
        newNode->key = (char*) calloc (1+strlen(key), sizeof(char));
        strcpy(newNode->key, key);
    }
    newNode->item = val;


    return newNode;
}

/* Look for a node with the key given in the list and returns an array of 2 nodes, containing
    the element and its predecessor in the list.
   Return NULL in case of error, or an array of 2 node_t* in case of success.
   In case a node with the key exists, the array will contain that node and its predecessor,
    otherwise it will contain NULL and list->last (or at least, return[1] SHOULD be list->last)
   This is an internal function and shouldn't be used outside of this file.                     */
node_t ** find (char* key, list_t* list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (key == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (list->size == 0)
        return NULL;
    if (list->head == NULL) {
        errno = EFAULT; // broken state of the list
        return NULL;
    }

    node_t ** temp = (node_t**) calloc (2, sizeof(node_t*));
    if (temp == NULL)
        return NULL; // error upthrows

    temp[0] = list->head;
    temp[1] = NULL; // predecessor

    while (temp[0] != NULL) {
        if (temp[0]->key != NULL)
            if (!strcmp(temp[0]->key, key)) // found
                return temp;
        temp[1] = temp[0];
        temp[0] = temp[0]->next;
    }

    if (temp[0] == NULL) {
        free(temp);
        return NULL;
    } else
        return temp;
}

// Adds a node to the tail of a list.
// If an error occurs, returns -1 and sets errno, otherwise returns the list new size.
int add_Last (char* key, void* val, list_t* list) {
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (list->unique) {
        node_t **nodes = find(key, list);
        if (nodes == NULL) { // NULL & errno = 0 -> not found -> good!
            if (errno!=0) // other errors, like list or key null, or error allocating memory
                return -2;
        } else if (nodes[0] != NULL) { // found! Well, we didn't want that now
            free(nodes);
            errno = EEXIST;
            return -3;
        } else if (errno != 0) { // weird error status
            free(nodes);
            return -4;
        }
    }

    node_t* newVal = (node_t*) create_Node(key, val);
    if (newVal == NULL)
        return -5;

    if (list->size == 0) {
        list->head = newVal;
        list->last = newVal;
    } else {
        list->last->next = newVal;
        list->last = newVal;
    }

    list->size++;

    return list->size;
}


// add a node to the tail of a list.
// if an error occurs, returns -1 and sets errno, otherwise returns the list new size.
int add_Head (char* key, void* val, list_t* list) {
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    if (list->unique) {
        node_t **nodes = find(key, list);
        if (nodes == NULL) { // NULL & errno = 0 -> not found -> good!
            free(nodes); 
            if (errno!=0) // other errors, like list or key null, or error allocating memory
                return -2;
        } else if (nodes[0] != NULL) { // found! Well, we didn't want that now
            free(nodes);
            errno = EEXIST;
            return -3;
        } else if (errno != 0) { // weird error status
            free(nodes);
            return -4;
        }
    }

    node_t* newVal = (node_t*) create_Node(key, val);
    if (newVal == NULL)
        return -5;

    newVal->next = list->head;
    list->head = newVal;

    if (list->size == 0)
        list->last = newVal;

    

    list->size++;

    return list->size;
}

/* Returns the item in the first node of the list (note - it's a void*, a cast will be necessary). 
 If remove is set true, it is also removed from the list, and the size reduced by 1.
 Returns NULL both in case of error and of null value to return!
 Sets errno in case of error - EINVAL for a bad argument. 

 DO NOTE: this operation will free the node, but the argument inside given back! Remember to free 
 the use!                                                                                         */
void* list_first (list_t* list, bool remove) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (list->size == 0)
        return NULL;

    if (!remove) {
        if (list->head == NULL) {
            errno = EFAULT; // broken state of the list
            return NULL;
        } else
            return list->head->item;
    }

    void* returnVal = list->head->item;

    node_t *temp = list->head;

    list->head = list->head->next;
    list->size--;

    if (list->size == 0)
        list->last = NULL;
    
    if (temp->key != NULL)
        free(temp->key);
    free(temp);

    return returnVal;
}

/* Returns the item in the last node of the list (note - it's a void*, a cast will be necessary). 
 If remove is set true, it is also removed from the list, and the size reduced by 1.
 Returns NULL both in case of error and of null value to return!
 Sets errno in case of error - EINVAL for a bad argument, or EFAULT if the list internal structure 
 is broken.                                                                
                       
  DO NOTE: this operation will free the node, but, of course, not the value given back! 
    Remember to free it after the use!                                                            */ 
void* list_last (list_t* list, bool remove) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (list->size == 0)
        return NULL;

    if (!remove) {
        if (list->last == NULL) {
            errno = EFAULT; // broken state of the list
            return NULL;
        } else
            return list->last->item;
    }

    if (list->head == NULL) {
            errno = EFAULT; // broken state of the list
            return NULL;
    }
    void* returnVal;

     // 1 element -> must remove that and set nulls
    if (list->head->next == NULL) {
        returnVal = list->head->item;
        if (list->head->key != NULL)
            free(list->head->key);
        free(list->head);

        list->head = list->last = NULL;
        list->size = 0;
        return returnVal;
    }
    
    // 2 or more elements -> navigate until second-to-last one, which will be the new last
    node_t *temp = list->head;
    while(temp->next->next != NULL) {
        temp = temp->next;
    }

    returnVal = list->last->item;
    temp->next = NULL;
    
    if (list->last->key != NULL)
        free(list->last->key);
    free(list->last);

    list->last = temp;
    list->size--;

    return returnVal;
}


/* Returns the item in N-th position. If remove is set true, it is also removed from the list, 
  and the size reduced by 1.
 Returns NULL both in case of error and of null value to return!
 Sets errno in case of error - EINVAL if the list is empty, ERANGE if the index is out of bound,
  and EFAULT if the list internal structure is broken.    
  
  DO NOTE: this operation will free the node, but, of course, not the value given back! 
    Remember to free it after the use!                                                          */
void* list_getAt(list_t* list, unsigned int index, bool remove) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (list->size <= index) {
        errno = ERANGE;
        return NULL;
    }
    
    // this will ease the case of removing and replacing head
    if (index == 0)
        return list_first(list, remove);
    if (index == list->size -1)
        return list_last(list, remove);

    if (list->head == NULL) { 
        // should have blocked at EINVAL - list->size > index >= 0 given it's unsigned int
        errno = EFAULT;
        return NULL;
    }

    
    node_t *temp = list->head;

    if (temp == NULL) {
        // should have blocked before: To get here, size must be > index 
        errno = EFAULT;
        return NULL;
    }

    unsigned int i = 1;
    while (temp->next != NULL && i<index) {
        temp = temp->next;
        i++;
    }

    if (temp == NULL) {
        // should have blocked before: To get here, size must be > index 
        errno = EFAULT;
        return NULL;
    }
    if (temp->next == list->last) {
        // should have blocked before: if index == size-1 => the function called list_last instead
        errno = EFAULT;
        return NULL;
    }

    void* returnVal = temp->next->item;
    if (!remove)
        return returnVal;

    node_t *toRemove = temp->next;
    // Removing the node to remove from the linked list
    temp->next = toRemove->next;
    toRemove->next = NULL;

    list->size--;

    if (toRemove->key != NULL)
        free(toRemove->key);
    free(toRemove);

    return returnVal;
}


/* Returns the (first) item with the associated key in the list.
    If remove is true, then the item will be removed from the list
    If found is not null, it will be set to true or false depending on if an item with 
        the given key exists in the list

  Returns NULL both in case of no result and in case of errors: errno might be set 
        and it might be proper to check it

  DO NOTE: this operation will free the node, but, of course, not the value given back! 
    Remember to free it after the use!                                                  */
void* list_get (list_t* list, char* key, bool remove, int* found) {
    node_t ** nodes = find(key, list);
    if (nodes == NULL) { // either error or not found, check errno.
        if (found != NULL)
            *found = false;
        free(nodes);
        return NULL;
    }

    void* valToReturn = nodes[0]->item;
    
    if (found != NULL) {
        *found = (nodes[0] != NULL);
        if (!(*found)) {
            free(nodes);
            return NULL;
        }
    }
    if (!remove) {
        free(nodes);
        return valToReturn;
    }

    list->size--;

    if (nodes[1] == NULL) { // nodes[0] == list->head
        list->head = list->head->next;
        nodes[0]->next = NULL;
        if (list->size == 0)
            list->last = NULL;

        
    } else {
        nodes[1]->next = nodes[0]->next;
        nodes[0]->next = NULL;
    }
    free(nodes[0]->key);
    free(nodes[0]);
    free(nodes);

    return valToReturn;
}

/* Frees all the items in the list given, and then the list itself, turning it into a NULL ptr.
 Returns the amount of elements removed
 If the returned value is lower than what the previously allocated list size was, the list was 
 badly structured (and some memory leaks might have occurred).        

 DO NOTE: The second argument can be &free if the value of the list is a normal value, but in the
 case that it's a structure, to avoid memory leaks a proper function should be created which frees
 all the dynamically allocated fields of the structure and then itself.                         */
unsigned int delete_List (list_t** list, void (*freeFunction)(void*) ) {
    if (list == NULL)
        return 0;

    unsigned int count = 0;

    node_t* deleting;

    while ((*list)->head !=NULL) {
        deleting = (*list)->head;
        (*list)->head = (*list)->head->next;

        freeFunction(deleting->item);
        free(deleting->key);
        free(deleting);
        count++;
    }
    

    free(*list);
    *list = NULL; // in case the free implementation just unchecks the value

    return count;
}



/* EXAMPLE MAIN FOR TESTING (uncomment and recompile this file alone for that) 
//          THIS TEST WILL CURRENTLY DO THE FOLLOWING:
//    - initialize a list that only takes unique keys
//    - insert integer into the list using an int* wrapper as follows
//      1) inserts 10 with key 'prova' -> success
//      2) inserts 20 with key 'prova' -> failure -> frees the wrapper
//      3) inserts 40 with key 'provola' -> success
//      4) inserts 30 with key 'provati' -> success
//    - then tries to get certain values and converting them to integers, as follow:
//      1) tries to get the value with key 'hey' -> doesn't exists -> failure
//      2) tries to get the value with key 'prova', without removing it -> success
//      3) tries to get the value with key 'prova' and remove it from the list -> success
//      4) as above tries to get the value with key prova -> now it fails
//      5) starts getting all the items stored in the list from the 1st position forwards
//         and prints them -> prints 40 and 30 and stops
//    It is possible to check with valgrind or similar tools that there shouldn't be memory leaks
//    However the responsibility of freeing resources properly is left to the user.

#include <stdio.h>

void print_list(list_t list) {
    fprintf(stdout, "printing list:\n");
    fprintf(stdout, "Size: %d\t Head:%p\t Last:%p\t Unique? %s\n", list.size, list.head, list.last, (list.unique) ? "Yes" : "No");
    node_t * temp = list.head;
    unsigned int i = 0;
    while (temp != NULL) {
        fprintf(stdout, "Element %d at %p -> Key:%s\tvalue: %p\n", i, temp, temp->key, temp->item);
        i++;
        temp = temp->next;
    }
    fprintf(stdout, "\n");
}

int main () {
    list_t* testInt = empty_List(true);

    print_list(*testInt);

    if (errno!=0) {
        perror("Created list");
        exit(EXIT_FAILURE);
    }

    int ret;


    int* wrapper = (int*) malloc(sizeof(int));
    *wrapper = 10;
    ret = add_Last("prova", wrapper, testInt); 
    if (ret < 0) {
        free(wrapper);
        fprintf (stdout, "%d\t", ret);
        perror("Prova:10");
        errno = 0;
    } else
        printf("Prova:10 - success!\n");


    print_list(*testInt);
    // debug insertion
    wrapper = (int*) malloc(sizeof(int));
    *wrapper = 20;
    ret = add_Last("prova", wrapper, testInt);
    if (ret < 0) {
        free(wrapper);
        fprintf (stdout, "%d\t", ret);
        perror("Prova:20");
        errno = 0;
    } else
        printf("Prova:20 - success!\n");

    print_list(*testInt);

    // debug insertion
    wrapper = (int*) malloc(sizeof(int));
    *wrapper = 40;
    ret = add_Last("provola", wrapper, testInt);
    if (ret < 0) {
        free(wrapper);
        fprintf (stdout, "%d\t", ret);
        perror("Prova:40");
        errno = 0;
    } else
        printf("Prova:40 - success!\n");

    print_list(*testInt);

    // debug insertion
    wrapper = (int*) malloc(sizeof(int));
    *wrapper = 30;
    ret = add_Last("provati", wrapper, testInt);
    if (ret < 0) {
        free(wrapper);
        fprintf (stdout, "%d\t", ret);
        perror("Prova:30");
        errno = 0;
    } else
        printf("Prova:30 - success!\n");

    print_list(*testInt);

    // debug get!
    wrapper = (int*) list_get(testInt, "hey!", false, &ret);
    if (errno != 0)
        perror("Getting Hey");
    if (!ret)
        fprintf(stdout, "Hey not found\n");
    else {
        if (wrapper == NULL)
            perror ("why null?");
        else
            fprintf(stdout, "Hey found with value %d!\n", *wrapper);
    }
    // debug get!
    wrapper = (int*) list_get(testInt, "prova", false, &ret);
    if (errno != 0)
        perror("Getting prova");
    if (!ret)
        fprintf(stdout, "prova not found\n");
    else {
        if (wrapper == NULL)
            perror ("why null?");
        else
            fprintf(stdout, "prova found with value %d!\n", *wrapper);
    }
    // debug get!
    wrapper = (int*) list_get(testInt, "prova", true, &ret);
    if (errno != 0)
        perror("Getting prova");
    if (!ret)
        fprintf(stdout, "prova not found\n");
    else {
        if (wrapper == NULL)
            perror ("why null?");
        else {
            fprintf(stdout, "prova found with value %d!\n", *wrapper);
            free(wrapper); // found and with proper value - consumed -> now freed!
        }
    }
    // debug get!
    wrapper = (int*) list_get(testInt, "prova", false, &ret);
    if (errno != 0)
        perror("Getting prova");
    if (!ret)
        fprintf(stdout, "prova not found\n");
    else {
        if (wrapper == NULL)
            perror ("why null?");
        else
            fprintf(stdout, "prova found with value %d!\n", *wrapper);
    }

    fprintf(stdout, "List is:\n");
    do {
        wrapper = (int*) list_getAt(testInt, 0, true);
        if (wrapper == NULL)
            continue;
        fprintf(stdout, "%d\t", *wrapper);
        if ((*wrapper)%10==9)
            fprintf(stdout, "\n");
        fflush(stdout);
        free(wrapper);
    } while (wrapper!=NULL);

    delete_List(&testInt, &free);
    return 0;
} */
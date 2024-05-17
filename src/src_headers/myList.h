#ifndef MY_LIST_HEADER
#define MY_LIST_HEADER
    typedef struct node node_t;

    #include <stdbool.h>

    typedef struct list_t {
        node_t* head;
        node_t* last;
        unsigned int size;
        bool unique;
    } list_t;

  /* Initializes an empty list. 
     If the argument is true, then the list will only accept unique keys
     Returns NULL in case of error, setting errno                         */
    list_t* empty_List (bool); 
    
    
  /* Add the first argument to the list in the second argument.
     Returns negative values in case of error, setting errno
     DO NOTE - if the list is set as unique, then errno will be set as EEXIST.
        If you don't want your program to stop, you should catch at least that 
        error and set errno back to 0 after checking.                          */
    int add_Last (char*, void*, list_t*);
    int add_Head (char*, void*, list_t*);   

  /* Return the appropriate values from the list, and if the second argument is 
        true, the value will be removed from the list.
    In case of error, return NULL setting errno.
    DO NOTE, however, that NULL might be a valid return as well.              */
    void* list_first (list_t*, bool);
    void* list_last (list_t*, bool);
    void* list_getAt(list_t*, unsigned int, bool);

  /* Return the first value in the list which key matches the parameter given.
     if the 3rd argument is true, the value will be also removed from the list.
     if the 4th argument is not null, after the function returns, it will be set
        to either true or false depending on whether the list contained a value 
        with the key given
    In case of error, returns NULL setting errno.
    DO NOTE, however, that NULL might be a valid return as well, and might also
        be the value returned in case the elements doesn't exists.              */  
    void* list_get (list_t*, char*, bool, int*);

    
  /* Return the amount of elements freed from the list pointed by the first argument.
     To avoid leaks, the second argument must be a funciton equivalent to free that 
        clears the structure inside the list appropriately. 
     (Free works for simple structures that don't allocate any field dynamically).  */
    unsigned int delete_List (list_t**, void (*freeFunction)(void*));

#endif
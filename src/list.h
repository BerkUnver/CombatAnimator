#ifndef LIST_H
#define LIST_H

typedef struct ListHeader {
    int count;
    int countAlloc;
    // Might want a custom allocator here in the future.
} ListHeader;

#define LIST(T) (T *)

#define LIST_NEW(T) ((T *) (malloc(sizeof(ListHeader)) + 1))

#define LIST_NEW_SIZED(list, T, count)\
do {\
    int countAllocated = count < 4 ? 4 : count;\
    ListHeader *header = malloc(sizeof(ListHeader) + sizeof(T) * countAllocated);\
    header->count = count;\
    header->countAllocated = countAllocated;\
} while (0)

#define LIST_HEADER(list) (((ListHeader *) (list)) - 1)
#define LIST_COUNT(list) LIST_HEADER(list)->count
#define LIST_FOR_EACH(list, i) for (int i = 0; i < LIST_HEADER(list)->count; i++)
#define LIST_FREE(list) free(LIST_HEADER(list))

// We evaluate the list just to make sure no goofy stuff happens when people pass in an expression.
#define LIST_ADD(list, item) \
{\
    auto list_eval = (list);\
    ListHeader *header = LIST_HEADER(list_eval);\
    header->count++;\
    if (header->count > header->countAllocated) {\
        if (header->countAllocated < 4) header->countAllocated = 4;\
        else header->countAllocated = ((float) header->countAllocated * 1.5f);\
        realloc(header, sizeof(ListHeader) * header->countAllocated);\
    }\
    list_eval[header->count - 1] = (item);\
}



#endif

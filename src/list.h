#ifndef LIST_H
#define LIST_H

#define LIST_ALLOC_MINIMUM 4

// List consists of a header followed immediately by the data in the list.
// The pointer to the list is the pointer to the start of the data. That way we can index it with normal [] syntax.
// The allocated memory starts at the pointer to the list minus the size of the list header.

typedef struct ListHeader {
    int count;
    int countAllocated;
    int itemSize;
    // Might want a custom allocator here in the future.
} ListHeader;

// Internal functions used by macros.
void *listNew(int itemSize, int count);
void *listClone(void *list);

#define LIST(T) T *
#define LIST_NEW(T) ((T *) listNew(sizeof(T), 0))
#define LIST_NEW_SIZED(T, count) ((T *) listNew(sizeof(T), count))

#define LIST_HEADER(list) (((ListHeader *) (list)) - 1)
#define LIST_COUNT(list) LIST_HEADER(list)->count
#define LIST_FOR_EACH(list, i) for (int i = 0; i < LIST_HEADER(list)->count; i++)
#define LIST_FREE(list) free(LIST_HEADER(list))
#define LIST_CLONE(T, list) ((T *) listClone(list))

// We evaluate the list just to make sure no goofy stuff happens when people pass in an expression.
#define LIST_ADD(listPtr, item) \
do {\
    ListHeader *header = LIST_HEADER(*(listPtr));\
    if (header->count == header->countAllocated) {\
        if (header->countAllocated < LIST_ALLOC_MINIMUM) header->countAllocated = LIST_ALLOC_MINIMUM;\
        else header->countAllocated = ((float) header->countAllocated * 1.5f);\
        header = (ListHeader *) realloc(header, sizeof(ListHeader) + sizeof(item) * header->countAllocated);\
        *(listPtr) = (void *) (header + 1);\
    }\
    (*(listPtr))[header->count] = (item);\
    header->count++;\
} while (0)

#define LIST_SHRINK(list, count) \
do {\
    LIST_HEADER(list)->count = (count);\
} while (0)

#define LIST_POP(list) \
do {\
    ListHeader *header = LIST_HEADER(list);\
    if (header->count > 0) header->count--;\
} while (0)
#endif

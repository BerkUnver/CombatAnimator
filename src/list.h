#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#define LIST_ALLOC_MINIMUM 4

// List consists of a header followed immediately by the data in the list.
// The pointer to the list is the pointer to the start of the data. That way we can index it with normal [] syntax.
// The allocated memory starts at the pointer to the list minus the size of the list header.

typedef struct ListHeader {
    int count;
    int countAllocated;
    int itemSize;
    int ___; // @todo: This is so asan won't complain about the members of the list being misaligned.
    // Might want a custom allocator here in the future.
} ListHeader;

#define LIST(T) T *
void *ListNew(int itemSize, int count);
#define LIST_NEW(T) ((T *) ListNew(sizeof(T), 0))
#define LIST_NEW_SIZED(T, count) ((T *) ListNew(sizeof(T), count))

#define LIST_HEADER(list) (((ListHeader *) (list)) - 1)
#define LIST_COUNT(list) LIST_HEADER(list)->count
#define LIST_FREE(list) free(LIST_HEADER(list))

void *ListClone(LIST(void) list);
#define LIST_CLONE(T, list) ((T *) ListClone(list))

// I'm pretty sure that 'item' won't be evaluated twice because of the sizeof operator.
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

#define LIST_SHRINK(list, countNew) LIST_HEADER(list)->count = (countNew);

#define LIST_POP(list) \
do {\
    ListHeader *header = LIST_HEADER(list);\
    if (header->count > 0) header->count--;\
} while (0)
#endif

void ListAddMany(LIST(void) * list, LIST(void) listEnd);
#define LIST_ADD_MANY(listPtr, listEnd) ListAddMany((LIST(void) *) listPtr, (LIST(void)) listEnd);



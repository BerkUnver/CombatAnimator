#ifndef LIST_H
#define LIST_H

#define LIST_ALLOC_MINIMUM 4

// List consists of a header followed immediately by the data in the list.
// The pointer to the list is the pointer to the start of the data. That way we can index it with normal [] syntax.
// The allocated memory starts at the pointer to the list minus the size of the list header.

typedef struct ListHeader {
    int count;
    int countAlloc;
    int itemSize;
    // Might want a custom allocator here in the future.
} ListHeader;

void *list_new(int itemSize, int count) {
    int countAllocated;
    if (count <= 0) {
        count = 0;
        countAllocated = 0;
    } else if (count < LIST_ALLOC_MINIMUM) {
        countAllocated = LIST_ALLOC_MINIMUM;
    } else {
        countAllocated = count;
    }
    ListHeader *header = malloc(sizeof(ListHeader) * count);
    *header = {
        .count = count,
        .countAllocated = countAllocated,
        .itemSize = itemSize;
    };
    return (void *) (header + 1);
}

void *list_clone(void *list) {
    ListHeader *header = ((ListHeader *) list) - 1;
    int size = sizeof(ListHeader) + header->count * header->itemSize;
    ListHeader *headerNew = malloc(size);
    memcpy(headerNew, header, size);
    return (void *) (header + 1);
}

#define LIST(T) T *
#define LIST_NEW(T) ((T *) list_new(sizeof(T), 0))
#define LIST_NEW_SIZED(T, count) ((T *) list_new(sizeof(T), count))

#define LIST_HEADER(list) (((ListHeader *) (list)) - 1)
#define LIST_COUNT(list) LIST_HEADER(list)->count
#define LIST_FOR_EACH(list, i) for (int i = 0; i < LIST_HEADER(list)->count; i++)
#define LIST_FREE(list) free(LIST_HEADER(list))
#define LIST_CLONE(T, list) ((T *) list_clone(list))
// We evaluate the list just to make sure no goofy stuff happens when people pass in an expression.
#define LIST_ADD(list, item) \
{\
    auto list_eval = (list);\
    ListHeader *header = LIST_HEADER(list_eval);\
    header->count++;\
    if (header->count > header->countAllocated) {\
        if (header->countAllocated < LIST_ALLOC_MINIMUM) header->countAllocated = LIST_ALLOC_MINIMUM;\
        else header->countAllocated = ((float) header->countAllocated * 1.5f);\
        realloc(header, sizeof(ListHeader) * header->countAllocated);\
    }\
    list_eval[header->count - 1] = (item);\
}

#endif

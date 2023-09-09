#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

void *listNew(int itemSize, int count) {
    int countAllocated;
    if (count <= 0) {
        count = 0;
        countAllocated = 0;
    } else if (count < LIST_ALLOC_MINIMUM) {
        countAllocated = LIST_ALLOC_MINIMUM;
    } else {
        countAllocated = count;
    }
    ListHeader *header = malloc(sizeof(ListHeader) + itemSize * countAllocated);
    *header = (ListHeader) {
        .count = count,
        .countAllocated = countAllocated,
        .itemSize = itemSize
    };
    return (void *) (header + 1);
}

void *listClone(void *list) {
    ListHeader *header = ((ListHeader *) list) - 1;
    int size = sizeof(ListHeader) + header->count * header->itemSize;
    ListHeader *headerNew = malloc(size);
    memcpy(headerNew, header, size);
    return (void *) (headerNew + 1);
}



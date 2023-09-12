#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

void *ListNew(int itemSize, int count) {
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

void *ListClone(LIST(void) list) {
    ListHeader *header = LIST_HEADER(list);
    int size = sizeof(ListHeader) + header->count * header->itemSize;
    ListHeader *headerNew = malloc(size);
    memcpy(headerNew, header, size);
    return (void *) (headerNew + 1);
}

void ListAddMany(LIST(void) *list, LIST(void) listEnd) {
    ListHeader *header = LIST_HEADER(*list);
    ListHeader *headerEnd = LIST_HEADER(listEnd);
    assert(header->itemSize == headerEnd->itemSize);
    
    int headerCountNew = header->count + headerEnd->count;
    if (headerCountNew > header->countAllocated) {
        if (headerCountNew <= 0) {
            header->countAllocated = 0;
        } else if (headerCountNew < LIST_ALLOC_MINIMUM) {
            header->countAllocated = LIST_ALLOC_MINIMUM;
        } else if (headerCountNew < ((float) header->countAllocated) * 1.5f) {
            header->countAllocated = ((float) header->countAllocated) * 1.5f;
        } else {
            header->countAllocated = header->count;
        }

        header = realloc(header, header->itemSize * header->countAllocated);
        *list = (void *) (header + 1);
    }
    
    memcpy(*list + header->itemSize * header->count, listEnd, headerEnd->count * headerEnd->itemSize);
    header->count = headerCountNew;
}

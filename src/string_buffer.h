#include <stdbool.h>

#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#define EXPAND_AMOUNT 1.5f
#define INITIAL_MAXIMUM_LENGTH 11

typedef struct StringBuffer {
    int mallocSize;
    int length;
    char *raw;
} StringBuffer;

StringBuffer StringBufferNew();
void StringBufferAddChar(StringBuffer *string, char chr);
void StringBufferAddString(StringBuffer *string, const char *end);
bool StringBufferRemoveChar(StringBuffer *string);
void StringBufferClear(StringBuffer *string);
void StringBufferFree(StringBuffer *string);

#endif

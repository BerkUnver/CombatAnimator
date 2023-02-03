#include <stdbool.h>

#ifndef EXPANDABLE_STRING
#define EXPANDABLE_STRING

#define EXPAND_AMOUNT 1.5f
#define INITIAL_MAXIMUM_LENGTH 11

typedef struct StringBuffer {
    int mallocSize;
    int length;
    char *raw;
} StringBuffer;

StringBuffer EmptyStringBuffer();
void AppendChar(StringBuffer *string, char chr);
void AppendString(StringBuffer *string, const char *end);
bool RemoveChar(StringBuffer *string);
void FreeStringBuffer(StringBuffer *string);

#endif

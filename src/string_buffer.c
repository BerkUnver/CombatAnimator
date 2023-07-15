#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "string_buffer.h"

StringBuffer StringBufferNew() {
    char *str = malloc(sizeof(char) * INITIAL_MAXIMUM_LENGTH);
    str[0] = '\0';
    return (StringBuffer) {INITIAL_MAXIMUM_LENGTH, 0, str};
}

void StringBufferAddChar(StringBuffer *string, char chr) {
    int requiredSize = string->length + 2;
    if (requiredSize > string->mallocSize) { // if the new char and the null ending char plus the length is greater than 0 then reallocate.
        int newMaxSize = (int) ceilf((float) sizeof(char) * (float) requiredSize * EXPAND_AMOUNT);
        char *newRaw = malloc(sizeof(char) * newMaxSize);
        memcpy(newRaw, string->raw, sizeof(char) * string->length);
        free(string->raw);
        string->raw = newRaw;
        string->mallocSize = newMaxSize;
    }
    string->raw[string->length] = chr;
    string->length++;
    string->raw[string->length] = '\0';
}

void StringBufferAddString(StringBuffer *string, const char *end){
    char c = end[0];
    int idx = 0;
    while (c != '\0') {
        StringBufferAddChar(string, c);
        idx++;
        c = end[idx];
    }
}

bool StringBufferRemoveChar(StringBuffer *string) {
    if (string->length <= 0) return false;
    string->length--;
    string->raw[string->length] = '\0';
    return true;
}

void StringBufferClear(StringBuffer *string) {
    string->raw[0] = '\0';
    string->length = 0;
}

void StringBufferFree(StringBuffer *string) {
    free(string->raw);
}
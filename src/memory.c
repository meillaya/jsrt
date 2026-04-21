#include "jsrt/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *jsrt_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return ptr;
}

void *jsrt_realloc(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    if (result == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return result;
}

char *jsrt_strdup(const char *value) {
    size_t length = strlen(value);
    char *copy = jsrt_malloc(length + 1U);
    memcpy(copy, value, length + 1U);
    return copy;
}

char *jsrt_vformat(const char *format, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (needed < 0) {
        fprintf(stderr, "Formatting failed\n");
        exit(1);
    }

    char *buffer = jsrt_malloc((size_t)needed + 1U);
    vsnprintf(buffer, (size_t)needed + 1U, format, args);
    return buffer;
}

char *jsrt_format(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *buffer = jsrt_vformat(format, args);
    va_end(args);
    return buffer;
}

#ifndef JSRT_MEMORY_H
#define JSRT_MEMORY_H

#include <stddef.h>
#include <stdarg.h>

void *jsrt_malloc(size_t size);
void *jsrt_realloc(void *ptr, size_t size);
char *jsrt_strdup(const char *value);
char *jsrt_vformat(const char *format, va_list args);
char *jsrt_format(const char *format, ...);

#endif

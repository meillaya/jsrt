#include "jsrt/diagnostics.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/memory.h"

static const char *find_line_start(const char *source, int target_line) {
    const char *cursor = source;
    int current_line = 1;

    while (*cursor != '\0' && current_line < target_line) {
        if (*cursor == '\n') {
            current_line += 1;
        }
        cursor += 1;
    }
    return cursor;
}

static const char *find_line_end(const char *line_start) {
    const char *cursor = line_start;
    while (*cursor != '\0' && *cursor != '\n') {
        cursor += 1;
    }
    return cursor;
}

char *format_source_diagnostic(
    const char *kind,
    const char *source,
    int line,
    int column,
    int width,
    const char *message
) {
    int caret_width = width > 0 ? width : 1;
    size_t i;

    if (source == NULL) {
        return jsrt_format("%s at %d:%d: %s", kind, line, column, message);
    }

    const char *line_start = find_line_start(source, line);
    const char *line_end = find_line_end(line_start);
    size_t line_length = (size_t)(line_end - line_start);
    char *line_text = jsrt_malloc(line_length + 1U);
    char *caret = jsrt_malloc((size_t)caret_width + 1U);
    char *result;

    memcpy(line_text, line_start, line_length);
    line_text[line_length] = '\0';
    for (i = 0; i < (size_t)caret_width; ++i) {
        caret[i] = '^';
    }
    caret[caret_width] = '\0';

    result = jsrt_format(
        "%s at %d:%d: %s\n%s\n%*s%s",
        kind,
        line,
        column,
        message,
        line_text,
        column > 1 ? column - 1 : 0,
        "",
        caret
    );

    free(line_text);
    free(caret);
    return result;
}

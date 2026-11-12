#include "jsrt/diagnostics.h"

#include <stdbool.h>
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

/* A malformed file can hold a single line of megabytes. Printing all of it
 * for every diagnostic turns a small input into a large one, so the echo of
 * the source line is capped. */
#define JSRT_MAX_DIAGNOSTIC_LINE 160U

char *format_source_diagnostic(
    const char *kind,
    const char *source,
    int line,
    int column,
    int width,
    const char *message
) {
    int display_column;
    int caret_width = width > 0 ? width : 1;
    size_t i;

    if (source == NULL) {
        return jsrt_format("%s at %d:%d: %s", kind, line, column, message);
    }

    {
        const char *line_start = find_line_start(source, line);
        const char *line_end = find_line_end(line_start);
        size_t line_length = (size_t)(line_end - line_start);
        size_t display_length = line_length;
        bool truncated = false;
        char *line_text;
        char *caret;
        char *result;

        if (display_length > JSRT_MAX_DIAGNOSTIC_LINE) {
            display_length = JSRT_MAX_DIAGNOSTIC_LINE;
            truncated = true;
        }
        line_text = jsrt_malloc(display_length + 4U);
        memcpy(line_text, line_start, display_length);
        if (truncated) {
            memcpy(line_text + display_length, "...", 3U);
            display_length += 3U;
        }
        line_text[display_length] = '\0';

        display_column = column > 0 ? column : 1;
        if ((size_t)(display_column - 1) > JSRT_MAX_DIAGNOSTIC_LINE) {
            display_column = (int)JSRT_MAX_DIAGNOSTIC_LINE + 1;
        }
        if ((size_t)(display_column - 1 + caret_width) > display_length) {
            int remaining = (int)display_length - (display_column - 1);
            caret_width = remaining > 0 ? remaining : 1;
        }

        caret = jsrt_malloc((size_t)caret_width + 1U);
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
            display_column > 1 ? display_column - 1 : 0,
            "",
            caret
        );

        free(line_text);
        free(caret);
        return result;
    }
}

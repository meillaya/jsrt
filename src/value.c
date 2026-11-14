#include "jsrt/value.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/ast.h"
#include "jsrt/bytecode.h"
#include "jsrt/env.h"
#include "jsrt/gc.h"
#include "jsrt/memory.h"

static void array_mark_reachable(JsArray *array);
static void object_mark_reachable(JsObject *object);
typedef struct ValueStringifyFrame {
    const void *aggregate;
    const struct ValueStringifyFrame *previous;
} ValueStringifyFrame;
typedef struct ValueEqualityFrame {
    const void *left;
    const void *right;
    const struct ValueEqualityFrame *previous;
} ValueEqualityFrame;
/* Both structural operations recurse over aggregates. The depth limit keeps a
 * hostile shape from overflowing the C stack, and the trace keeps shared
 * subgraphs from being compared again. */
#define JSRT_MAX_VALUE_DEPTH 512
#define JSRT_MAX_STRINGIFY_BYTES (1U << 20)

typedef struct ValueEqualityTrace {
    const void *left;
    const void *right;
    struct ValueEqualityTrace *next;
} ValueEqualityTrace;

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool truncated;
} StringifyBuffer;

static void stringify_value(Value value, const ValueStringifyFrame *frames, int depth, StringifyBuffer *buffer);
static bool value_equals_internal(Value left, Value right, ValueEqualityTrace **trace, int depth);
static bool stringify_frame_contains(const ValueStringifyFrame *frames, const void *aggregate);

Value value_number(double number) {
    Value value;
    value.type = VALUE_NUMBER;
    value.as.number = number;
    return value;
}

Value value_bool(bool boolean) {
    Value value;
    value.type = VALUE_BOOL;
    value.as.boolean = boolean;
    return value;
}

Value value_null(void) {
    Value value;
    value.type = VALUE_NULL;
    value.as.number = 0.0;
    return value;
}

Value value_string(const char *string) {
    Value value;
    value.type = VALUE_STRING;
    value.as.string = jsrt_strdup(string);
    return value;
}

Value value_string_take(char *string) {
    Value value;
    value.type = VALUE_STRING;
    value.as.string = string;
    return value;
}

Value value_function(JsFunction *function) {
    Value value;
    value.type = VALUE_FUNCTION;
    value.as.function = function;
    return value;
}

Value value_array_take(JsArray *array) {
    Value value;
    value.type = VALUE_ARRAY;
    value.as.array = array;
    return value;
}

Value value_object_take(JsObject *object) {
    Value value;
    value.type = VALUE_OBJECT;
    value.as.object = object;
    return value;
}

Value value_clone(Value value) {
    switch (value.type) {
        case VALUE_NUMBER:
            return value_number(value.as.number);
        case VALUE_BOOL:
            return value_bool(value.as.boolean);
        case VALUE_NULL:
            return value_null();
        case VALUE_STRING:
            return value_string(value.as.string);
        case VALUE_FUNCTION:
        case VALUE_ARRAY:
        case VALUE_OBJECT:
            return value;
    }
    return value_null();
}

bool value_is_truthy(Value value) {
    switch (value.type) {
        case VALUE_BOOL:
            return value.as.boolean;
        case VALUE_NULL:
            return false;
        case VALUE_NUMBER:
            return value.as.number != 0.0;
        case VALUE_STRING:
            return value.as.string[0] != '\0';
        case VALUE_FUNCTION:
        case VALUE_ARRAY:
        case VALUE_OBJECT:
            return true;
    }
    return false;
}

static bool stringify_frame_contains(const ValueStringifyFrame *frames, const void *aggregate) {
    while (frames != NULL) {
        if (frames->aggregate == aggregate) {
            return true;
        }
        frames = frames->previous;
    }
    return false;
}

static bool equality_trace_contains(const ValueEqualityTrace *trace, const void *left, const void *right) {
    while (trace != NULL) {
        if (trace->left == left && trace->right == right) {
            return true;
        }
        trace = trace->next;
    }
    return false;
}

static void equality_trace_free(ValueEqualityTrace *trace) {
    while (trace != NULL) {
        ValueEqualityTrace *next = trace->next;
        free(trace);
        trace = next;
    }
}

static bool array_equals_internal(const JsArray *left, const JsArray *right, ValueEqualityTrace **trace, int depth) {
    size_t i;

    if (depth > JSRT_MAX_VALUE_DEPTH) {
        return false;
    }
    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL) {
        return left == right;
    }
    if (left->count != right->count) {
        return false;
    }
    if (equality_trace_contains(*trace, left, right)) {
        return true;
    }

    {
        ValueEqualityTrace *entry = jsrt_malloc(sizeof(*entry));
        entry->left = left;
        entry->right = right;
        entry->next = *trace;
        *trace = entry;
    }
    for (i = 0; i < left->count; ++i) {
        if (!value_equals_internal(left->items[i], right->items[i], trace, depth + 1)) {
            return false;
        }
    }
    return true;
}

static bool object_equals_internal(const JsObject *left, const JsObject *right, ValueEqualityTrace **trace, int depth) {
    size_t i;

    if (depth > JSRT_MAX_VALUE_DEPTH) {
        return false;
    }
    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL) {
        return left == right;
    }
    if (left->count != right->count) {
        return false;
    }
    if (equality_trace_contains(*trace, left, right)) {
        return true;
    }

    {
        ValueEqualityTrace *entry = jsrt_malloc(sizeof(*entry));
        entry->left = left;
        entry->right = right;
        entry->next = *trace;
        *trace = entry;
    }
    for (i = 0; i < left->count; ++i) {
        if (strcmp(left->items[i].key, right->items[i].key) != 0) {
            return false;
        }
        if (!value_equals_internal(left->items[i].value, right->items[i].value, trace, depth + 1)) {
            return false;
        }
    }
    return true;
}

static bool value_equals_internal(Value left, Value right, ValueEqualityTrace **trace, int depth) {
    if (left.type != right.type) {
        return false;
    }

    switch (left.type) {
        case VALUE_NUMBER:
            return left.as.number == right.as.number;
        case VALUE_BOOL:
            return left.as.boolean == right.as.boolean;
        case VALUE_NULL:
            return true;
        case VALUE_STRING:
            return strcmp(left.as.string, right.as.string) == 0;
        case VALUE_FUNCTION:
            return left.as.function == right.as.function;
        case VALUE_ARRAY:
            return array_equals_internal(left.as.array, right.as.array, trace, depth);
        case VALUE_OBJECT:
            return object_equals_internal(left.as.object, right.as.object, trace, depth);
    }
    return false;
}

bool value_equals(Value left, Value right) {
    ValueEqualityTrace *trace = NULL;
    bool equal = value_equals_internal(left, right, &trace, 0);

    equality_trace_free(trace);
    return equal;
}

static void stringify_buffer_init(StringifyBuffer *buffer) {
    buffer->capacity = 64U;
    buffer->data = jsrt_malloc(buffer->capacity);
    buffer->data[0] = '\0';
    buffer->length = 0U;
    buffer->truncated = false;
}

static void stringify_append(StringifyBuffer *buffer, const char *text, size_t length) {
    size_t needed;

    if (buffer->truncated) {
        return;
    }
    if (buffer->length + length > JSRT_MAX_STRINGIFY_BYTES) {
        buffer->truncated = true;
        return;
    }
    needed = buffer->length + length + 1U;
    if (needed > buffer->capacity) {
        while (needed > buffer->capacity) {
            buffer->capacity *= 2U;
        }
        buffer->data = jsrt_realloc(buffer->data, buffer->capacity);
    }
    memcpy(buffer->data + buffer->length, text, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
}

static void stringify_append_text(StringifyBuffer *buffer, const char *text) {
    stringify_append(buffer, text, strlen(text));
}

static char *stringify_finish(StringifyBuffer *buffer) {
    if (buffer->truncated) {
        free(buffer->data);
        return jsrt_strdup("<output limit>");
    }
    return buffer->data;
}

static void stringify_array(const JsArray *array, const ValueStringifyFrame *frames, int depth, StringifyBuffer *buffer) {
    size_t i;
    ValueStringifyFrame frame = {array, frames};

    stringify_append_text(buffer, "[");
    for (i = 0; i < array->count; ++i) {
        if (buffer->truncated) {
            return;
        }
        if (i > 0) {
            stringify_append_text(buffer, ", ");
        }
        stringify_value(array->items[i], &frame, depth + 1, buffer);
    }
    stringify_append_text(buffer, "]");
}

static void stringify_object(const JsObject *object, const ValueStringifyFrame *frames, int depth, StringifyBuffer *buffer) {
    size_t i;
    ValueStringifyFrame frame = {object, frames};

    stringify_append_text(buffer, "{");
    for (i = 0; i < object->count; ++i) {
        if (buffer->truncated) {
            return;
        }
        if (i > 0) {
            stringify_append_text(buffer, ", ");
        }
        stringify_append_text(buffer, object->items[i].key);
        stringify_append_text(buffer, ": ");
        stringify_value(object->items[i].value, &frame, depth + 1, buffer);
    }
    stringify_append_text(buffer, "}");
}

static void stringify_function(const JsFunction *function, StringifyBuffer *buffer) {
    const char *name = NULL;

    if (function != NULL && function->is_native && function->native_name != NULL) {
        stringify_append_text(buffer, "<builtin ");
        stringify_append_text(buffer, function->native_name);
        stringify_append_text(buffer, ">");
        return;
    }
    if (function != NULL && function->bytecode != NULL) {
        name = function->bytecode->name;
    } else if (function != NULL && function->declaration != NULL && function->declaration->kind == STMT_FUNCTION) {
        name = function->declaration->as.function.name;
    } else if (function != NULL && function->expression != NULL) {
        name = function->expression->as.function.name;
    } else if (function != NULL) {
        name = function->name;
    }
    if (name != NULL) {
        stringify_append_text(buffer, "<function ");
        stringify_append_text(buffer, name);
        stringify_append_text(buffer, ">");
    } else {
        stringify_append_text(buffer, "<function>");
    }
}

static void stringify_value(Value value, const ValueStringifyFrame *frames, int depth, StringifyBuffer *buffer) {
    if (buffer->truncated) {
        return;
    }
    if (depth > JSRT_MAX_VALUE_DEPTH) {
        stringify_append_text(buffer, "<depth limit>");
        return;
    }

    switch (value.type) {
        case VALUE_NUMBER: {
            char *text = jsrt_format("%.15g", value.as.number);
            stringify_append_text(buffer, text);
            free(text);
            break;
        }
        case VALUE_BOOL:
            stringify_append_text(buffer, value.as.boolean ? "true" : "false");
            break;
        case VALUE_NULL:
            stringify_append_text(buffer, "null");
            break;
        case VALUE_STRING:
            stringify_append_text(buffer, value.as.string);
            break;
        case VALUE_FUNCTION:
            stringify_function(value.as.function, buffer);
            break;
        case VALUE_ARRAY:
            if (stringify_frame_contains(frames, value.as.array)) {
                stringify_append_text(buffer, "<cycle>");
            } else if (value.as.array != NULL) {
                stringify_array(value.as.array, frames, depth, buffer);
            }
            break;
        case VALUE_OBJECT:
            if (stringify_frame_contains(frames, value.as.object)) {
                stringify_append_text(buffer, "<cycle>");
            } else if (value.as.object != NULL) {
                stringify_object(value.as.object, frames, depth, buffer);
            }
            break;
    }
}

char *value_to_string(Value value) {
    StringifyBuffer buffer;

    stringify_buffer_init(&buffer);
    stringify_value(value, NULL, 0, &buffer);
    return stringify_finish(&buffer);
}

void value_retain(Value value) {
    (void)value;
}

static void array_mark_reachable(JsArray *array) {
    size_t i;

    if (array == NULL || array->gc.marked) {
        return;
    }

    array->gc.marked = true;
    for (i = 0; i < array->count; ++i) {
        value_mark_reachable(array->items[i]);
    }
}

static void object_mark_reachable(JsObject *object) {
    size_t i;

    if (object == NULL || object->gc.marked) {
        return;
    }

    object->gc.marked = true;
    for (i = 0; i < object->count; ++i) {
        value_mark_reachable(object->items[i].value);
    }
    if (object->prototype != NULL) {
        object_mark_reachable(object->prototype);
    }
}

void value_mark_reachable(Value value) {
    switch (value.type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
        case VALUE_NULL:
        case VALUE_STRING:
            return;
        case VALUE_FUNCTION:
            if (value.as.function == NULL || value.as.function->gc.marked) {
                return;
            }
            value.as.function->gc.marked = true;
            value_mark_reachable(value.as.function->prototype);
            {
                size_t i;
                for (i = 0; i < value.as.function->capture_count; ++i) {
                    value_mark_reachable(value.as.function->captures[i]);
                }
            }
            env_mark_reachable(value.as.function->closure);
            return;
        case VALUE_ARRAY:
            array_mark_reachable(value.as.array);
            return;
        case VALUE_OBJECT:
            object_mark_reachable(value.as.object);
            return;
    }
}

void value_release_reference(Value value) {
    if (value.type == VALUE_STRING) {
        free(value.as.string);
    }
}

void value_free(Value value) {
    if (value.type == VALUE_STRING) {
        free(value.as.string);
    }
}

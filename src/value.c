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
static char *value_to_string_internal(Value value, const ValueStringifyFrame *frames);
static bool value_equals_internal(Value left, Value right, const ValueEqualityFrame *frames);
static bool stringify_frame_contains(const ValueStringifyFrame *frames, const void *aggregate);
static bool equality_frame_contains(const ValueEqualityFrame *frames, const void *left, const void *right);

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
            return fabs(value.as.number) > 0.000000001;
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

static bool equality_frame_contains(const ValueEqualityFrame *frames, const void *left, const void *right) {
    while (frames != NULL) {
        if (frames->left == left && frames->right == right) {
            return true;
        }
        frames = frames->previous;
    }
    return false;
}

static bool array_equals_internal(const JsArray *left, const JsArray *right, const ValueEqualityFrame *frames) {
    size_t i;

    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL) {
        return left == right;
    }
    if (left->count != right->count) {
        return false;
    }
    if (equality_frame_contains(frames, left, right)) {
        return true;
    }

    {
        ValueEqualityFrame frame = {left, right, frames};
        for (i = 0; i < left->count; ++i) {
            if (!value_equals_internal(left->items[i], right->items[i], &frame)) {
                return false;
            }
        }
    }
    return true;
}

static bool object_equals_internal(const JsObject *left, const JsObject *right, const ValueEqualityFrame *frames) {
    size_t i;

    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL) {
        return left == right;
    }
    if (left->count != right->count) {
        return false;
    }
    if (equality_frame_contains(frames, left, right)) {
        return true;
    }
    {
        ValueEqualityFrame frame = {left, right, frames};
        for (i = 0; i < left->count; ++i) {
            if (strcmp(left->items[i].key, right->items[i].key) != 0) {
                return false;
            }
            if (!value_equals_internal(left->items[i].value, right->items[i].value, &frame)) {
                return false;
            }
        }
    }
    return true;
}

static bool value_equals_internal(Value left, Value right, const ValueEqualityFrame *frames) {
    if (left.type != right.type) {
        return false;
    }

    switch (left.type) {
        case VALUE_NUMBER:
            return fabs(left.as.number - right.as.number) < 0.000000001;
        case VALUE_BOOL:
            return left.as.boolean == right.as.boolean;
        case VALUE_NULL:
            return true;
        case VALUE_STRING:
            return strcmp(left.as.string, right.as.string) == 0;
        case VALUE_FUNCTION:
            return left.as.function == right.as.function;
        case VALUE_ARRAY:
            return array_equals_internal(left.as.array, right.as.array, frames);
        case VALUE_OBJECT:
            return object_equals_internal(left.as.object, right.as.object, frames);
    }
    return false;
}

bool value_equals(Value left, Value right) {
    return value_equals_internal(left, right, NULL);
}

static char *join_strings(char *prefix, const char *suffix) {
    char *joined = jsrt_format("%s%s", prefix, suffix);
    free(prefix);
    return joined;
}

static char *array_to_string(const JsArray *array, const ValueStringifyFrame *frames) {
    size_t i;
    char *result = jsrt_strdup("[");
    ValueStringifyFrame frame = {array, frames};
    for (i = 0; i < array->count; ++i) {
        char *item = value_to_string_internal(array->items[i], &frame);
        if (i > 0) {
            result = join_strings(result, ", ");
        }
        result = join_strings(result, item);
        free(item);
    }
    result = join_strings(result, "]");
    return result;
}

static char *object_to_string(const JsObject *object, const ValueStringifyFrame *frames) {
    size_t i;
    char *result = jsrt_strdup("{");
    ValueStringifyFrame frame = {object, frames};
    for (i = 0; i < object->count; ++i) {
        char *value_text = value_to_string_internal(object->items[i].value, &frame);
        char *pair = jsrt_format("%s: %s", object->items[i].key, value_text);
        if (i > 0) {
            result = join_strings(result, ", ");
        }
        result = join_strings(result, pair);
        free(value_text);
        free(pair);
    }
    result = join_strings(result, "}");
    return result;
}

static char *value_to_string_internal(Value value, const ValueStringifyFrame *frames) {
    switch (value.type) {
        case VALUE_NUMBER:
            return jsrt_format("%.15g", value.as.number);
        case VALUE_BOOL:
            return jsrt_strdup(value.as.boolean ? "true" : "false");
        case VALUE_NULL:
            return jsrt_strdup("null");
        case VALUE_STRING:
            return jsrt_strdup(value.as.string);
        case VALUE_FUNCTION:
            if (value.as.function != NULL && value.as.function->is_native && value.as.function->native_name != NULL) {
                return jsrt_format("<builtin %s>", value.as.function->native_name);
            }
            if (value.as.function != NULL && value.as.function->bytecode != NULL &&
                value.as.function->bytecode->name != NULL) {
                return jsrt_format("<function %s>", value.as.function->bytecode->name);
            }
            if (value.as.function != NULL && value.as.function->declaration != NULL &&
                value.as.function->declaration->kind == STMT_FUNCTION) {
                return jsrt_format("<function %s>", value.as.function->declaration->as.function.name);
            }
            if (value.as.function != NULL && value.as.function->expression != NULL &&
                value.as.function->expression->as.function.name != NULL) {
                return jsrt_format("<function %s>", value.as.function->expression->as.function.name);
            }
            return jsrt_strdup("<function>");
        case VALUE_ARRAY:
            if (stringify_frame_contains(frames, value.as.array)) {
                return jsrt_strdup("<cycle>");
            }
            return array_to_string(value.as.array, frames);
        case VALUE_OBJECT:
            if (stringify_frame_contains(frames, value.as.object)) {
                return jsrt_strdup("<cycle>");
            }
            return object_to_string(value.as.object, frames);
    }
    return jsrt_strdup("<unknown>");
}

char *value_to_string(Value value) {
    return value_to_string_internal(value, NULL);
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
            if (value.as.function->prototype_object != NULL) {
                object_mark_reachable(value.as.function->prototype_object);
            }
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

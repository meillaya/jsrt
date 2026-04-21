#include "jsrt/value.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/gc.h"
#include "jsrt/memory.h"

static JsArray *array_clone(const JsArray *array);
static JsObject *object_clone(const JsObject *object);
static void array_mark_reachable(JsArray *array);
static void object_mark_reachable(JsObject *object);

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
            return value;
        case VALUE_ARRAY:
            return value_array_take(array_clone(value.as.array));
        case VALUE_OBJECT:
            return value_object_take(object_clone(value.as.object));
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

static bool array_equals(const JsArray *left, const JsArray *right) {
    size_t i;
    if (left->count != right->count) {
        return false;
    }
    for (i = 0; i < left->count; ++i) {
        if (!value_equals(left->items[i], right->items[i])) {
            return false;
        }
    }
    return true;
}

static bool object_equals(const JsObject *left, const JsObject *right) {
    size_t i;
    if (left->count != right->count) {
        return false;
    }
    for (i = 0; i < left->count; ++i) {
        if (strcmp(left->items[i].key, right->items[i].key) != 0) {
            return false;
        }
        if (!value_equals(left->items[i].value, right->items[i].value)) {
            return false;
        }
    }
    return true;
}

bool value_equals(Value left, Value right) {
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
            return array_equals(left.as.array, right.as.array);
        case VALUE_OBJECT:
            return object_equals(left.as.object, right.as.object);
    }
    return false;
}

static char *join_strings(char *prefix, const char *suffix) {
    char *joined = jsrt_format("%s%s", prefix, suffix);
    free(prefix);
    return joined;
}

static char *array_to_string(const JsArray *array) {
    size_t i;
    char *result = jsrt_strdup("[");
    for (i = 0; i < array->count; ++i) {
        char *item = value_to_string(array->items[i]);
        if (i > 0) {
            result = join_strings(result, ", ");
        }
        result = join_strings(result, item);
        free(item);
    }
    result = join_strings(result, "]");
    return result;
}

static char *object_to_string(const JsObject *object) {
    size_t i;
    char *result = jsrt_strdup("{");
    for (i = 0; i < object->count; ++i) {
        char *value_text = value_to_string(object->items[i].value);
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

char *value_to_string(Value value) {
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
            return array_to_string(value.as.array);
        case VALUE_OBJECT:
            return object_to_string(value.as.object);
    }
    return jsrt_strdup("<unknown>");
}

void value_retain(Value value) {
    (void)value;
}

static JsArray *array_clone(const JsArray *array) {
    size_t i;
    JsArray *clone = jsrt_malloc(sizeof(*clone));
    gc_register_object(&clone->gc, GC_OBJECT_ARRAY);
    clone->count = array->count;
    clone->items = jsrt_malloc(sizeof(*clone->items) * array->count);
    for (i = 0; i < array->count; ++i) {
        clone->items[i] = value_clone(array->items[i]);
    }
    return clone;
}

static JsObject *object_clone(const JsObject *object) {
    size_t i;
    JsObject *clone = jsrt_malloc(sizeof(*clone));
    gc_register_object(&clone->gc, GC_OBJECT_OBJECT);
    clone->count = object->count;
    clone->items = jsrt_malloc(sizeof(*clone->items) * object->count);
    for (i = 0; i < object->count; ++i) {
        clone->items[i].key = jsrt_strdup(object->items[i].key);
        clone->items[i].value = value_clone(object->items[i].value);
    }
    return clone;
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

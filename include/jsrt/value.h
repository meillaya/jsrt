#ifndef JSRT_VALUE_H
#define JSRT_VALUE_H

#include <stdbool.h>
#include <stddef.h>

#include "jsrt/ast.h"
#include "jsrt/gc.h"

struct BytecodeFunction;
typedef struct Env Env;
typedef struct JsArray JsArray;
typedef struct JsObjectEntry JsObjectEntry;
typedef struct JsObject JsObject;
typedef struct JsFunction JsFunction;
typedef struct Value Value;

typedef enum {
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_NULL,
    VALUE_STRING,
    VALUE_FUNCTION,
    VALUE_ARRAY,
    VALUE_OBJECT
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
        char *string;
        JsFunction *function;
        JsArray *array;
        JsObject *object;
    } as;
} Value;

struct JsFunction {
    GcHeader gc;
    bool is_native;
    bool is_class_constructor;
    const struct Stmt *declaration;
    const struct Expr *expression;
    const char *name;
    const StringArray *params;
    const StmtArray *body;
    Env *closure;
    const char *native_name;
    size_t native_arity;
    const struct BytecodeFunction *bytecode;
    Value prototype;
    Value *captures;
    size_t capture_count;
};

struct JsArray {
    GcHeader gc;
    Value *items;
    size_t count;
};

struct JsObjectEntry {
    char *key;
    Value value;
};

struct JsObject {
    GcHeader gc;
    JsObjectEntry *items;
    size_t count;
    JsObject *prototype;
};

Value value_number(double number);
Value value_bool(bool boolean);
Value value_null(void);
Value value_string(const char *string);
Value value_string_take(char *string);
Value value_function(JsFunction *function);
Value value_array_take(JsArray *array);
Value value_object_take(JsObject *object);
Value value_clone(Value value);

bool value_is_truthy(Value value);
bool value_equals(Value left, Value right);
char *value_to_string(Value value);
void value_retain(Value value);
void value_free(Value value);
void value_mark_reachable(Value value);
void value_release_reference(Value value);

#endif

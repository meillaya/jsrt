#include "jsrt/eval.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/diagnostics.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/memory.h"
#include "jsrt/parser.h"

typedef struct ActiveEnvScope {
    Env *env;
    struct ActiveEnvScope *previous;
} ActiveEnvScope;

typedef struct ModuleRecord {
    char *path;
    char *source;
    TokenArray tokens;
    StmtArray program;
    Env *env;
    StringArray exports;
    bool lexed;
    bool parsed;
    bool loading;
    struct ModuleRecord *next;
} ModuleRecord;

typedef struct {
    Value *items;
    size_t count;
    size_t capacity;
} JobQueue;

typedef struct {
    const char *source;
    const char *module_path;
    Env *env;
    Env *root_env;
    StringArray *exports;
    JobQueue *jobs;
    ActiveEnvScope *active_envs;
    int loop_depth;
    int switch_depth;
} Evaluator;

static ModuleRecord *module_records = NULL;
static int eval_execution_depth = 0;

static void job_queue_init(JobQueue *queue) {
    queue->items = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

static void job_queue_push(JobQueue *queue, Value callback) {
    if (queue->count == queue->capacity) {
        size_t next_capacity = queue->capacity == 0 ? 4U : queue->capacity * 2U;
        queue->items = jsrt_realloc(queue->items, next_capacity * sizeof(*queue->items));
        queue->capacity = next_capacity;
    }
    queue->items[queue->count++] = value_clone(callback);
}

static Value job_queue_pop_front(JobQueue *queue) {
    Value value = queue->items[0];
    size_t i;

    for (i = 1; i < queue->count; ++i) {
        queue->items[i - 1U] = queue->items[i];
    }
    queue->count -= 1U;
    return value;
}

static void job_queue_free(JobQueue *queue) {
    size_t i;
    for (i = 0; i < queue->count; ++i) {
        value_free(queue->items[i]);
    }
    free(queue->items);
    queue->items = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

typedef enum {
    ASSIGN_STEP_PROPERTY,
    ASSIGN_STEP_INDEX
} AssignStepKind;

typedef struct {
    AssignStepKind kind;
    const char *property_name;
    const Expr *index_expr;
    int line;
    int column;
} AssignStep;

typedef struct {
    const char *root_name;
    bool root_is_this;
    AssignStep *items;
    size_t count;
    size_t capacity;
} AssignPath;

typedef enum {
    BUILTIN_PRINT,
    BUILTIN_LEN,
    BUILTIN_TYPEOF,
    BUILTIN_SET_PROTOTYPE_OF,
    BUILTIN_DEFER,
    BUILTIN_RUN_JOBS,
    BUILTIN_STR_CONTAINS,
    BUILTIN_STR_SLICE,
    BUILTIN_REGEX_MATCH
} BuiltinKind;

static const StringArray EMPTY_PARAMS = {NULL, 0U, 0U};
static const StmtArray EMPTY_BODY = {NULL, 0U, 0U};

static EvalResult result_ok(Value value) {
    EvalResult result;
    result.had_error = false;
    result.is_return = false;
    result.is_break = false;
    result.is_continue = false;
    result.value = value;
    result.runtime_error_kind = RUNTIME_ERROR_NONE;
    result.error_line = 0;
    result.error_column = 0;
    result.error_width = 0;
    result.error_message = NULL;
    return result;
}

static const char *runtime_error_kind_name(RuntimeErrorKind kind) {
    switch (kind) {
        case RUNTIME_ERROR_NONE:
            return "none";
        case RUNTIME_ERROR_TYPE:
            return "type";
        case RUNTIME_ERROR_NAME:
            return "name";
        case RUNTIME_ERROR_ARITY:
            return "arity";
        case RUNTIME_ERROR_LOOKUP:
            return "lookup";
        case RUNTIME_ERROR_ASSIGNMENT:
            return "assignment";
        case RUNTIME_ERROR_CONTROL_FLOW:
            return "control-flow";
        case RUNTIME_ERROR_INDEX:
            return "index";
        case RUNTIME_ERROR_MATH:
            return "math";
        case RUNTIME_ERROR_INTERNAL:
            return "internal";
    }
    return "unknown";
}

static EvalResult result_error(char *message) {
    EvalResult result = result_ok(value_null());
    result.had_error = true;
    result.error_message = message;
    return result;
}

static char *read_module_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long size;
    size_t read_count;
    char *buffer;

    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);
    buffer = jsrt_malloc((size_t)size + 1U);
    read_count = fread(buffer, 1U, (size_t)size, file);
    buffer[read_count] = '\0';
    fclose(file);
    return buffer;
}

static char *module_dirname(const char *path) {
    const char *slash;

    if (path == NULL || path[0] == '\0') {
        return jsrt_strdup(".");
    }
    slash = strrchr(path, '/');
    if (slash == NULL) {
        return jsrt_strdup(".");
    }
    if (slash == path) {
        return jsrt_strdup("/");
    }
    {
        size_t length = (size_t)(slash - path);
        char *dir = jsrt_malloc(length + 1U);
        memcpy(dir, path, length);
        dir[length] = '\0';
        return dir;
    }
}

static char *resolve_module_path(const char *base_path, const char *specifier) {
    char *candidate;

    if (specifier[0] == '/') {
        candidate = jsrt_strdup(specifier);
    } else {
        char *dir = module_dirname(base_path);
        candidate = jsrt_format("%s/%s", dir, specifier);
        free(dir);
    }
    return candidate;
}

static ModuleRecord *module_record_find(const char *path) {
    ModuleRecord *record = module_records;
    while (record != NULL) {
        if (strcmp(record->path, path) == 0) {
            return record;
        }
        record = record->next;
    }
    return NULL;
}

static ModuleRecord *module_record_create(const char *path, char *source) {
    ModuleRecord *record = jsrt_malloc(sizeof(*record));
    record->path = jsrt_strdup(path);
    record->source = source;
    token_array_init(&record->tokens);
    stmt_array_init(&record->program);
    record->env = env_create(NULL);
    string_array_init(&record->exports);
    record->lexed = false;
    record->parsed = false;
    record->loading = true;
    record->next = module_records;
    module_records = record;
    return record;
}

static void module_records_free_all(void) {
    ModuleRecord *record = module_records;

    while (record != NULL) {
        ModuleRecord *next = record->next;
        if (record->parsed) {
            stmt_array_free(&record->program);
        }
        if (record->lexed) {
            token_array_free(&record->tokens);
        }
        string_array_free(&record->exports);
        free(record->source);
        free(record->path);
        free(record);
        record = next;
    }
    module_records = NULL;
}

static EvalResult result_runtime_error_at(
    Evaluator *evaluator,
    RuntimeErrorKind kind,
    int line,
    int column,
    int width,
    const char *message
) {
    EvalResult result;
    char *label = jsrt_format("Runtime error [%s]", runtime_error_kind_name(kind));
    char *diagnostic = format_source_diagnostic(label, evaluator->source, line, column, width, message);

    free(label);
    result = result_error(diagnostic);
    result.runtime_error_kind = kind;
    result.error_line = line;
    result.error_column = column;
    result.error_width = width;
    return result;
}

static EvalResult result_runtime_error_at_take(
    Evaluator *evaluator,
    RuntimeErrorKind kind,
    int line,
    int column,
    int width,
    char *message
) {
    EvalResult result = result_runtime_error_at(evaluator, kind, line, column, width, message);
    free(message);
    return result;
}

void eval_result_free(EvalResult *result) {
    if (result == NULL) {
        return;
    }
    if (eval_execution_depth == 0) {
        module_records_free_all();
    }
    free(result->error_message);
    result->error_message = NULL;
    result->runtime_error_kind = RUNTIME_ERROR_NONE;
    result->error_line = 0;
    result->error_column = 0;
    result->error_width = 0;
}

static bool ensure_number(Evaluator *evaluator, Value value, const Expr *expr, const char *context, EvalResult *out_error) {
    if (value.type != VALUE_NUMBER) {
        *out_error = result_runtime_error_at(evaluator, RUNTIME_ERROR_TYPE, expr->line, expr->column, 1, context);
        return false;
    }
    return true;
}

static EvalResult eval_expression(Evaluator *evaluator, const Expr *expr);
static EvalResult execute_statement(Evaluator *evaluator, const Stmt *stmt);
static EvalResult invoke_function(
    Evaluator *evaluator,
    const Expr *surface_expr,
    Value callee_value,
    Value receiver_value,
    const ExprArray *arguments_expr,
    bool is_constructor
);
static EvalResult eval_program_internal(
    const StmtArray *program,
    const char *source,
    Env *env,
    const char *module_path,
    StringArray *exports
);
static ptrdiff_t object_find_key(const JsObject *object, const char *key);
static void object_set_key(JsObject *object, const char *key, Value value);
static bool object_lookup_key(JsObject *object, const char *key, JsObject **owner, ptrdiff_t *index);

static JsObject *create_object(size_t count) {
    JsObject *object = jsrt_malloc(sizeof(*object));
    size_t i;

    gc_register_object(&object->gc, GC_OBJECT_OBJECT);
    object->count = count;
    object->items = jsrt_malloc(sizeof(*object->items) * count);
    object->prototype = NULL;
    for (i = 0; i < count; ++i) {
        object->items[i].key = NULL;
        object->items[i].value = value_null();
    }
    return object;
}

static JsFunction *create_user_function(
    const char *name,
    const StringArray *params,
    const StmtArray *body,
    const Stmt *declaration,
    const Expr *expression,
    Env *closure,
    bool is_class_constructor
) {
    JsFunction *function = jsrt_malloc(sizeof(*function));
    gc_register_object(&function->gc, GC_OBJECT_FUNCTION);
    function->is_native = false;
    function->is_class_constructor = is_class_constructor;
    function->declaration = declaration;
    function->expression = expression;
    function->name = name;
    function->params = params;
    function->body = body;
    function->closure = closure;
    function->native_name = NULL;
    function->native_arity = 0U;
    function->bytecode = NULL;
    function->prototype = value_object_take(create_object(0U));
    function->captures = NULL;
    function->capture_count = 0U;
    return function;
}

static JsFunction *create_native_function(const char *name, size_t arity, BuiltinKind kind) {
    JsFunction *function = jsrt_malloc(sizeof(*function));
    gc_register_object(&function->gc, GC_OBJECT_FUNCTION);
    function->is_native = true;
    function->is_class_constructor = false;
    function->declaration = (const Stmt *)(size_t)(kind + 1);
    function->expression = NULL;
    function->name = name;
    function->params = NULL;
    function->body = NULL;
    function->closure = NULL;
    function->native_name = name;
    function->native_arity = arity;
    function->bytecode = NULL;
    function->prototype = value_null();
    function->captures = NULL;
    function->capture_count = 0U;
    return function;
}

static BuiltinKind js_native_kind(const JsFunction *function) {
    return (BuiltinKind)((size_t)function->declaration - 1U);
}

static const char *js_function_name(const JsFunction *function) {
    if (function->is_native) {
        return function->native_name;
    }
    return function->name;
}

static const StringArray *js_function_params(const JsFunction *function) {
    if (function->is_native) {
        return NULL;
    }
    return function->params;
}

static const StmtArray *js_function_body(const JsFunction *function) {
    if (function->is_native) {
        return NULL;
    }
    return function->body;
}

static const char *value_type_name(Value value) {
    switch (value.type) {
        case VALUE_NUMBER:
            return "number";
        case VALUE_BOOL:
            return "bool";
        case VALUE_NULL:
            return "null";
        case VALUE_STRING:
            return "string";
        case VALUE_FUNCTION:
            return "function";
        case VALUE_ARRAY:
            return "array";
        case VALUE_OBJECT:
            return "object";
    }
    return "unknown";
}

static bool value_is_object_like(Value value) {
    return value.type == VALUE_OBJECT || value.type == VALUE_ARRAY || value.type == VALUE_FUNCTION;
}

static bool regex_match_here(const char *pattern, const char *text);

static bool regex_match_star(int c, const char *pattern, const char *text) {
    const char *cursor = text;

    do {
        if (regex_match_here(pattern, cursor)) {
            return true;
        }
    } while (*cursor != '\0' && (*cursor++ == c || c == '.'));

    return false;
}

static bool regex_match_here(const char *pattern, const char *text) {
    if (pattern[0] == '\0') {
        return true;
    }
    if (pattern[1] == '*') {
        return regex_match_star(pattern[0], pattern + 2, text);
    }
    if (pattern[0] == '$' && pattern[1] == '\0') {
        return *text == '\0';
    }
    if (*text != '\0' && (pattern[0] == '.' || pattern[0] == *text)) {
        return regex_match_here(pattern + 1, text + 1);
    }
    return false;
}

static bool regex_match_bounded(const char *pattern, const char *text) {
    if (pattern[0] == '^') {
        return regex_match_here(pattern + 1, text);
    }
    do {
        if (regex_match_here(pattern, text)) {
            return true;
        }
    } while (*text++ != '\0');
    return false;
}

static bool function_property_is_prototype(const char *name) {
    return strcmp(name, "prototype") == 0;
}

static EvalResult unsupported_function_property_error(Evaluator *evaluator, int line, int column) {
    return result_runtime_error_at(
        evaluator,
        RUNTIME_ERROR_TYPE,
        line,
        column,
        1,
        "only function.prototype is supported in L3"
    );
}

static EvalResult read_named_property(Evaluator *evaluator, Value target, const char *name, int line, int column) {
    ptrdiff_t key_index;
    JsObject *owner;

    if (target.type == VALUE_OBJECT) {
        if (object_lookup_key(target.as.object, name, &owner, &key_index)) {
            Value result = value_clone(owner->items[(size_t)key_index].value);
            value_free(target);
            return result_ok(result);
        }
        value_free(target);
        return result_runtime_error_at_take(
            evaluator,
            RUNTIME_ERROR_LOOKUP,
            line,
            column,
            1,
            jsrt_format("object key '%s' not found", name)
        );
    }

    if (target.type == VALUE_FUNCTION) {
        if (function_property_is_prototype(name)) {
            Value result = value_clone(target.as.function->prototype);
            value_free(target);
            return result_ok(result);
        }
        value_free(target);
        return unsupported_function_property_error(evaluator, line, column);
    }

    value_free(target);
    return result_runtime_error_at(
        evaluator,
        RUNTIME_ERROR_TYPE,
        line,
        column,
        1,
        "property access requires an object value"
    );
}

static EvalResult read_indexed_property(Evaluator *evaluator, Value target, Value key, int line, int column) {
    ptrdiff_t key_index;
    JsObject *owner;

    if (target.type == VALUE_ARRAY) {
        size_t index;
        if (key.type != VALUE_NUMBER) {
            value_free(target);
            value_free(key);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                line,
                column,
                1,
                "array indexing expects a numeric index"
            );
        }
        index = (size_t)key.as.number;
        if ((double)index != key.as.number || index >= target.as.array->count) {
            value_free(target);
            value_free(key);
            return result_runtime_error_at(evaluator, RUNTIME_ERROR_INDEX, line, column, 1, "array index out of bounds");
        }
        {
            Value result = value_clone(target.as.array->items[index]);
            value_free(target);
            value_free(key);
            return result_ok(result);
        }
    }

    if (target.type == VALUE_OBJECT) {
        if (key.type != VALUE_STRING) {
            value_free(target);
            value_free(key);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                line,
                column,
                1,
                "object indexing expects a string key"
            );
        }
        if (object_lookup_key(target.as.object, key.as.string, &owner, &key_index)) {
            Value result = value_clone(owner->items[(size_t)key_index].value);
            value_free(target);
            value_free(key);
            return result_ok(result);
        }
        {
            char *message = jsrt_format("object key '%s' not found", key.as.string);
            value_free(target);
            value_free(key);
            return result_runtime_error_at_take(evaluator, RUNTIME_ERROR_LOOKUP, line, column, 1, message);
        }
    }

    if (target.type == VALUE_FUNCTION) {
        if (key.type != VALUE_STRING) {
            value_free(target);
            value_free(key);
            return unsupported_function_property_error(evaluator, line, column);
        }
        if (function_property_is_prototype(key.as.string)) {
            Value result = value_clone(target.as.function->prototype);
            value_free(target);
            value_free(key);
            return result_ok(result);
        }
        value_free(target);
        value_free(key);
        return unsupported_function_property_error(evaluator, line, column);
    }

    value_free(target);
    value_free(key);
    return result_runtime_error_at(
        evaluator,
        RUNTIME_ERROR_TYPE,
        line,
        column,
        1,
        "indexing requires an array or object value"
    );
}

static EvalResult assign_named_property(Evaluator *evaluator, Value *target, const char *name, Value value, int line, int column) {
    if (target->type == VALUE_OBJECT) {
        object_set_key(target->as.object, name, value);
        return result_ok(value_null());
    }
    if (target->type == VALUE_FUNCTION) {
        if (!function_property_is_prototype(name)) {
            return unsupported_function_property_error(evaluator, line, column);
        }
        if (target->as.function->is_native) {
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                line,
                column,
                1,
                "native function prototypes are read-only in L3"
            );
        }
        if (value.type != VALUE_NULL && value.type != VALUE_OBJECT) {
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                line,
                column,
                1,
                "function prototype must be an object or null"
            );
        }
        value_free(target->as.function->prototype);
        target->as.function->prototype = value_clone(value);
        return result_ok(value_null());
    }
    return result_runtime_error_at(
        evaluator,
        RUNTIME_ERROR_TYPE,
        line,
        column,
        1,
        "property assignment requires an object value"
    );
}

static EvalResult assign_indexed_property(Evaluator *evaluator, Value *target, Value key, Value value, int line, int column) {
    if (target->type == VALUE_ARRAY) {
        size_t index;
        if (key.type != VALUE_NUMBER) {
            value_free(key);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                line,
                column,
                1,
                "array assignment expects a numeric index"
            );
        }
        index = (size_t)key.as.number;
        if ((double)index != key.as.number || index >= target->as.array->count) {
            value_free(key);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_INDEX,
                line,
                column,
                1,
                "array assignment index out of bounds"
            );
        }
        value_free(target->as.array->items[index]);
        target->as.array->items[index] = value_clone(value);
        value_free(key);
        return result_ok(value_null());
    }
    if (target->type == VALUE_OBJECT) {
        if (key.type != VALUE_STRING) {
            value_free(key);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                line,
                column,
                1,
                "object assignment expects a string key"
            );
        }
        object_set_key(target->as.object, key.as.string, value);
        value_free(key);
        return result_ok(value_null());
    }
    if (target->type == VALUE_FUNCTION) {
        EvalResult result;
        if (key.type != VALUE_STRING) {
            value_free(key);
            return unsupported_function_property_error(evaluator, line, column);
        }
        result = assign_named_property(evaluator, target, key.as.string, value, line, column);
        value_free(key);
        return result;
    }
    value_free(key);
    return result_runtime_error_at(
        evaluator,
        RUNTIME_ERROR_TYPE,
        line,
        column,
        1,
        "index assignment requires an array or object value"
    );
}

static EvalResult call_native_function(Evaluator *evaluator, const Expr *expr, const JsFunction *function, Value *arguments) {
    switch (js_native_kind(function)) {
        case BUILTIN_PRINT: {
            char *text = value_to_string(arguments[0]);
            printf("%s\n", text);
            free(text);
            return result_ok(value_null());
        }
        case BUILTIN_LEN:
            switch (arguments[0].type) {
                case VALUE_STRING:
                    return result_ok(value_number((double)strlen(arguments[0].as.string)));
                case VALUE_ARRAY:
                    return result_ok(value_number((double)arguments[0].as.array->count));
                case VALUE_OBJECT:
                    return result_ok(value_number((double)arguments[0].as.object->count));
                default:
                    return result_runtime_error_at(
                        evaluator,
                        RUNTIME_ERROR_TYPE,
                        expr->line,
                        expr->column,
                        1,
                        "len() expects a string, array, or object"
                    );
            }
        case BUILTIN_TYPEOF:
            return result_ok(value_string(value_type_name(arguments[0])));
        case BUILTIN_SET_PROTOTYPE_OF: {
            JsObject *receiver;
            JsObject *prototype = NULL;

            if (arguments[0].type != VALUE_OBJECT || arguments[0].as.object == NULL) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    expr->line,
                    expr->column,
                    1,
                    "setPrototypeOf() expects an object receiver"
                );
            }
            if (arguments[1].type != VALUE_NULL &&
                (arguments[1].type != VALUE_OBJECT || arguments[1].as.object == NULL)) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    expr->line,
                    expr->column,
                    1,
                    "setPrototypeOf() expects an object or null prototype"
                );
            }

            receiver = arguments[0].as.object;
            if (arguments[1].type == VALUE_OBJECT) {
                prototype = arguments[1].as.object;
            }

            if (prototype == receiver) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_ASSIGNMENT,
                    expr->line,
                    expr->column,
                    1,
                    "setPrototypeOf() cannot set an object as its own prototype"
                );
            }

            while (prototype != NULL) {
                if (prototype == receiver) {
                    return result_runtime_error_at(
                        evaluator,
                        RUNTIME_ERROR_ASSIGNMENT,
                        expr->line,
                        expr->column,
                        1,
                        "setPrototypeOf() cannot create a prototype cycle"
                    );
                }
                prototype = prototype->prototype;
            }

            receiver->prototype = arguments[1].type == VALUE_OBJECT ? arguments[1].as.object : NULL;
            return result_ok(value_clone(arguments[0]));
        }
        case BUILTIN_DEFER:
            if (arguments[0].type != VALUE_FUNCTION || arguments[0].as.function == NULL) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    expr->line,
                    expr->column,
                    1,
                    "defer() expects a function callback"
                );
            }
            job_queue_push(evaluator->jobs, arguments[0]);
            return result_ok(value_null());
        case BUILTIN_RUN_JOBS: {
            ExprArray empty_arguments = {NULL, 0U, 0U};
            while (evaluator->jobs != NULL && evaluator->jobs->count > 0U) {
                Value callback = job_queue_pop_front(evaluator->jobs);
                EvalResult callback_result = invoke_function(
                    evaluator,
                    expr,
                    callback,
                    value_null(),
                    &empty_arguments,
                    false
                );
                value_free(callback);
                if (callback_result.had_error) {
                    return callback_result;
                }
                value_free(callback_result.value);
            }
            return result_ok(value_null());
        }
        case BUILTIN_STR_CONTAINS:
            if (arguments[0].type != VALUE_STRING || arguments[1].type != VALUE_STRING) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    expr->line,
                    expr->column,
                    1,
                    "strContains() expects two strings"
                );
            }
            return result_ok(value_bool(strstr(arguments[0].as.string, arguments[1].as.string) != NULL));
        case BUILTIN_STR_SLICE: {
            size_t length;
            size_t start;
            size_t end;
            char *slice;
            if (arguments[0].type != VALUE_STRING || arguments[1].type != VALUE_NUMBER || arguments[2].type != VALUE_NUMBER) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    expr->line,
                    expr->column,
                    1,
                    "strSlice() expects a string and numeric start/end"
                );
            }
            length = strlen(arguments[0].as.string);
            start = (size_t)arguments[1].as.number;
            end = (size_t)arguments[2].as.number;
            if ((double)start != arguments[1].as.number || (double)end != arguments[2].as.number ||
                start > end || end > length) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_INDEX,
                    expr->line,
                    expr->column,
                    1,
                    "strSlice() range is out of bounds"
                );
            }
            slice = jsrt_malloc(end - start + 1U);
            memcpy(slice, arguments[0].as.string + start, end - start);
            slice[end - start] = '\0';
            return result_ok(value_string_take(slice));
        }
        case BUILTIN_REGEX_MATCH:
            if (arguments[0].type != VALUE_STRING || arguments[1].type != VALUE_STRING) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    expr->line,
                    expr->column,
                    1,
                    "regexMatch() expects text and pattern strings"
                );
            }
            return result_ok(value_bool(regex_match_bounded(arguments[1].as.string, arguments[0].as.string)));
    }

    return result_runtime_error_at(evaluator, RUNTIME_ERROR_INTERNAL, expr->line, expr->column, 1, "unsupported builtin function");
}

static void define_builtin(Env *env, const char *name, size_t arity, BuiltinKind kind) {
    Value builtin = value_function(create_native_function(name, arity, kind));
    env_define(env, name, builtin, true);
    value_free(builtin);
}

static void define_global_builtins(Env *env) {
    define_builtin(env, "print", 1U, BUILTIN_PRINT);
    define_builtin(env, "len", 1U, BUILTIN_LEN);
    define_builtin(env, "typeOf", 1U, BUILTIN_TYPEOF);
    define_builtin(env, "setPrototypeOf", 2U, BUILTIN_SET_PROTOTYPE_OF);
    define_builtin(env, "defer", 1U, BUILTIN_DEFER);
    define_builtin(env, "runJobs", 0U, BUILTIN_RUN_JOBS);
    define_builtin(env, "strContains", 2U, BUILTIN_STR_CONTAINS);
    define_builtin(env, "strSlice", 3U, BUILTIN_STR_SLICE);
    define_builtin(env, "regexMatch", 2U, BUILTIN_REGEX_MATCH);
}

static EvalResult execute_switch(Evaluator *evaluator, const Stmt *stmt) {
    EvalResult value_result = eval_expression(evaluator, stmt->as.switch_stmt.expression);
    GcTempRoot value_root;
    size_t start_index = stmt->as.switch_stmt.cases.count;
    size_t i;

    if (value_result.had_error) {
        return value_result;
    }

    gc_temp_root_push_value(&value_root, &value_result.value);
    for (i = 0; i < stmt->as.switch_stmt.cases.count; ++i) {
        SwitchCase *switch_case = &stmt->as.switch_stmt.cases.items[i];
        if (switch_case->is_default) {
            if (start_index == stmt->as.switch_stmt.cases.count) {
                start_index = i;
            }
            continue;
        }
        {
            EvalResult case_value = eval_expression(evaluator, switch_case->value);
            bool matched;
            if (case_value.had_error) {
                gc_temp_root_pop(&value_root);
                value_free(value_result.value);
                return case_value;
            }
            matched = value_equals(value_result.value, case_value.value);
            value_free(case_value.value);
            if (matched) {
                start_index = i;
                break;
            }
        }
    }
    gc_temp_root_pop(&value_root);

    if (start_index == stmt->as.switch_stmt.cases.count) {
        value_free(value_result.value);
        return result_ok(value_null());
    }

    value_free(value_result.value);
    evaluator->switch_depth += 1;
    for (i = start_index; i < stmt->as.switch_stmt.cases.count; ++i) {
        size_t j;
        StmtArray *statements = &stmt->as.switch_stmt.cases.items[i].statements;
        for (j = 0; j < statements->count; ++j) {
            EvalResult result = execute_statement(evaluator, statements->items[j]);
            if (result.had_error || result.is_return || result.is_continue) {
                evaluator->switch_depth -= 1;
                return result;
            }
            if (result.is_break) {
                value_free(result.value);
                evaluator->switch_depth -= 1;
                return result_ok(value_null());
            }
            value_free(result.value);
        }
    }
    evaluator->switch_depth -= 1;
    return result_ok(value_null());
}

static void assign_path_init(AssignPath *path) {
    path->root_name = NULL;
    path->root_is_this = false;
    path->items = NULL;
    path->count = 0;
    path->capacity = 0;
}

static void assign_path_free(AssignPath *path) {
    free(path->items);
    path->items = NULL;
    path->count = 0;
    path->capacity = 0;
}

static void assign_path_push(AssignPath *path, AssignStep step) {
    if (path->count == path->capacity) {
        size_t next_capacity = path->capacity == 0 ? 4U : path->capacity * 2U;
        path->items = jsrt_realloc(path->items, next_capacity * sizeof(*path->items));
        path->capacity = next_capacity;
    }
    path->items[path->count++] = step;
}

static bool build_assign_path(const Expr *expr, AssignPath *path) {
    AssignStep step;

    switch (expr->kind) {
        case EXPR_IDENTIFIER:
            path->root_name = expr->as.identifier.name;
            path->root_is_this = false;
            return true;
        case EXPR_THIS:
            path->root_name = NULL;
            path->root_is_this = true;
            return true;
        case EXPR_PROPERTY:
            if (!build_assign_path(expr->as.property.target, path)) {
                return false;
            }
            step.kind = ASSIGN_STEP_PROPERTY;
            step.property_name = expr->as.property.name;
            step.index_expr = NULL;
            step.line = expr->line;
            step.column = expr->column;
            assign_path_push(path, step);
            return true;
        case EXPR_INDEX:
            if (!build_assign_path(expr->as.index.target, path)) {
                return false;
            }
            step.kind = ASSIGN_STEP_INDEX;
            step.property_name = NULL;
            step.index_expr = expr->as.index.index;
            step.line = expr->line;
            step.column = expr->column;
            assign_path_push(path, step);
            return true;
        default:
            return false;
    }
}

static ptrdiff_t object_find_key(const JsObject *object, const char *key) {
    size_t i;

    for (i = 0; i < object->count; ++i) {
        if (strcmp(object->items[i].key, key) == 0) {
            return (ptrdiff_t)i;
        }
    }

    return -1;
}

static void object_set_key(JsObject *object, const char *key, Value value) {
    ptrdiff_t existing = object_find_key(object, key);

    if (existing >= 0) {
        value_free(object->items[existing].value);
        object->items[existing].value = value_clone(value);
        return;
    }

    object->items = jsrt_realloc(object->items, sizeof(*object->items) * (object->count + 1U));
    object->items[object->count].key = jsrt_strdup(key);
    object->items[object->count].value = value_clone(value);
    object->count += 1U;
}

static bool object_lookup_key(JsObject *object, const char *key, JsObject **owner, ptrdiff_t *index) {
    JsObject *current = object;

    while (current != NULL) {
        ptrdiff_t current_index = object_find_key(current, key);
        if (current_index >= 0) {
            if (owner != NULL) {
                *owner = current;
            }
            if (index != NULL) {
                *index = current_index;
            }
            return true;
        }
        current = current->prototype;
    }

    return false;
}

static EvalResult assign_into_path(Evaluator *evaluator, Value *root_value, const AssignPath *path, Value assigned_value) {
    GcTempRoot root_value_root;
    GcTempRoot assigned_value_root;
    size_t i;
    Value *current = root_value;

    gc_temp_root_push_value(&root_value_root, root_value);
    gc_temp_root_push_value(&assigned_value_root, &assigned_value);

    for (i = 0; i < path->count; ++i) {
        bool is_last = i + 1U == path->count;
        const AssignStep *step = &path->items[i];

        if (step->kind == ASSIGN_STEP_PROPERTY) {
            ptrdiff_t key_index;
            JsObject *owner;

            if (is_last) {
                EvalResult result = assign_named_property(
                    evaluator,
                    current,
                    step->property_name,
                    assigned_value,
                    step->line,
                    step->column
                );
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return result;
            }
            if (current->type == VALUE_FUNCTION) {
                if (!function_property_is_prototype(step->property_name)) {
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return unsupported_function_property_error(evaluator, step->line, step->column);
                }
                current = &current->as.function->prototype;
                continue;
            }
            if (current->type != VALUE_OBJECT) {
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_TYPE,
                    step->line,
                    step->column,
                    1,
                    "property assignment requires an object value"
                );
            }
            if (!object_lookup_key(current->as.object, step->property_name, &owner, &key_index)) {
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return result_runtime_error_at_take(
                    evaluator,
                    RUNTIME_ERROR_LOOKUP,
                    step->line,
                    step->column,
                    1,
                    jsrt_format("object key '%s' not found", step->property_name)
                );
            }
            current = &owner->items[(size_t)key_index].value;
            continue;
        }

        {
            EvalResult index_result = eval_expression(evaluator, step->index_expr);
            if (index_result.had_error) {
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return index_result;
            }

            if (current->type == VALUE_ARRAY) {
                size_t index;
                if (index_result.value.type != VALUE_NUMBER) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_runtime_error_at(
                        evaluator,
                        RUNTIME_ERROR_TYPE,
                        step->line,
                        step->column,
                        1,
                        "array assignment expects a numeric index"
                    );
                }
                index = (size_t)index_result.value.as.number;
                if ((double)index != index_result.value.as.number || index >= current->as.array->count) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_runtime_error_at(
                        evaluator,
                        RUNTIME_ERROR_INDEX,
                        step->line,
                        step->column,
                        1,
                        "array assignment index out of bounds"
                    );
                }
                value_free(index_result.value);
                if (is_last) {
                    value_free(current->as.array->items[index]);
                    current->as.array->items[index] = value_clone(assigned_value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_ok(value_null());
                }
                current = &current->as.array->items[index];
                continue;
            }

            if (current->type == VALUE_OBJECT) {
                ptrdiff_t key_index;
                JsObject *owner;
                if (index_result.value.type != VALUE_STRING) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_runtime_error_at(
                        evaluator,
                        RUNTIME_ERROR_TYPE,
                        step->line,
                        step->column,
                        1,
                        "object assignment expects a string key"
                    );
                }
                key_index = object_find_key(current->as.object, index_result.value.as.string);
                if (is_last) {
                    object_set_key(current->as.object, index_result.value.as.string, assigned_value);
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_ok(value_null());
                }
                if (!object_lookup_key(current->as.object, index_result.value.as.string, &owner, &key_index)) {
                    char *message = jsrt_format("object key '%s' not found", index_result.value.as.string);
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_runtime_error_at_take(evaluator, RUNTIME_ERROR_LOOKUP, step->line, step->column, 1, message);
                }
                value_free(index_result.value);
                current = &owner->items[(size_t)key_index].value;
                continue;
            }

            if (current->type == VALUE_FUNCTION) {
                if (is_last) {
                    EvalResult result = assign_indexed_property(
                        evaluator,
                        current,
                        index_result.value,
                        assigned_value,
                        step->line,
                        step->column
                    );
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result;
                }
                if (index_result.value.type != VALUE_STRING) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return unsupported_function_property_error(evaluator, step->line, step->column);
                }
                if (!function_property_is_prototype(index_result.value.as.string)) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return unsupported_function_property_error(evaluator, step->line, step->column);
                }
                value_free(index_result.value);
                current = &current->as.function->prototype;
                continue;
            }

            value_free(index_result.value);
            gc_temp_root_pop(&assigned_value_root);
            gc_temp_root_pop(&root_value_root);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_TYPE,
                step->line,
                step->column,
                1,
                "index assignment requires an array or object value"
            );
        }
    }

    gc_temp_root_pop(&assigned_value_root);
    gc_temp_root_pop(&root_value_root);
    return result_runtime_error_at(evaluator, RUNTIME_ERROR_ASSIGNMENT, 1, 1, 1, "invalid assignment path");
}

static EvalResult eval_assignment(Evaluator *evaluator, const Expr *expr) {
    EvalResult value_result = eval_expression(evaluator, expr->as.assign.value);

    if (value_result.had_error) {
        return value_result;
    }

    if (expr->as.assign.target->kind == EXPR_IDENTIFIER) {
        const char *name = expr->as.assign.target->as.identifier.name;
        {
            EnvAssignResult assign_result = env_assign(evaluator->env, name, value_result.value);
            if (assign_result == ENV_ASSIGN_OK) {
                return value_result;
            }
            value_free(value_result.value);
            if (assign_result == ENV_ASSIGN_CONST) {
                return result_runtime_error_at_take(
                    evaluator,
                    RUNTIME_ERROR_ASSIGNMENT,
                    expr->as.assign.target->line,
                    expr->as.assign.target->column,
                    (int)strlen(name),
                    jsrt_format("cannot assign to const variable '%s'", name)
                );
            }
            return result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_NAME,
                expr->as.assign.target->line,
                expr->as.assign.target->column,
                (int)strlen(name),
                jsrt_format("undefined variable '%s'", name)
            );
        }
    }

    {
        AssignPath path;
        Value root_value;
        EvalResult assign_result;
        GcTempRoot value_root;
        GcTempRoot target_root;

        assign_path_init(&path);
        if (!build_assign_path(expr->as.assign.target, &path) || (!path.root_is_this && path.root_name == NULL)) {
            assign_path_free(&path);
            value_free(value_result.value);
            return result_runtime_error_at(
                evaluator,
                RUNTIME_ERROR_ASSIGNMENT,
                expr->line,
                expr->column,
                1,
                "assignment target must start with a variable"
            );
        }
        if (path.root_is_this) {
            if (!env_get_this(evaluator->env, &root_value)) {
                assign_path_free(&path);
                value_free(value_result.value);
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_INTERNAL,
                    expr->as.assign.target->line,
                    expr->as.assign.target->column,
                    4,
                    "missing this binding"
                );
            }
        } else if (!env_get(evaluator->env, path.root_name, &root_value)) {
            assign_path_free(&path);
            value_free(value_result.value);
            return result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_NAME,
                expr->as.assign.target->line,
                expr->as.assign.target->column,
                (int)strlen(path.root_name),
                jsrt_format("undefined variable '%s'", path.root_name)
            );
        }

        gc_temp_root_push_value(&value_root, &value_result.value);
        gc_temp_root_push_value(&target_root, &root_value);
        assign_result = assign_into_path(evaluator, &root_value, &path, value_result.value);
        gc_temp_root_pop(&target_root);
        gc_temp_root_pop(&value_root);
        assign_path_free(&path);
        if (assign_result.had_error) {
            value_free(root_value);
            value_free(value_result.value);
            return assign_result;
        }
        eval_result_free(&assign_result);
        value_free(root_value);
        return value_result;
    }
}

static void collect_garbage(Evaluator *evaluator, const Value *root_value) {
    ActiveEnvScope *scope = evaluator->active_envs != NULL ? evaluator->active_envs->previous : NULL;
    GcTempRoot *active_roots = NULL;
    GcTempRoot *job_roots = NULL;
    size_t root_count = 0;
    size_t job_count = evaluator->jobs != NULL ? evaluator->jobs->count : 0U;
    size_t i;

    while (scope != NULL) {
        root_count += 1U;
        scope = scope->previous;
    }
    if (root_count > 0) {
        active_roots = jsrt_malloc(sizeof(*active_roots) * root_count);
        scope = evaluator->active_envs->previous;
        for (i = 0; i < root_count; ++i) {
            gc_temp_root_push_env(&active_roots[i], scope->env);
            scope = scope->previous;
        }
    }
    if (job_count > 0) {
        job_roots = jsrt_malloc(sizeof(*job_roots) * job_count);
        for (i = 0; i < job_count; ++i) {
            gc_temp_root_push_value(&job_roots[i], &evaluator->jobs->items[i]);
        }
    }
    gc_collect_with_reason(evaluator->env, root_value, "eval-safe-point");
    for (i = job_count; i > 0; --i) {
        gc_temp_root_pop(&job_roots[i - 1U]);
    }
    for (i = root_count; i > 0; --i) {
        gc_temp_root_pop(&active_roots[i - 1U]);
    }
    free(job_roots);
    free(active_roots);
}

static EvalResult execute_block(Evaluator *evaluator, const StmtArray *statements, Env *env) {
    Env *previous = evaluator->env;
    ActiveEnvScope active_scope;
    EvalResult result = result_ok(value_null());
    size_t i;

    active_scope.env = env;
    active_scope.previous = evaluator->active_envs;
    evaluator->env = env;
    evaluator->active_envs = &active_scope;
    for (i = 0; i < statements->count; ++i) {
        value_free(result.value);
        result.value = value_null();
        collect_garbage(evaluator, NULL);
        result = execute_statement(evaluator, statements->items[i]);
        if (result.had_error || result.is_return || result.is_break || result.is_continue) {
            evaluator->active_envs = active_scope.previous;
            evaluator->env = previous;
            return result;
        }
    }

    evaluator->active_envs = active_scope.previous;
    evaluator->env = previous;
    return result;
}

typedef struct {
    Value callee;
    Value receiver;
} EvaluatedCallTarget;

static void evaluated_call_target_init(EvaluatedCallTarget *target) {
    target->callee = value_null();
    target->receiver = value_null();
}

static void evaluated_call_target_free(EvaluatedCallTarget *target) {
    value_free(target->callee);
    value_free(target->receiver);
    target->callee = value_null();
    target->receiver = value_null();
}

static EvalResult evaluate_call_target(Evaluator *evaluator, const Expr *callee_expr, EvaluatedCallTarget *out) {
    evaluated_call_target_init(out);

    if (callee_expr->kind == EXPR_PROPERTY) {
        EvalResult target_result = eval_expression(evaluator, callee_expr->as.property.target);
        EvalResult property_result;

        if (target_result.had_error) {
            return target_result;
        }
        out->receiver = value_clone(target_result.value);
        property_result = read_named_property(
            evaluator,
            target_result.value,
            callee_expr->as.property.name,
            callee_expr->line,
            callee_expr->column
        );
        if (property_result.had_error) {
            value_free(out->receiver);
            out->receiver = value_null();
            return property_result;
        }
        out->callee = property_result.value;
        return result_ok(value_null());
    }

    if (callee_expr->kind == EXPR_INDEX) {
        EvalResult target_result = eval_expression(evaluator, callee_expr->as.index.target);
        EvalResult index_result;
        EvalResult property_result;
        GcTempRoot target_root;

        if (target_result.had_error) {
            return target_result;
        }
        gc_temp_root_push_value(&target_root, &target_result.value);
        index_result = eval_expression(evaluator, callee_expr->as.index.index);
        gc_temp_root_pop(&target_root);
        if (index_result.had_error) {
            value_free(target_result.value);
            return index_result;
        }
        out->receiver = value_clone(target_result.value);
        property_result = read_indexed_property(
            evaluator,
            target_result.value,
            index_result.value,
            callee_expr->line,
            callee_expr->column
        );
        if (property_result.had_error) {
            value_free(out->receiver);
            out->receiver = value_null();
            return property_result;
        }
        out->callee = property_result.value;
        return result_ok(value_null());
    }

    out->receiver = value_null();
    return eval_expression(evaluator, callee_expr);
}

static EvalResult invoke_function(
    Evaluator *evaluator,
    const Expr *surface_expr,
    Value callee_value,
    Value receiver_value,
    const ExprArray *arguments_expr,
    bool is_constructor
) {
    size_t i;
    Value *arguments;
    GcTempRoot *argument_roots = NULL;
    GcTempRoot callee_root;
    GcTempRoot receiver_root;
    EvalResult result;
    const StringArray *params;
    const StmtArray *body;
    const char *function_name;
    Env *call_env;
    const JsFunction *function;

    if (callee_value.type != VALUE_FUNCTION || callee_value.as.function == NULL) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_TYPE,
            surface_expr->line,
            surface_expr->column,
            1,
            is_constructor ? "attempted to construct a non-function value" : "attempted to call a non-function value"
        );
    }

    function = callee_value.as.function;
    params = js_function_params(function);
    body = js_function_body(function);
    function_name = js_function_name(function);
    if (is_constructor && function->is_native) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_TYPE,
            surface_expr->line,
            surface_expr->column,
            1,
            "native functions are not constructors"
        );
    }
    if (!is_constructor && function->is_class_constructor) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_TYPE,
            surface_expr->line,
            surface_expr->column,
            1,
            "class constructors must be called with 'new'"
        );
    }
    if (!function->is_native && (params == NULL || body == NULL)) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_INTERNAL,
            surface_expr->line,
            surface_expr->column,
            1,
            "invalid function body"
        );
    }

    if ((!function->is_native && arguments_expr->count != params->count) ||
        (function->is_native && arguments_expr->count != function->native_arity)) {
        return result_runtime_error_at_take(
            evaluator,
            RUNTIME_ERROR_ARITY,
            surface_expr->line,
            surface_expr->column,
            1,
            jsrt_format(
                "expected %zu arguments but got %zu",
                function->is_native ? function->native_arity : params->count,
                arguments_expr->count
            )
        );
    }

    arguments = jsrt_malloc(sizeof(*arguments) * arguments_expr->count);
    if (arguments_expr->count > 0) {
        argument_roots = jsrt_malloc(sizeof(*argument_roots) * arguments_expr->count);
    }
    gc_temp_root_push_value(&callee_root, &callee_value);
    gc_temp_root_push_value(&receiver_root, &receiver_value);
    if (is_constructor && receiver_value.type == VALUE_OBJECT) {
        receiver_value.as.object->prototype =
            function->prototype.type == VALUE_OBJECT ? function->prototype.as.object : NULL;
    }
    for (i = 0; i < arguments_expr->count; ++i) {
        EvalResult arg_result = eval_expression(evaluator, arguments_expr->items[i]);
        if (arg_result.had_error) {
            size_t j;
            for (j = i; j > 0; --j) {
                gc_temp_root_pop(&argument_roots[j - 1U]);
                value_free(arguments[j - 1U]);
            }
            free(argument_roots);
            free(arguments);
            gc_temp_root_pop(&receiver_root);
            gc_temp_root_pop(&callee_root);
            return arg_result;
        }
        arguments[i] = arg_result.value;
        gc_temp_root_push_value(&argument_roots[i], &arguments[i]);
    }

    if (function->is_native) {
        result = call_native_function(evaluator, surface_expr, function, arguments);
        for (i = arguments_expr->count; i > 0; --i) {
            gc_temp_root_pop(&argument_roots[i - 1U]);
            value_free(arguments[i - 1U]);
        }
        free(argument_roots);
        free(arguments);
        gc_temp_root_pop(&receiver_root);
        gc_temp_root_pop(&callee_root);
        return result;
    }

    call_env = env_create(function->closure);
    env_set_this(call_env, receiver_value);
    if (function_name != NULL) {
        env_define(call_env, function_name, callee_value, true);
    }
    for (i = 0; i < params->count; ++i) {
        env_define(call_env, params->items[i], arguments[i], false);
    }
    for (i = arguments_expr->count; i > 0; --i) {
        gc_temp_root_pop(&argument_roots[i - 1U]);
        value_free(arguments[i - 1U]);
    }
    free(argument_roots);
    free(arguments);

    {
        int previous_loop_depth = evaluator->loop_depth;
        int previous_switch_depth = evaluator->switch_depth;
        evaluator->loop_depth = 0;
        evaluator->switch_depth = 0;
        result = execute_block(evaluator, body, call_env);
        evaluator->loop_depth = previous_loop_depth;
        evaluator->switch_depth = previous_switch_depth;
    }
    env_destroy(call_env);
    collect_garbage(evaluator, result.had_error ? NULL : &result.value);
    gc_temp_root_pop(&receiver_root);
    gc_temp_root_pop(&callee_root);

    if (result.had_error) {
        return result;
    }
    if (result.is_return) {
        result.is_return = false;
        if (is_constructor) {
            if (value_is_object_like(result.value)) {
                return result;
            }
            value_free(result.value);
            return result_ok(value_clone(receiver_value));
        }
        return result;
    }
    value_free(result.value);
    if (is_constructor) {
        return result_ok(value_clone(receiver_value));
    }
    return result_ok(value_null());
}

static EvalResult eval_call(Evaluator *evaluator, const Expr *expr) {
    EvaluatedCallTarget target;
    EvalResult target_result = evaluate_call_target(evaluator, expr->as.call.callee, &target);
    EvalResult result;

    if (target_result.had_error) {
        return target_result;
    }
    if (expr->as.call.callee->kind != EXPR_PROPERTY && expr->as.call.callee->kind != EXPR_INDEX) {
        target.callee = target_result.value;
    }
    result = invoke_function(evaluator, expr, target.callee, target.receiver, &expr->as.call.arguments, false);
    evaluated_call_target_free(&target);
    return result;
}

static EvalResult eval_new(Evaluator *evaluator, const Expr *expr) {
    EvaluatedCallTarget target;
    EvalResult target_result = evaluate_call_target(evaluator, expr->as.new_expr.callee, &target);
    EvalResult result;

    if (target_result.had_error) {
        return target_result;
    }
    if (expr->as.new_expr.callee->kind != EXPR_PROPERTY && expr->as.new_expr.callee->kind != EXPR_INDEX) {
        target.callee = target_result.value;
    }
    value_free(target.receiver);
    target.receiver = value_object_take(create_object(0U));
    result = invoke_function(evaluator, expr, target.callee, target.receiver, &expr->as.new_expr.arguments, true);
    evaluated_call_target_free(&target);
    return result;
}

static EvalResult eval_binary_plus(Value left_value, Value right_value) {
    if (left_value.type == VALUE_NUMBER && right_value.type == VALUE_NUMBER) {
        double value = left_value.as.number + right_value.as.number;
        value_free(left_value);
        value_free(right_value);
        return result_ok(value_number(value));
    }

    {
        char *left_text = value_to_string(left_value);
        char *right_text = value_to_string(right_value);
        char *joined = jsrt_format("%s%s", left_text, right_text);
        free(left_text);
        free(right_text);
        value_free(left_value);
        value_free(right_value);
        return result_ok(value_string_take(joined));
    }
}

static EvalResult eval_logical(Evaluator *evaluator, const Expr *expr) {
    EvalResult left = eval_expression(evaluator, expr->as.logical.left);
    bool truthy;

    if (left.had_error) {
        return left;
    }

    truthy = value_is_truthy(left.value);
    if (expr->as.logical.op == TOKEN_OR_OR) {
        if (truthy) {
            return left;
        }
    } else if (expr->as.logical.op == TOKEN_AND_AND) {
        if (!truthy) {
            return left;
        }
    } else {
        value_free(left.value);
        return result_runtime_error_at(evaluator, RUNTIME_ERROR_INTERNAL, expr->line, expr->column, 2, "unsupported logical operator");
    }

    value_free(left.value);
    return eval_expression(evaluator, expr->as.logical.right);
}

static EvalResult eval_array_literal(Evaluator *evaluator, const Expr *expr) {
    size_t i;
    JsArray *array = jsrt_malloc(sizeof(*array));
    Value array_value;
    GcTempRoot array_root;

    gc_register_object(&array->gc, GC_OBJECT_ARRAY);
    array->count = expr->as.array.elements.count;
    array->items = jsrt_malloc(sizeof(*array->items) * array->count);
    array_value = value_array_take(array);
    gc_temp_root_push_value(&array_root, &array_value);
    for (i = 0; i < array->count; ++i) {
        array->items[i] = value_null();
    }

    for (i = 0; i < array->count; ++i) {
        EvalResult element = eval_expression(evaluator, expr->as.array.elements.items[i]);
        if (element.had_error) {
            gc_temp_root_pop(&array_root);
            return element;
        }
        array->items[i] = element.value;
    }

    gc_temp_root_pop(&array_root);
    return result_ok(array_value);
}

static EvalResult eval_object_literal(Evaluator *evaluator, const Expr *expr) {
    size_t i;
    JsObject *object = create_object(expr->as.object.properties.count);
    Value object_value;
    GcTempRoot object_root;

    object_value = value_object_take(object);
    gc_temp_root_push_value(&object_root, &object_value);

    for (i = 0; i < object->count; ++i) {
        EvalResult property_value = eval_expression(evaluator, expr->as.object.properties.items[i].value);
        if (property_value.had_error) {
            gc_temp_root_pop(&object_root);
            return property_value;
        }
        object->items[i].key = jsrt_strdup(expr->as.object.properties.items[i].key);
        object->items[i].value = property_value.value;
    }

    gc_temp_root_pop(&object_root);
    return result_ok(object_value);
}

static EvalResult eval_index(Evaluator *evaluator, const Expr *expr) {
    EvalResult target = eval_expression(evaluator, expr->as.index.target);
    EvalResult index;
    GcTempRoot target_root;

    if (target.had_error) {
        return target;
    }
    gc_temp_root_push_value(&target_root, &target.value);
    index = eval_expression(evaluator, expr->as.index.index);
    gc_temp_root_pop(&target_root);
    if (index.had_error) {
        value_free(target.value);
        return index;
    }
    return read_indexed_property(evaluator, target.value, index.value, expr->line, expr->column);
}

static EvalResult eval_property(Evaluator *evaluator, const Expr *expr) {
    EvalResult target = eval_expression(evaluator, expr->as.property.target);
    if (target.had_error) {
        return target;
    }
    return read_named_property(evaluator, target.value, expr->as.property.name, expr->line, expr->column);
}

static EvalResult eval_expression(Evaluator *evaluator, const Expr *expr) {
    switch (expr->kind) {
        case EXPR_LITERAL:
            switch (expr->as.literal.kind) {
                case LITERAL_NUMBER:
                    return result_ok(value_number(expr->as.literal.number));
                case LITERAL_BOOL:
                    return result_ok(value_bool(expr->as.literal.boolean));
                case LITERAL_NULL:
                    return result_ok(value_null());
                case LITERAL_STRING:
                    return result_ok(value_string(expr->as.literal.string));
            }
            break;
        case EXPR_IDENTIFIER: {
            Value value;
            if (!env_get(evaluator->env, expr->as.identifier.name, &value)) {
                return result_runtime_error_at_take(
                    evaluator,
                    RUNTIME_ERROR_NAME,
                    expr->line,
                    expr->column,
                    (int)strlen(expr->as.identifier.name),
                    jsrt_format("undefined variable '%s'", expr->as.identifier.name)
                );
            }
            return result_ok(value);
        }
        case EXPR_THIS: {
            Value value;
            if (env_get_this(evaluator->env, &value)) {
                return result_ok(value);
            }
            return result_ok(value_null());
        }
        case EXPR_ASSIGN: {
            return eval_assignment(evaluator, expr);
        }
        case EXPR_UNARY: {
            EvalResult operand_result = eval_expression(evaluator, expr->as.unary.operand);
            if (operand_result.had_error) {
                return operand_result;
            }
            if (expr->as.unary.op == TOKEN_MINUS) {
                EvalResult error;
                if (!ensure_number(evaluator, operand_result.value, expr, "unary '-' expects a number", &error)) {
                    value_free(operand_result.value);
                    return error;
                }
                operand_result.value.as.number = -operand_result.value.as.number;
                return operand_result;
            }
            if (expr->as.unary.op == TOKEN_BANG) {
                bool truthy = value_is_truthy(operand_result.value);
                value_free(operand_result.value);
                return result_ok(value_bool(!truthy));
            }
            value_free(operand_result.value);
            return result_runtime_error_at(evaluator, RUNTIME_ERROR_INTERNAL, expr->line, expr->column, 1, "unsupported unary operator");
        }
        case EXPR_BINARY: {
            EvalResult left = eval_expression(evaluator, expr->as.binary.left);
            GcTempRoot left_root;
            EvalResult right;
            EvalResult error;
            if (left.had_error) {
                return left;
            }
            gc_temp_root_push_value(&left_root, &left.value);
            right = eval_expression(evaluator, expr->as.binary.right);
            if (right.had_error) {
                gc_temp_root_pop(&left_root);
                value_free(left.value);
                return right;
            }
            gc_temp_root_pop(&left_root);

            switch (expr->as.binary.op) {
                case TOKEN_PLUS:
                    return eval_binary_plus(left.value, right.value);
                case TOKEN_MINUS:
                case TOKEN_STAR:
                case TOKEN_SLASH:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQUAL:
                case TOKEN_LESS:
                case TOKEN_LESS_EQUAL:
                    if (!ensure_number(evaluator, left.value, expr, "binary operator expects numbers", &error) ||
                        !ensure_number(evaluator, right.value, expr, "binary operator expects numbers", &error)) {
                        value_free(left.value);
                        value_free(right.value);
                        return error;
                    }
                    break;
                default:
                    break;
            }

            switch (expr->as.binary.op) {
                case TOKEN_MINUS: {
                    double value = left.value.as.number - right.value.as.number;
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_number(value));
                }
                case TOKEN_STAR: {
                    double value = left.value.as.number * right.value.as.number;
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_number(value));
                }
                case TOKEN_SLASH: {
                    if (right.value.as.number == 0.0) {
                        value_free(left.value);
                        value_free(right.value);
                        return result_runtime_error_at(evaluator, RUNTIME_ERROR_MATH, expr->line, expr->column, 1, "division by zero");
                    }
                    {
                        double value = left.value.as.number / right.value.as.number;
                        value_free(left.value);
                        value_free(right.value);
                        return result_ok(value_number(value));
                    }
                }
                case TOKEN_GREATER: {
                    bool value = left.value.as.number > right.value.as.number;
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_bool(value));
                }
                case TOKEN_GREATER_EQUAL: {
                    bool value = left.value.as.number >= right.value.as.number;
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_bool(value));
                }
                case TOKEN_LESS: {
                    bool value = left.value.as.number < right.value.as.number;
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_bool(value));
                }
                case TOKEN_LESS_EQUAL: {
                    bool value = left.value.as.number <= right.value.as.number;
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_bool(value));
                }
                case TOKEN_EQUAL_EQUAL: {
                    bool value = value_equals(left.value, right.value);
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_bool(value));
                }
                case TOKEN_BANG_EQUAL: {
                    bool value = !value_equals(left.value, right.value);
                    value_free(left.value);
                    value_free(right.value);
                    return result_ok(value_bool(value));
                }
                default:
                    value_free(left.value);
                    value_free(right.value);
                    return result_runtime_error_at(evaluator, RUNTIME_ERROR_INTERNAL, expr->line, expr->column, 1, "unsupported binary operator");
            }
        }
        case EXPR_LOGICAL:
            return eval_logical(evaluator, expr);
        case EXPR_CALL:
            return eval_call(evaluator, expr);
        case EXPR_NEW:
            return eval_new(evaluator, expr);
        case EXPR_FUNCTION: {
            JsFunction *function = create_user_function(
                expr->as.function.name,
                &expr->as.function.params,
                &expr->as.function.body,
                NULL,
                expr,
                evaluator->env,
                false
            );
            return result_ok(value_function(function));
        }
        case EXPR_ARRAY:
            return eval_array_literal(evaluator, expr);
        case EXPR_OBJECT:
            return eval_object_literal(evaluator, expr);
        case EXPR_INDEX:
            return eval_index(evaluator, expr);
        case EXPR_PROPERTY:
            return eval_property(evaluator, expr);
    }
    return result_runtime_error_at(evaluator, RUNTIME_ERROR_INTERNAL, expr->line, expr->column, 1, "unsupported expression kind");
}

static bool string_array_contains(const StringArray *array, const char *value) {
    size_t i;
    for (i = 0; i < array->count; ++i) {
        if (strcmp(array->items[i], value) == 0) {
            return true;
        }
    }
    return false;
}

static const char *export_declaration_name(const Stmt *declaration) {
    switch (declaration->kind) {
        case STMT_VAR:
            return declaration->as.var.name;
        case STMT_FUNCTION:
            return declaration->as.function.name;
        case STMT_CLASS:
            return declaration->as.class_stmt.name;
        default:
            return NULL;
    }
}

static EvalResult execute_export_statement(Evaluator *evaluator, const Stmt *stmt) {
    const char *name = export_declaration_name(stmt->as.export_stmt.declaration);
    EvalResult result;

    if (evaluator->exports == NULL) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_CONTROL_FLOW,
            stmt->line,
            stmt->column,
            6,
            "export declarations require module evaluation"
        );
    }
    if (evaluator->env != evaluator->root_env) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_CONTROL_FLOW,
            stmt->line,
            stmt->column,
            6,
            "export declarations are only supported at module top level"
        );
    }
    if (name == NULL) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_INTERNAL,
            stmt->line,
            stmt->column,
            6,
            "unsupported export declaration"
        );
    }

    result = execute_statement(evaluator, stmt->as.export_stmt.declaration);
    if (result.had_error) {
        return result;
    }
    if (!string_array_contains(evaluator->exports, name)) {
        string_array_push(evaluator->exports, jsrt_strdup(name));
    }
    return result;
}

static EvalResult load_module_record(Evaluator *evaluator, const Stmt *import_stmt, const char *specifier, ModuleRecord **out_record) {
    char *resolved = resolve_module_path(evaluator->module_path, specifier);
    ModuleRecord *record = module_record_find(resolved);

    if (record != NULL) {
        if (record->loading) {
            EvalResult error = result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_INTERNAL,
                import_stmt->line,
                import_stmt->column,
                6,
                jsrt_format("cyclic module import is unsupported: %s", specifier)
            );
            free(resolved);
            return error;
        }
        *out_record = record;
        free(resolved);
        return result_ok(value_null());
    }

    {
        char *source = read_module_file(resolved);
        char *error = NULL;
        EvalResult module_result;

        if (source == NULL) {
            EvalResult load_error = result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_LOOKUP,
                import_stmt->line,
                import_stmt->column,
                6,
                jsrt_format("unable to read module '%s'", specifier)
            );
            free(resolved);
            return load_error;
        }

        record = module_record_create(resolved, source);
        free(resolved);

        if (!lex_source(record->source, &record->tokens, &error)) {
            record->loading = false;
            module_result = result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_INTERNAL,
                import_stmt->line,
                import_stmt->column,
                6,
                jsrt_format("module lex error: %s", error != NULL ? error : "unknown error")
            );
            free(error);
            return module_result;
        }
        record->lexed = true;

        if (!parse_tokens(&record->tokens, record->source, &record->program, &error)) {
            record->loading = false;
            module_result = result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_INTERNAL,
                import_stmt->line,
                import_stmt->column,
                6,
                jsrt_format("module parse error: %s", error != NULL ? error : "unknown error")
            );
            free(error);
            return module_result;
        }
        record->parsed = true;

        {
            GcTempRoot importer_root;
            gc_temp_root_push_env(&importer_root, evaluator->root_env);
            module_result = eval_program_internal(&record->program, record->source, record->env, record->path, &record->exports);
            gc_temp_root_pop(&importer_root);
        }
        record->loading = false;
        if (module_result.had_error) {
            return module_result;
        }
        value_free(module_result.value);
        *out_record = record;
        return result_ok(value_null());
    }
}

static EvalResult execute_import_statement(Evaluator *evaluator, const Stmt *stmt) {
    ModuleRecord *record = NULL;
    EvalResult load_result;
    size_t i;

    if (evaluator->env != evaluator->root_env) {
        return result_runtime_error_at(
            evaluator,
            RUNTIME_ERROR_CONTROL_FLOW,
            stmt->line,
            stmt->column,
            6,
            "import declarations are only supported at module top level"
        );
    }

    load_result = load_module_record(evaluator, stmt, stmt->as.import_stmt.module_path, &record);
    if (load_result.had_error) {
        return load_result;
    }
    value_free(load_result.value);

    for (i = 0; i < stmt->as.import_stmt.specifiers.count; ++i) {
        const ImportSpecifier *specifier = &stmt->as.import_stmt.specifiers.items[i];
        Value imported_value;

        if (!string_array_contains(&record->exports, specifier->imported_name)) {
            return result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_LOOKUP,
                stmt->line,
                stmt->column,
                6,
                jsrt_format("module '%s' does not export '%s'", stmt->as.import_stmt.module_path, specifier->imported_name)
            );
        }
        if (!env_get(record->env, specifier->imported_name, &imported_value)) {
            return result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_LOOKUP,
                stmt->line,
                stmt->column,
                6,
                jsrt_format("module export '%s' is unavailable", specifier->imported_name)
            );
        }
        if (!env_define(evaluator->env, specifier->local_name, imported_value, true)) {
            value_free(imported_value);
            return result_runtime_error_at_take(
                evaluator,
                RUNTIME_ERROR_NAME,
                stmt->line,
                stmt->column,
                (int)strlen(specifier->local_name),
                jsrt_format("redeclaration of '%s' in the same scope", specifier->local_name)
            );
        }
        value_free(imported_value);
    }

    return result_ok(value_null());
}

static EvalResult execute_statement(Evaluator *evaluator, const Stmt *stmt) {
    switch (stmt->kind) {
        case STMT_EXPR:
            return eval_expression(evaluator, stmt->as.expr.expression);
        case STMT_VAR: {
            EvalResult result = stmt->as.var.initializer != NULL
                ? eval_expression(evaluator, stmt->as.var.initializer)
                : result_ok(value_null());
            if (result.had_error) {
                return result;
            }
                if (!env_define(evaluator->env, stmt->as.var.name, result.value, stmt->as.var.is_const)) {
                    value_free(result.value);
                    return result_runtime_error_at_take(
                        evaluator,
                        RUNTIME_ERROR_NAME,
                        stmt->line,
                        stmt->column,
                        (int)strlen(stmt->as.var.name),
                    jsrt_format("redeclaration of '%s' in the same scope", stmt->as.var.name)
                );
            }
            return result;
        }
        case STMT_BLOCK: {
            Env *block_env = env_create(evaluator->env);
            EvalResult result = execute_block(evaluator, &stmt->as.block.statements, block_env);
            env_destroy(block_env);
            collect_garbage(evaluator, result.had_error ? NULL : &result.value);
            return result;
        }
        case STMT_IF: {
            EvalResult condition = eval_expression(evaluator, stmt->as.if_stmt.condition);
            if (condition.had_error) {
                return condition;
            }
            if (value_is_truthy(condition.value)) {
                value_free(condition.value);
                return execute_statement(evaluator, stmt->as.if_stmt.then_branch);
            }
            value_free(condition.value);
            if (stmt->as.if_stmt.else_branch != NULL) {
                return execute_statement(evaluator, stmt->as.if_stmt.else_branch);
            }
            return result_ok(value_null());
        }
        case STMT_DO_WHILE: {
            evaluator->loop_depth += 1;
            for (;;) {
                EvalResult body = execute_statement(evaluator, stmt->as.do_while_stmt.body);
                if (body.had_error || body.is_return) {
                    evaluator->loop_depth -= 1;
                    return body;
                }
                if (body.is_break) {
                    value_free(body.value);
                    evaluator->loop_depth -= 1;
                    return result_ok(value_null());
                }
                value_free(body.value);

                {
                    EvalResult condition = eval_expression(evaluator, stmt->as.do_while_stmt.condition);
                    if (condition.had_error) {
                        evaluator->loop_depth -= 1;
                        return condition;
                    }
                    if (!value_is_truthy(condition.value)) {
                        value_free(condition.value);
                        evaluator->loop_depth -= 1;
                        return result_ok(value_null());
                    }
                    value_free(condition.value);
                }
            }
        }
        case STMT_SWITCH:
            return execute_switch(evaluator, stmt);
        case STMT_IMPORT:
            return execute_import_statement(evaluator, stmt);
        case STMT_EXPORT:
            return execute_export_statement(evaluator, stmt);
        case STMT_FOR: {
            Env *previous_env = evaluator->env;
            ActiveEnvScope active_scope;
            EvalResult result = result_ok(value_null());

            active_scope.env = env_create(evaluator->env);
            active_scope.previous = evaluator->active_envs;
            evaluator->env = active_scope.env;
            evaluator->active_envs = &active_scope;

            if (stmt->as.for_stmt.init != NULL) {
                result = execute_statement(evaluator, stmt->as.for_stmt.init);
                if (result.had_error || result.is_return) {
                    evaluator->active_envs = active_scope.previous;
                    evaluator->env = previous_env;
                    env_destroy(active_scope.env);
                    return result;
                }
                value_free(result.value);
                result.value = value_null();
            }

            evaluator->loop_depth += 1;
            for (;;) {
                EvalResult condition = stmt->as.for_stmt.condition != NULL
                    ? eval_expression(evaluator, stmt->as.for_stmt.condition)
                    : result_ok(value_bool(true));
                if (condition.had_error) {
                    evaluator->loop_depth -= 1;
                    evaluator->active_envs = active_scope.previous;
                    evaluator->env = previous_env;
                    env_destroy(active_scope.env);
                    return condition;
                }
                if (!value_is_truthy(condition.value)) {
                    value_free(condition.value);
                    break;
                }
                value_free(condition.value);

                {
                    EvalResult body = execute_statement(evaluator, stmt->as.for_stmt.body);
                    bool should_continue = body.is_continue;
                    bool should_break = body.is_break;

                    if (body.had_error || body.is_return) {
                        evaluator->loop_depth -= 1;
                        evaluator->active_envs = active_scope.previous;
                        evaluator->env = previous_env;
                        env_destroy(active_scope.env);
                        return body;
                    }

                    if (should_break) {
                        value_free(body.value);
                        break;
                    }

                    value_free(body.value);

                    if (stmt->as.for_stmt.increment != NULL) {
                        EvalResult increment = eval_expression(evaluator, stmt->as.for_stmt.increment);
                        if (increment.had_error) {
                            evaluator->loop_depth -= 1;
                            evaluator->active_envs = active_scope.previous;
                            evaluator->env = previous_env;
                            env_destroy(active_scope.env);
                            return increment;
                        }
                        value_free(increment.value);
                    }

                    if (should_continue) {
                        continue;
                    }
                }
            }

            evaluator->loop_depth -= 1;
            evaluator->active_envs = active_scope.previous;
            evaluator->env = previous_env;
            env_destroy(active_scope.env);
            collect_garbage(evaluator, NULL);
            return result_ok(value_null());
        }
        case STMT_WHILE: {
            evaluator->loop_depth += 1;
            for (;;) {
                EvalResult condition = eval_expression(evaluator, stmt->as.while_stmt.condition);
                if (condition.had_error) {
                    evaluator->loop_depth -= 1;
                    return condition;
                }
                if (!value_is_truthy(condition.value)) {
                    value_free(condition.value);
                    evaluator->loop_depth -= 1;
                    return result_ok(value_null());
                }
                value_free(condition.value);

                {
                    EvalResult body = execute_statement(evaluator, stmt->as.while_stmt.body);
                    if (body.had_error || body.is_return) {
                        evaluator->loop_depth -= 1;
                        return body;
                    }
                    if (body.is_break) {
                        value_free(body.value);
                        evaluator->loop_depth -= 1;
                        return result_ok(value_null());
                    }
                    if (body.is_continue) {
                        value_free(body.value);
                        continue;
                    }
                    value_free(body.value);
                }
            }
        }
        case STMT_FUNCTION: {
            JsFunction *function = create_user_function(
                stmt->as.function.name,
                &stmt->as.function.params,
                &stmt->as.function.body,
                stmt,
                NULL,
                evaluator->env,
                false
            );
            {
                Value fn_value = value_function(function);
                if (!env_define(evaluator->env, stmt->as.function.name, fn_value, true)) {
                    value_free(fn_value);
                    return result_runtime_error_at_take(
                        evaluator,
                        RUNTIME_ERROR_NAME,
                        stmt->line,
                        stmt->column,
                        (int)strlen(stmt->as.function.name),
                        jsrt_format("redeclaration of '%s' in the same scope", stmt->as.function.name)
                    );
                }
                value_free(fn_value);
            }
            return result_ok(value_null());
        }
        case STMT_CLASS: {
            const ClassMethod *constructor = NULL;
            JsFunction *function;
            Value class_value;
            size_t i;

            for (i = 0; i < stmt->as.class_stmt.methods.count; ++i) {
                if (stmt->as.class_stmt.methods.items[i].is_constructor) {
                    constructor = &stmt->as.class_stmt.methods.items[i];
                    break;
                }
            }

            function = create_user_function(
                stmt->as.class_stmt.name,
                constructor != NULL ? &constructor->params : &EMPTY_PARAMS,
                constructor != NULL ? &constructor->body : &EMPTY_BODY,
                stmt,
                NULL,
                evaluator->env,
                true
            );
            class_value = value_function(function);
            if (!env_define(evaluator->env, stmt->as.class_stmt.name, class_value, true)) {
                value_free(class_value);
                return result_runtime_error_at_take(
                    evaluator,
                    RUNTIME_ERROR_NAME,
                    stmt->line,
                    stmt->column,
                    (int)strlen(stmt->as.class_stmt.name),
                    jsrt_format("redeclaration of '%s' in the same scope", stmt->as.class_stmt.name)
                );
            }

            for (i = 0; i < stmt->as.class_stmt.methods.count; ++i) {
                const ClassMethod *method = &stmt->as.class_stmt.methods.items[i];
                if (!method->is_constructor) {
                    JsFunction *method_function = create_user_function(
                        method->name,
                        &method->params,
                        &method->body,
                        NULL,
                        NULL,
                        evaluator->env,
                        false
                    );
                    Value method_value = value_function(method_function);
                    object_set_key(function->prototype.as.object, method->name, method_value);
                    value_free(method_value);
                }
            }

            value_free(class_value);
            return result_ok(value_null());
        }
        case STMT_RETURN: {
            EvalResult result = stmt->as.return_stmt.value != NULL
                ? eval_expression(evaluator, stmt->as.return_stmt.value)
                : result_ok(value_null());
            if (result.had_error) {
                return result;
            }
            result.is_return = true;
            return result;
        }
        case STMT_BREAK: {
            EvalResult result;
            if (evaluator->loop_depth <= 0 && evaluator->switch_depth <= 0) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_CONTROL_FLOW,
                    stmt->line,
                    stmt->column,
                    5,
                    "break is only valid inside a loop or switch"
                );
            }
            result = result_ok(value_null());
            result.is_break = true;
            return result;
        }
        case STMT_CONTINUE: {
            EvalResult result;
            if (evaluator->loop_depth <= 0) {
                return result_runtime_error_at(
                    evaluator,
                    RUNTIME_ERROR_CONTROL_FLOW,
                    stmt->line,
                    stmt->column,
                    8,
                    "continue is only valid inside a loop"
                );
            }
            result = result_ok(value_null());
            result.is_continue = true;
            return result;
        }
    }
    return result_runtime_error_at(evaluator, RUNTIME_ERROR_INTERNAL, stmt->line, stmt->column, 1, "unsupported statement kind");
}

static EvalResult eval_program_internal(
    const StmtArray *program,
    const char *source,
    Env *env,
    const char *module_path,
    StringArray *exports
) {
    ActiveEnvScope active_scope;
    Evaluator evaluator;
    JobQueue jobs;
    EvalResult result = result_ok(value_null());
    size_t i;

    job_queue_init(&jobs);
    active_scope.env = env;
    active_scope.previous = NULL;
    evaluator.source = source;
    evaluator.module_path = module_path;
    evaluator.env = env;
    evaluator.root_env = env;
    evaluator.exports = exports;
    evaluator.jobs = &jobs;
    evaluator.active_envs = &active_scope;
    evaluator.loop_depth = 0;
    evaluator.switch_depth = 0;
    define_global_builtins(env);
    env_set_this(env, value_null());

    eval_execution_depth += 1;
    for (i = 0; i < program->count; ++i) {
        value_free(result.value);
        result.value = value_null();
        collect_garbage(&evaluator, NULL);
        result = execute_statement(&evaluator, program->items[i]);
        if (result.had_error) {
            eval_execution_depth -= 1;
            job_queue_free(&jobs);
            return result;
        }
        if (result.is_return) {
            value_free(result.value);
            eval_execution_depth -= 1;
            job_queue_free(&jobs);
            return result_runtime_error_at(
                &evaluator,
                RUNTIME_ERROR_CONTROL_FLOW,
                program->items[i]->line,
                program->items[i]->column,
                6,
                "return is only valid inside a function body"
            );
        }
    }
    eval_execution_depth -= 1;
    job_queue_free(&jobs);
    return result;
}

EvalResult eval_program_with_path(const StmtArray *program, const char *source, Env *env, const char *module_path) {
    StringArray exports;
    EvalResult result;

    string_array_init(&exports);
    result = eval_program_internal(program, source, env, module_path, &exports);
    string_array_free(&exports);
    return result;
}

EvalResult eval_program(const StmtArray *program, const char *source, Env *env) {
    return eval_program_with_path(program, source, env, NULL);
}

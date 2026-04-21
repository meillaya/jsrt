#include "jsrt/eval.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/diagnostics.h"
#include "jsrt/gc.h"
#include "jsrt/memory.h"

typedef struct ActiveEnvScope {
    Env *env;
    struct ActiveEnvScope *previous;
} ActiveEnvScope;

typedef struct {
    const char *source;
    Env *env;
    ActiveEnvScope *active_envs;
    int loop_depth;
    int switch_depth;
} Evaluator;

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
    AssignStep *items;
    size_t count;
    size_t capacity;
} AssignPath;

typedef enum {
    BUILTIN_PRINT,
    BUILTIN_LEN,
    BUILTIN_TYPEOF
} BuiltinKind;

static EvalResult result_ok(Value value) {
    EvalResult result;
    result.had_error = false;
    result.is_return = false;
    result.is_break = false;
    result.is_continue = false;
    result.value = value;
    result.error_message = NULL;
    return result;
}

static EvalResult result_error(char *message) {
    EvalResult result = result_ok(value_null());
    result.had_error = true;
    result.error_message = message;
    return result;
}

static EvalResult result_error_at(Evaluator *evaluator, int line, int column, int width, const char *message) {
    return result_error(format_source_diagnostic("Runtime error", evaluator->source, line, column, width, message));
}

static EvalResult result_error_at_take(Evaluator *evaluator, int line, int column, int width, char *message) {
    EvalResult result = result_error_at(evaluator, line, column, width, message);
    free(message);
    return result;
}

void eval_result_free(EvalResult *result) {
    if (result == NULL) {
        return;
    }
    free(result->error_message);
    result->error_message = NULL;
}

static bool ensure_number(Evaluator *evaluator, Value value, const Expr *expr, const char *context, EvalResult *out_error) {
    if (value.type != VALUE_NUMBER) {
        *out_error = result_error_at(evaluator, expr->line, expr->column, 1, context);
        return false;
    }
    return true;
}

static EvalResult eval_expression(Evaluator *evaluator, const Expr *expr);
static EvalResult execute_statement(Evaluator *evaluator, const Stmt *stmt);

static JsFunction *create_user_function(const Stmt *declaration, const Expr *expression, Env *closure) {
    JsFunction *function = jsrt_malloc(sizeof(*function));
    gc_register_object(&function->gc, GC_OBJECT_FUNCTION);
    function->is_native = false;
    function->declaration = declaration;
    function->expression = expression;
    function->closure = closure;
    function->native_name = NULL;
    function->native_arity = 0U;
    return function;
}

static JsFunction *create_native_function(const char *name, size_t arity, BuiltinKind kind) {
    JsFunction *function = jsrt_malloc(sizeof(*function));
    gc_register_object(&function->gc, GC_OBJECT_FUNCTION);
    function->is_native = true;
    function->declaration = (const Stmt *)(size_t)(kind + 1);
    function->expression = NULL;
    function->closure = NULL;
    function->native_name = name;
    function->native_arity = arity;
    return function;
}

static BuiltinKind js_native_kind(const JsFunction *function) {
    return (BuiltinKind)((size_t)function->declaration - 1U);
}

static const char *js_function_name(const JsFunction *function) {
    if (function->is_native) {
        return function->native_name;
    }
    if (function->declaration != NULL && function->declaration->kind == STMT_FUNCTION) {
        return function->declaration->as.function.name;
    }
    if (function->expression != NULL) {
        return function->expression->as.function.name;
    }
    return NULL;
}

static const StringArray *js_function_params(const JsFunction *function) {
    if (function->is_native) {
        return NULL;
    }
    if (function->declaration != NULL && function->declaration->kind == STMT_FUNCTION) {
        return &function->declaration->as.function.params;
    }
    if (function->expression != NULL) {
        return &function->expression->as.function.params;
    }
    return NULL;
}

static const StmtArray *js_function_body(const JsFunction *function) {
    if (function->is_native) {
        return NULL;
    }
    if (function->declaration != NULL && function->declaration->kind == STMT_FUNCTION) {
        return &function->declaration->as.function.body;
    }
    if (function->expression != NULL) {
        return &function->expression->as.function.body;
    }
    return NULL;
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
                    return result_error_at(
                        evaluator,
                        expr->line,
                        expr->column,
                        1,
                        "len() expects a string, array, or object"
                    );
            }
        case BUILTIN_TYPEOF:
            return result_ok(value_string(value_type_name(arguments[0])));
    }

    return result_error_at(evaluator, expr->line, expr->column, 1, "unsupported builtin function");
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

            if (current->type != VALUE_OBJECT) {
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return result_error_at(evaluator, step->line, step->column, 1, "property assignment requires an object value");
            }

            key_index = object_find_key(current->as.object, step->property_name);
            if (is_last) {
                object_set_key(current->as.object, step->property_name, assigned_value);
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return result_ok(value_null());
            }
            if (key_index < 0) {
                gc_temp_root_pop(&assigned_value_root);
                gc_temp_root_pop(&root_value_root);
                return result_error_at_take(
                    evaluator,
                    step->line,
                    step->column,
                    1,
                    jsrt_format("object key '%s' not found", step->property_name)
                );
            }
            current = &current->as.object->items[key_index].value;
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
                    return result_error_at(evaluator, step->line, step->column, 1, "array assignment expects a numeric index");
                }
                index = (size_t)index_result.value.as.number;
                if ((double)index != index_result.value.as.number || index >= current->as.array->count) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_error_at(evaluator, step->line, step->column, 1, "array assignment index out of bounds");
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
                if (index_result.value.type != VALUE_STRING) {
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_error_at(evaluator, step->line, step->column, 1, "object assignment expects a string key");
                }
                key_index = object_find_key(current->as.object, index_result.value.as.string);
                if (is_last) {
                    object_set_key(current->as.object, index_result.value.as.string, assigned_value);
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_ok(value_null());
                }
                if (key_index < 0) {
                    char *message = jsrt_format("object key '%s' not found", index_result.value.as.string);
                    value_free(index_result.value);
                    gc_temp_root_pop(&assigned_value_root);
                    gc_temp_root_pop(&root_value_root);
                    return result_error_at_take(evaluator, step->line, step->column, 1, message);
                }
                value_free(index_result.value);
                current = &current->as.object->items[key_index].value;
                continue;
            }

            value_free(index_result.value);
            gc_temp_root_pop(&assigned_value_root);
            gc_temp_root_pop(&root_value_root);
            return result_error_at(evaluator, step->line, step->column, 1, "index assignment requires an array or object value");
        }
    }

    gc_temp_root_pop(&assigned_value_root);
    gc_temp_root_pop(&root_value_root);
    return result_error_at(evaluator, 1, 1, 1, "invalid assignment path");
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
                return result_error_at_take(
                    evaluator,
                    expr->as.assign.target->line,
                    expr->as.assign.target->column,
                    (int)strlen(name),
                    jsrt_format("cannot assign to const variable '%s'", name)
                );
            }
            return result_error_at_take(
                evaluator,
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
        if (!build_assign_path(expr->as.assign.target, &path) || path.root_name == NULL) {
            assign_path_free(&path);
            value_free(value_result.value);
            return result_error_at(evaluator, expr->line, expr->column, 1, "assignment target must start with a variable");
        }
        if (!env_get(evaluator->env, path.root_name, &root_value)) {
            assign_path_free(&path);
            value_free(value_result.value);
            return result_error_at_take(
                evaluator,
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

        {
            EnvAssignResult assign_root = env_assign(evaluator->env, path.root_name, root_value);
            if (assign_root == ENV_ASSIGN_OK) {
                value_free(root_value);
                return value_result;
            }
            value_free(root_value);
            value_free(value_result.value);
            if (assign_root == ENV_ASSIGN_CONST) {
                return result_error_at_take(
                    evaluator,
                    expr->as.assign.target->line,
                    expr->as.assign.target->column,
                    (int)strlen(path.root_name),
                    jsrt_format("cannot assign to const variable '%s'", path.root_name)
                );
            }
            return result_error_at_take(
                evaluator,
                expr->as.assign.target->line,
                expr->as.assign.target->column,
                (int)strlen(path.root_name),
                jsrt_format("undefined variable '%s'", path.root_name)
            );
        }
    }
}

static void collect_garbage(Evaluator *evaluator, const Value *root_value) {
    ActiveEnvScope *scope = evaluator->active_envs != NULL ? evaluator->active_envs->previous : NULL;
    GcTempRoot *active_roots = NULL;
    size_t root_count = 0;
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
    gc_collect_with_reason(evaluator->env, root_value, "eval-safe-point");
    for (i = root_count; i > 0; --i) {
        gc_temp_root_pop(&active_roots[i - 1U]);
    }
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

static EvalResult eval_call(Evaluator *evaluator, const Expr *expr) {
    EvalResult callee_result = eval_expression(evaluator, expr->as.call.callee);
    size_t i;
    Value *arguments;
    GcTempRoot *argument_roots = NULL;
    GcTempRoot callee_root;
    EvalResult result;
    const StringArray *params;
    const StmtArray *body;
    const char *function_name;
    Env *call_env;
    const JsFunction *function;

    if (callee_result.had_error) {
        return callee_result;
    }
    if (callee_result.value.type != VALUE_FUNCTION || callee_result.value.as.function == NULL) {
        value_free(callee_result.value);
        return result_error_at(evaluator, expr->line, expr->column, 1, "attempted to call a non-function value");
    }

    function = callee_result.value.as.function;
    params = js_function_params(function);
    body = js_function_body(function);
    function_name = js_function_name(function);
    if (!function->is_native && (params == NULL || body == NULL)) {
        value_free(callee_result.value);
        return result_error_at(evaluator, expr->line, expr->column, 1, "invalid function body");
    }

    if ((!function->is_native && expr->as.call.arguments.count != params->count) ||
        (function->is_native && expr->as.call.arguments.count != function->native_arity)) {
        value_free(callee_result.value);
        return result_error_at_take(
            evaluator,
            expr->line,
            expr->column,
            1,
            jsrt_format(
                "expected %zu arguments but got %zu",
                function->is_native ? function->native_arity : params->count,
                expr->as.call.arguments.count
            )
        );
    }

    arguments = jsrt_malloc(sizeof(*arguments) * expr->as.call.arguments.count);
    if (expr->as.call.arguments.count > 0) {
        argument_roots = jsrt_malloc(sizeof(*argument_roots) * expr->as.call.arguments.count);
    }
    gc_temp_root_push_value(&callee_root, &callee_result.value);
    for (i = 0; i < expr->as.call.arguments.count; ++i) {
        EvalResult arg_result = eval_expression(evaluator, expr->as.call.arguments.items[i]);
        if (arg_result.had_error) {
            size_t j;
            for (j = i; j > 0; --j) {
                gc_temp_root_pop(&argument_roots[j - 1U]);
                value_free(arguments[j - 1U]);
            }
            free(argument_roots);
            free(arguments);
            gc_temp_root_pop(&callee_root);
            value_free(callee_result.value);
            return arg_result;
        }
        arguments[i] = arg_result.value;
        gc_temp_root_push_value(&argument_roots[i], &arguments[i]);
    }

    if (function->is_native) {
        result = call_native_function(evaluator, expr, function, arguments);
        for (i = expr->as.call.arguments.count; i > 0; --i) {
            gc_temp_root_pop(&argument_roots[i - 1U]);
            value_free(arguments[i - 1U]);
        }
        free(argument_roots);
        free(arguments);
        gc_temp_root_pop(&callee_root);
        value_free(callee_result.value);
        return result;
    }

    call_env = env_create(function->closure);
    if (function_name != NULL) {
        env_define(call_env, function_name, callee_result.value, true);
    }
    for (i = 0; i < params->count; ++i) {
        env_define(call_env, params->items[i], arguments[i], false);
    }
    for (i = expr->as.call.arguments.count; i > 0; --i) {
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
    gc_temp_root_pop(&callee_root);
    value_free(callee_result.value);

    if (result.had_error) {
        return result;
    }
    if (result.is_return) {
        result.is_return = false;
        return result;
    }
    value_free(result.value);
    return result_ok(value_null());
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
        return result_error_at(evaluator, expr->line, expr->column, 2, "unsupported logical operator");
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
    JsObject *object = jsrt_malloc(sizeof(*object));
    Value object_value;
    GcTempRoot object_root;

    gc_register_object(&object->gc, GC_OBJECT_OBJECT);
    object->count = expr->as.object.properties.count;
    object->items = jsrt_malloc(sizeof(*object->items) * object->count);
    object_value = value_object_take(object);
    gc_temp_root_push_value(&object_root, &object_value);
    for (i = 0; i < object->count; ++i) {
        object->items[i].key = NULL;
        object->items[i].value = value_null();
    }

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
    size_t i;

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

    if (target.value.type == VALUE_ARRAY) {
        if (index.value.type != VALUE_NUMBER) {
            value_free(target.value);
            value_free(index.value);
            return result_error_at(evaluator, expr->line, expr->column, 1, "array indexing expects a numeric index");
        }
        i = (size_t)index.value.as.number;
        if ((double)i != index.value.as.number || i >= target.value.as.array->count) {
            value_free(target.value);
            value_free(index.value);
            return result_error_at(evaluator, expr->line, expr->column, 1, "array index out of bounds");
        }
        {
            Value result = value_clone(target.value.as.array->items[i]);
            value_free(target.value);
            value_free(index.value);
            return result_ok(result);
        }
    }

    if (target.value.type == VALUE_OBJECT) {
        if (index.value.type != VALUE_STRING) {
            value_free(target.value);
            value_free(index.value);
            return result_error_at(evaluator, expr->line, expr->column, 1, "object indexing expects a string key");
        }
        for (i = 0; i < target.value.as.object->count; ++i) {
            if (strcmp(target.value.as.object->items[i].key, index.value.as.string) == 0) {
                Value result = value_clone(target.value.as.object->items[i].value);
                value_free(target.value);
                value_free(index.value);
                return result_ok(result);
            }
        }
        value_free(target.value);
        value_free(index.value);
        return result_error_at_take(evaluator, expr->line, expr->column, 1, jsrt_format("object key '%s' not found", expr->as.index.index->kind == EXPR_LITERAL && expr->as.index.index->as.literal.kind == LITERAL_STRING ? expr->as.index.index->as.literal.string : "?"));
    }

    value_free(target.value);
    value_free(index.value);
    return result_error_at(evaluator, expr->line, expr->column, 1, "indexing requires an array or object value");
}

static EvalResult eval_property(Evaluator *evaluator, const Expr *expr) {
    EvalResult target = eval_expression(evaluator, expr->as.property.target);
    size_t i;
    if (target.had_error) {
        return target;
    }
    if (target.value.type != VALUE_OBJECT) {
        value_free(target.value);
        return result_error_at(evaluator, expr->line, expr->column, 1, "property access requires an object value");
    }
    for (i = 0; i < target.value.as.object->count; ++i) {
        if (strcmp(target.value.as.object->items[i].key, expr->as.property.name) == 0) {
            Value result = value_clone(target.value.as.object->items[i].value);
            value_free(target.value);
            return result_ok(result);
        }
    }
    value_free(target.value);
    return result_error_at_take(evaluator, expr->line, expr->column, 1, jsrt_format("object key '%s' not found", expr->as.property.name));
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
                return result_error_at_take(
                    evaluator,
                    expr->line,
                    expr->column,
                    (int)strlen(expr->as.identifier.name),
                    jsrt_format("undefined variable '%s'", expr->as.identifier.name)
                );
            }
            return result_ok(value);
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
            return result_error_at(evaluator, expr->line, expr->column, 1, "unsupported unary operator");
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
                        return result_error_at(evaluator, expr->line, expr->column, 1, "division by zero");
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
                    return result_error_at(evaluator, expr->line, expr->column, 1, "unsupported binary operator");
            }
        }
        case EXPR_LOGICAL:
            return eval_logical(evaluator, expr);
        case EXPR_CALL:
            return eval_call(evaluator, expr);
        case EXPR_FUNCTION: {
            JsFunction *function = create_user_function(NULL, expr, evaluator->env);
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
    return result_error_at(evaluator, expr->line, expr->column, 1, "unsupported expression kind");
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
                return result_error_at_take(
                    evaluator,
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
            JsFunction *function = create_user_function(stmt, NULL, evaluator->env);
            {
                Value fn_value = value_function(function);
                if (!env_define(evaluator->env, stmt->as.function.name, fn_value, true)) {
                    value_free(fn_value);
                    return result_error_at_take(
                        evaluator,
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
                return result_error_at(evaluator, stmt->line, stmt->column, 5, "break is only valid inside a loop or switch");
            }
            result = result_ok(value_null());
            result.is_break = true;
            return result;
        }
        case STMT_CONTINUE: {
            EvalResult result;
            if (evaluator->loop_depth <= 0) {
                return result_error_at(evaluator, stmt->line, stmt->column, 8, "continue is only valid inside a loop");
            }
            result = result_ok(value_null());
            result.is_continue = true;
            return result;
        }
    }
    return result_error_at(evaluator, stmt->line, stmt->column, 1, "unsupported statement kind");
}

EvalResult eval_program(const StmtArray *program, const char *source, Env *env) {
    ActiveEnvScope active_scope;
    Evaluator evaluator;
    EvalResult result = result_ok(value_null());
    size_t i;

    active_scope.env = env;
    active_scope.previous = NULL;
    evaluator.source = source;
    evaluator.env = env;
    evaluator.active_envs = &active_scope;
    evaluator.loop_depth = 0;
    evaluator.switch_depth = 0;
    define_global_builtins(env);

    for (i = 0; i < program->count; ++i) {
        value_free(result.value);
        result.value = value_null();
        collect_garbage(&evaluator, NULL);
        result = execute_statement(&evaluator, program->items[i]);
        if (result.had_error) {
            return result;
        }
        if (result.is_return) {
            value_free(result.value);
            return result_error_at(&evaluator, program->items[i]->line, program->items[i]->column, 6, "return is only valid inside a function body");
        }
    }
    return result;
}

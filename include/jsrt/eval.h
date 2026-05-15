#ifndef JSRT_EVAL_H
#define JSRT_EVAL_H

#include <stdbool.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/value.h"

typedef enum {
    RUNTIME_ERROR_NONE,
    RUNTIME_ERROR_TYPE,
    RUNTIME_ERROR_NAME,
    RUNTIME_ERROR_ARITY,
    RUNTIME_ERROR_LOOKUP,
    RUNTIME_ERROR_ASSIGNMENT,
    RUNTIME_ERROR_CONTROL_FLOW,
    RUNTIME_ERROR_INDEX,
    RUNTIME_ERROR_MATH,
    RUNTIME_ERROR_INTERNAL
} RuntimeErrorKind;

typedef struct {
    bool had_error;
    bool is_return;
    bool is_break;
    bool is_continue;
    Value value;
    RuntimeErrorKind runtime_error_kind;
    int error_line;
    int error_column;
    int error_width;
    char *error_message;
} EvalResult;

EvalResult eval_program(const StmtArray *program, const char *source, Env *env);
EvalResult eval_program_with_path(const StmtArray *program, const char *source, Env *env, const char *module_path);
void eval_result_free(EvalResult *result);

#endif

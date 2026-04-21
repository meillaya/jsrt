#ifndef JSRT_EVAL_H
#define JSRT_EVAL_H

#include <stdbool.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/value.h"

typedef struct {
    bool had_error;
    bool is_return;
    bool is_break;
    bool is_continue;
    Value value;
    char *error_message;
} EvalResult;

EvalResult eval_program(const StmtArray *program, const char *source, Env *env);
void eval_result_free(EvalResult *result);

#endif

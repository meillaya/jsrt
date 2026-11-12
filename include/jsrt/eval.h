#ifndef JSRT_EVAL_H
#define JSRT_EVAL_H

#include <stdbool.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/value.h"

/* Deepest chain of user function calls the tree-walk evaluator accepts. The
 * evaluator recurses on the C stack, so deeper chains would crash the host
 * instead of reporting an error. The VM applies the same limit. */
#define JSRT_MAX_CALL_DEPTH 256

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

/* Both entry points take ownership of the parsed program so functions in it
 * stay valid for as long as the environment lives. The incoming StmtArray is
 * reset; freeing it afterwards is a no-op. */
EvalResult eval_program(StmtArray *program, const char *source, Env *env);
EvalResult eval_program_with_path(StmtArray *program, const char *source, Env *env, const char *module_path);
void eval_result_free(EvalResult *result);

/* Host cancellation hook, checked between statements and between VM
 * instructions. The check runs often, so keep it cheap. A true return stops
 * the running program with an "execution interrupted" error. Pass NULL to
 * clear the hook. The hook is process-global: callers must not share it
 * between threads. */
typedef bool (*EvalInterruptCheck)(void *user_data);
void eval_set_interrupt_check(EvalInterruptCheck check, void *user_data);
bool eval_interrupt_requested(void);

#endif

#include "jsrt/runtime.h"

#include <stdio.h>
#include <stdlib.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/memory.h"
#include "jsrt/parser.h"
#include "jsrt/value.h"

struct JsrtRuntime {
    int reserved;
};

const char *jsrt_runtime_version(void) {
    return "0.1.0";
}

static JsrtRunResult jsrt_result_ok_from_value(Value value) {
    JsrtRunResult result;
    result.ok = true;
    result.has_value = value.type != VALUE_NULL;
    result.value_text = result.has_value ? value_to_string(value) : NULL;
    result.error_message = NULL;
    result.runtime_error_kind = RUNTIME_ERROR_NONE;
    return result;
}

static JsrtRunResult jsrt_result_error(char *message, RuntimeErrorKind kind) {
    JsrtRunResult result;
    result.ok = false;
    result.has_value = false;
    result.value_text = NULL;
    result.error_message = message;
    result.runtime_error_kind = kind;
    return result;
}

static char *read_runtime_file(const char *path) {
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

JsrtRuntime *jsrt_runtime_create(void) {
    JsrtRuntime *runtime = jsrt_malloc(sizeof(*runtime));
    runtime->reserved = 0;
    return runtime;
}

void jsrt_runtime_destroy(JsrtRuntime *runtime) {
    free(runtime);
    gc_collect_with_reason(NULL, NULL, "runtime-destroy");
}

JsrtRunResult jsrt_runtime_eval_source(JsrtRuntime *runtime, const char *source, const char *module_path) {
    TokenArray tokens;
    StmtArray program;
    Env *env;
    EvalResult eval_result;
    JsrtRunResult run_result;
    char *error = NULL;
    bool lexed = false;
    bool parsed = false;

    (void)runtime;
    token_array_init(&tokens);
    stmt_array_init(&program);

    if (!lex_source(source, &tokens, &error)) {
        return jsrt_result_error(error, RUNTIME_ERROR_NONE);
    }
    lexed = true;

    if (!parse_tokens(&tokens, source, &program, &error)) {
        if (lexed) {
            token_array_free(&tokens);
        }
        return jsrt_result_error(error, RUNTIME_ERROR_NONE);
    }
    parsed = true;

    env = env_create(NULL);
    eval_result = eval_program_with_path(&program, source, env, module_path);
    env_destroy(env);

    if (eval_result.had_error) {
        run_result = jsrt_result_error(eval_result.error_message, eval_result.runtime_error_kind);
        eval_result.error_message = NULL;
    } else {
        run_result = jsrt_result_ok_from_value(eval_result.value);
    }

    value_free(eval_result.value);
    eval_result_free(&eval_result);
    gc_collect_with_reason(NULL, NULL, "runtime-eval-cleanup");
    if (parsed) {
        stmt_array_free(&program);
    }
    if (lexed) {
        token_array_free(&tokens);
    }
    return run_result;
}

JsrtRunResult jsrt_runtime_eval_file(JsrtRuntime *runtime, const char *path) {
    char *source = read_runtime_file(path);
    JsrtRunResult result;

    if (source == NULL) {
        return jsrt_result_error(jsrt_format("Unable to read file: %s", path), RUNTIME_ERROR_LOOKUP);
    }
    result = jsrt_runtime_eval_source(runtime, source, path);
    free(source);
    return result;
}

void jsrt_run_result_free(JsrtRunResult *result) {
    if (result == NULL) {
        return;
    }
    free(result->value_text);
    free(result->error_message);
    result->value_text = NULL;
    result->error_message = NULL;
    result->ok = false;
    result->has_value = false;
    result->runtime_error_kind = RUNTIME_ERROR_NONE;
}

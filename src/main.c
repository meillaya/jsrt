#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/eval.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/memory.h"
#include "jsrt/parser.h"
#include "jsrt/value.h"

static char *read_file(const char *path) {
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

static bool env_flag_enabled(const char *value) {
    return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0 && strcmp(value, "false") != 0 &&
        strcmp(value, "off") != 0 && strcmp(value, "no") != 0;
}

int main(int argc, char **argv) {
    char *source;
    TokenArray tokens;
    StmtArray program;
    char *error = NULL;
    Env *env = NULL;
    EvalResult result = {false, false, false, false, {VALUE_NULL, {0}}, RUNTIME_ERROR_NONE, 0, 0, 0, NULL};
    int exit_code = 0;
    bool lexed = false;
    bool parsed = false;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <script.js>\n", argv[0]);
        return 1;
    }

    if (env_flag_enabled(getenv("JSRT_GC_TRACE"))) {
        gc_set_trace_enabled(true);
    }

    source = read_file(argv[1]);
    if (source == NULL) {
        fprintf(stderr, "Unable to read file: %s\n", argv[1]);
        return 1;
    }

    if (!lex_source(source, &tokens, &error)) {
        fprintf(stderr, "%s\n", error);
        exit_code = 1;
        goto cleanup;
    }
    lexed = true;

    if (!parse_tokens(&tokens, source, &program, &error)) {
        fprintf(stderr, "%s\n", error);
        exit_code = 1;
        goto cleanup;
    }
    parsed = true;

    env = env_create(NULL);
    result = eval_program(&program, source, env);
    if (result.had_error) {
        fprintf(stderr, "%s\n", result.error_message);
        exit_code = 1;
        goto cleanup;
    }

    if (result.value.type != VALUE_NULL) {
        char *text = value_to_string(result.value);
        printf("%s\n", text);
        free(text);
    }

cleanup:
    value_free(result.value);
    eval_result_free(&result);
    if (env != NULL) {
        env_destroy(env);
    }
    gc_collect_with_reason(NULL, NULL, "process-cleanup");
    if (parsed) {
        stmt_array_free(&program);
    }
    if (lexed) {
        token_array_free(&tokens);
    }
    free(source);
    free(error);
    return exit_code;
}

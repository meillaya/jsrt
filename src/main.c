#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "jsrt/ast.h"
#include "jsrt/bytecode.h"
#include "jsrt/compiler.h"
#include "jsrt/env.h"
#include "jsrt/eval.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/memory.h"
#include "jsrt/parser.h"
#include "jsrt/runtime.h"
#include "jsrt/value.h"
#include "jsrt/vm.h"

typedef struct {
    const char *script_path;
    const char *eval_source;
    bool backend_vm;
} CliOptions;

static void print_usage(FILE *stream, const char *program) {
    fprintf(
        stream,
        "Usage: %s [options] [file]\n"
        "\n"
        "Run a JavaScript-like script from a file, from -e, or from standard input.\n"
        "With no file, no -e, and a terminal on standard input, %s starts a REPL.\n"
        "\n"
        "Options:\n"
        "  -e, --eval CODE   evaluate CODE\n"
        "  --vm              run on the bytecode VM backend\n"
        "  -h, --help        print this help and exit\n"
        "  -V, --version     print the version and exit\n"
        "\n"
        "Environment:\n"
        "  JSRT_BACKEND=vm   same as --vm\n"
        "  JSRT_GC_TRACE=1   write collector traces to standard error\n",
        program,
        program
    );
}

static bool env_flag_enabled(const char *value) {
    return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0 && strcmp(value, "false") != 0 &&
        strcmp(value, "off") != 0 && strcmp(value, "no") != 0;
}

static char *read_stream(FILE *stream) {
    size_t capacity = 4096U;
    size_t length = 0;
    char *buffer = jsrt_malloc(capacity);

    for (;;) {
        size_t space = capacity - length;
        size_t read_count;

        if (space == 0U) {
            capacity *= 2U;
            buffer = jsrt_realloc(buffer, capacity);
            continue;
        }
        read_count = fread(buffer + length, 1U, space, stream);
        length += read_count;
        if (read_count < space) {
            if (ferror(stream)) {
                free(buffer);
                return NULL;
            }
            break;
        }
    }
    buffer = jsrt_realloc(buffer, length + 1U);
    buffer[length] = '\0';
    return buffer;
}

static char *read_script_file(const char *path) {
    FILE *file = fopen(path, "rb");
    char *source;

    if (file == NULL) {
        fprintf(stderr, "jsrt: cannot read '%s': %s\n", path, strerror(errno));
        return NULL;
    }
    source = read_stream(file);
    fclose(file);
    if (source == NULL) {
        fprintf(stderr, "jsrt: cannot read '%s': %s\n", path, strerror(errno));
    }
    return source;
}

static bool input_is_balanced(const char *text) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    bool line_comment = false;
    bool block_comment = false;
    char quote = '\0';
    const char *cursor;

    for (cursor = text; *cursor != '\0'; ++cursor) {
        char current = *cursor;

        if (line_comment) {
            if (current == '\n') {
                line_comment = false;
            }
            continue;
        }
        if (block_comment) {
            if (current == '*' && cursor[1] == '/') {
                block_comment = false;
                cursor += 1;
            }
            continue;
        }
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (current == '\\') {
                escaped = true;
            } else if (current == quote) {
                in_string = false;
            }
            continue;
        }
        if (current == '/' && cursor[1] == '/') {
            line_comment = true;
            cursor += 1;
            continue;
        }
        if (current == '/' && cursor[1] == '*') {
            block_comment = true;
            cursor += 1;
            continue;
        }
        if (current == '"' || current == '\'') {
            in_string = true;
            quote = current;
            continue;
        }
        if (current == '{' || current == '(' || current == '[') {
            depth += 1;
        } else if (current == '}' || current == ')' || current == ']') {
            depth -= 1;
        }
    }
    return depth <= 0 && !in_string && !block_comment;
}

static bool is_repl_exit_command(const char *text) {
    static const char *commands[] = {"quit", "exit", ".quit", ".exit", ":quit", ":exit"};
    size_t index;

    while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r') {
        text += 1;
    }
    for (index = 0; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        size_t length = strlen(commands[index]);
        if (strncmp(text, commands[index], length) == 0) {
            const char *rest = text + length;
            while (*rest == ' ' || *rest == '\t' || *rest == '\n' || *rest == '\r' || *rest == ';') {
                rest += 1;
            }
            if (*rest == '\0') {
                return true;
            }
        }
    }
    return false;
}

static int execute_source(const char *source, const char *module_path, bool backend_vm, Env *env) {
    TokenArray tokens;
    StmtArray program;
    BytecodeProgram bytecode;
    EvalResult result;
    char *error = NULL;
    bool lexed = false;
    bool parsed = false;
    bool backend_ready = false;
    int exit_code = 0;

    token_array_init(&tokens);
    stmt_array_init(&program);

    if (!lex_source(source, &tokens, &error)) {
        fprintf(stderr, "%s\n", error);
        free(error);
        token_array_free(&tokens);
        return 1;
    }
    lexed = true;

    if (!parse_tokens(&tokens, source, &program, &error)) {
        fprintf(stderr, "%s\n", error);
        free(error);
        token_array_free(&tokens);
        return 1;
    }
    parsed = true;

    if (backend_vm) {
        if (!compile_program(&program, source, &bytecode, &error)) {
            fprintf(stderr, "%s\n", error);
            free(error);
            stmt_array_free(&program);
            token_array_free(&tokens);
            return 1;
        }
        backend_ready = true;
        result = vm_execute_program(&bytecode, source);
    } else {
        result = eval_program_with_path(&program, source, env, module_path);
    }

    if (result.had_error) {
        fprintf(stderr, "%s\n", result.error_message);
        exit_code = 1;
    } else if (result.value.type != VALUE_NULL) {
        char *text = value_to_string(result.value);
        printf("%s\n", text);
        free(text);
    }

    value_free(result.value);
    eval_result_free(&result);
    if (backend_ready) {
        bytecode_program_free(&bytecode);
    }
    if (parsed) {
        stmt_array_free(&program);
    }
    if (lexed) {
        token_array_free(&tokens);
    }
    return exit_code;
}

static int run_repl(bool backend_vm) {
    Env *env = env_create(NULL);
    char *line = NULL;
    size_t line_capacity = 0;
    char *pending = NULL;
    size_t pending_length = 0;
    size_t pending_capacity = 0;

    for (;;) {
        ssize_t line_length;

        fputs(pending_length == 0U ? "jsrt> " : "  ... ", stdout);
        fflush(stdout);
        line_length = getline(&line, &line_capacity, stdin);
        if (line_length < 0) {
            putchar('\n');
            break;
        }
        if (pending_length + (size_t)line_length + 1U > pending_capacity) {
            pending_capacity = (pending_length + (size_t)line_length + 1U) * 2U;
            pending = jsrt_realloc(pending, pending_capacity);
        }
        memcpy(pending + pending_length, line, (size_t)line_length);
        pending_length += (size_t)line_length;
        pending[pending_length] = '\0';

        if (!input_is_balanced(pending)) {
            continue;
        }
        if (is_repl_exit_command(pending)) {
            break;
        }
        if (pending_length > 0U) {
            execute_source(pending, "<repl>", backend_vm, env);
        }
        pending_length = 0;
        if (pending != NULL) {
            pending[0] = '\0';
        }
    }

    free(line);
    free(pending);
    env_destroy(env);
    gc_collect_with_reason(NULL, NULL, "process-cleanup");
    return 0;
}

static int parse_options(int argc, char **argv, CliOptions *options) {
    int index;

    options->script_path = NULL;
    options->eval_source = NULL;
    options->backend_vm = false;
    if (getenv("JSRT_BACKEND") != NULL && strcmp(getenv("JSRT_BACKEND"), "vm") == 0) {
        options->backend_vm = true;
    }

    for (index = 1; index < argc; ++index) {
        const char *argument = argv[index];

        if (strcmp(argument, "--") == 0) {
            index += 1;
            if (index < argc && options->script_path != NULL) {
                fprintf(stderr, "jsrt: unexpected argument '%s'\n", argv[index]);
                return 2;
            }
            if (index < argc) {
                options->script_path = argv[index];
            }
            if (index + 1 < argc) {
                fprintf(stderr, "jsrt: unexpected argument '%s'\n", argv[index + 1]);
                return 2;
            }
            return 0;
        }
        if (strcmp(argument, "-h") == 0 || strcmp(argument, "--help") == 0) {
            print_usage(stdout, argv[0]);
            return 1;
        }
        if (strcmp(argument, "-V") == 0 || strcmp(argument, "--version") == 0) {
            printf("jsrt %s\n", jsrt_runtime_version());
            return 1;
        }
        if (strcmp(argument, "--vm") == 0) {
            options->backend_vm = true;
            continue;
        }
        if (strcmp(argument, "-e") == 0 || strcmp(argument, "--eval") == 0) {
            index += 1;
            if (index >= argc) {
                fprintf(stderr, "jsrt: option '%s' requires a CODE argument\n", argument);
                return 2;
            }
            options->eval_source = argv[index];
            continue;
        }
        if (strncmp(argument, "--eval=", 7) == 0) {
            options->eval_source = argument + 7;
            continue;
        }
        if (argument[0] == '-' && argument[1] != '\0') {
            fprintf(stderr, "jsrt: unknown option '%s'\n\n", argument);
            print_usage(stderr, argv[0]);
            return 2;
        }
        if (options->script_path != NULL) {
            fprintf(stderr, "jsrt: unexpected argument '%s'\n", argument);
            return 2;
        }
        options->script_path = argument;
    }

    if (options->eval_source != NULL && options->script_path != NULL) {
        fprintf(stderr, "jsrt: -e cannot be combined with a script file\n");
        return 2;
    }
    return 0;
}

int main(int argc, char **argv) {
    CliOptions options;
    char *source = NULL;
    const char *module_path = "<stdin>";
    Env *env;
    int option_result;
    int exit_code;

    option_result = parse_options(argc, argv, &options);
    if (option_result == 1) {
        return 0;
    }
    if (option_result != 0) {
        return option_result;
    }

    if (env_flag_enabled(getenv("JSRT_GC_TRACE"))) {
        gc_set_trace_enabled(true);
    }

    if (options.eval_source != NULL) {
        source = jsrt_strdup(options.eval_source);
        module_path = "<eval>";
    } else if (options.script_path != NULL) {
        module_path = options.script_path;
        if (strcmp(options.script_path, "-") == 0) {
            source = read_stream(stdin);
        } else {
            source = read_script_file(options.script_path);
            if (source == NULL) {
                return 1;
            }
        }
    } else if (isatty(STDIN_FILENO)) {
        if (options.backend_vm) {
            fprintf(stderr, "jsrt: the REPL uses the tree-walk backend; --vm applies to file and -e runs\n");
        }
        return run_repl(false);
    } else {
        source = read_stream(stdin);
    }

    if (source == NULL) {
        fprintf(stderr, "jsrt: cannot read script from standard input\n");
        return 1;
    }

    env = env_create(NULL);
    exit_code = execute_source(source, module_path, options.backend_vm, env);
    env_destroy(env);
    gc_collect_with_reason(NULL, NULL, "process-cleanup");
    free(source);
    return exit_code;
}

#include <stdlib.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/eval.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/parser.h"
#include "test.h"

static char *lex_error_for(const char *source) {
    TokenArray tokens;
    char *error = NULL;

    if (lex_source(source, &tokens, &error)) {
        token_array_free(&tokens);
        return NULL;
    }

    return error;
}

static char *parse_error_for(const char *source) {
    TokenArray tokens;
    StmtArray program;
    char *error = NULL;

    stmt_array_init(&program);
    if (!lex_source(source, &tokens, &error)) {
        return error;
    }
    if (parse_tokens(&tokens, source, &program, &error)) {
        stmt_array_free(&program);
        token_array_free(&tokens);
        return NULL;
    }

    token_array_free(&tokens);
    return error;
}

static EvalResult eval_result_for(const char *source) {
    TokenArray tokens;
    StmtArray program;
    char *error = NULL;
    Env *env;
    EvalResult result;

    if (!lex_source(source, &tokens, &error)) {
        EvalResult failure;
        failure.had_error = true;
        failure.is_return = false;
        failure.is_break = false;
        failure.is_continue = false;
        failure.value = value_null();
        failure.error_message = error;
        return failure;
    }

    if (!parse_tokens(&tokens, source, &program, &error)) {
        EvalResult failure;
        token_array_free(&tokens);
        failure.had_error = true;
        failure.is_return = false;
        failure.is_break = false;
        failure.is_continue = false;
        failure.value = value_null();
        failure.error_message = error;
        return failure;
    }

    env = env_create(NULL);
    result = eval_program(&program, source, env);
    env_destroy(env);
    gc_collect_with_reason(NULL, result.had_error ? NULL : &result.value, "test-result-cleanup");
    stmt_array_free(&program);
    token_array_free(&tokens);
    return result;
}

void run_diagnostics_tests(TestContext *ctx) {
    char *error = lex_error_for("let x = @;");
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_STRING_EQ(
        ctx,
        error,
        "Lexer error at 1:9: unexpected character '@'\n"
        "let x = @;\n"
        "        ^"
    );
    free(error);

    error = lex_error_for("let s = \"oops");
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_STRING_EQ(
        ctx,
        error,
        "Lexer error at 1:9: unterminated string literal\n"
        "let s = \"oops\n"
        "        ^"
    );
    free(error);

    error = parse_error_for("let x = [1, 2;");
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_STRING_EQ(
        ctx,
        error,
        "Parser error at 1:14: expected ']' after array literal\n"
        "let x = [1, 2;\n"
        "             ^"
    );
    free(error);

    error = parse_error_for("while x { x = x - 1; }");
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_STRING_EQ(
        ctx,
        error,
        "Parser error at 1:7: expected '(' after 'while'\n"
        "while x { x = x - 1; }\n"
        "      ^"
    );
    free(error);

    error = parse_error_for(
        "let broken = [1, 2;\n"
        "while nope { let x = 1; }\n"
        "let ok = 3;\n"
    );
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_STRING_EQ(
        ctx,
        error,
        "Parser error at 1:19: expected ']' after array literal\n"
        "let broken = [1, 2;\n"
        "                  ^\n\n"
        "Parser error at 2:7: expected '(' after 'while'\n"
        "while nope { let x = 1; }\n"
        "      ^^^^\n\n"
        "Parser error at 2:25: expected expression\n"
        "while nope { let x = 1; }\n"
        "                        ^"
    );
    free(error);

    {
        EvalResult result = eval_result_for("let arr = [1];\narr[3];\n");
        EXPECT_TRUE(ctx, result.had_error);
        EXPECT_STRING_EQ(
            ctx,
            result.error_message,
            "Runtime error at 2:4: array index out of bounds\n"
            "arr[3];\n"
            "   ^"
        );
        eval_result_free(&result);
    }

    {
        EvalResult result = eval_result_for("continue;\n");
        EXPECT_TRUE(ctx, result.had_error);
        EXPECT_STRING_EQ(
            ctx,
            result.error_message,
            "Runtime error at 1:1: continue is only valid inside a loop\n"
            "continue;\n"
            "^^^^^^^^"
        );
        eval_result_free(&result);
    }
}

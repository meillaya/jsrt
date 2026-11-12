#include <stdlib.h>
#include <string.h>

#include "jsrt/lexer.h"
#include "test.h"

void run_lexer_tests(TestContext *ctx) {
    TokenArray tokens;
    char *error = NULL;
    EXPECT_TRUE(ctx, lex_source("let x = [42, 7] && true;\nwhile (x || false) { x = false; }", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[3].type, TOKEN_LEFT_BRACKET);
    EXPECT_INT_EQ(ctx, tokens.items[5].type, TOKEN_COMMA);
    EXPECT_INT_EQ(ctx, tokens.items[7].type, TOKEN_RIGHT_BRACKET);
    EXPECT_INT_EQ(ctx, tokens.items[8].type, TOKEN_AND_AND);
    EXPECT_INT_EQ(ctx, tokens.items[10].type, TOKEN_SEMICOLON);
    EXPECT_INT_EQ(ctx, tokens.items[11].type, TOKEN_WHILE);
    EXPECT_INT_EQ(ctx, tokens.items[14].type, TOKEN_OR_OR);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("for (;;) { break; continue; }", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_FOR);
    EXPECT_INT_EQ(ctx, tokens.items[6].type, TOKEN_BREAK);
    EXPECT_INT_EQ(ctx, tokens.items[8].type, TOKEN_CONTINUE);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("do { x = x + 1; } while (x < 3);", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_DO);
    EXPECT_INT_EQ(ctx, tokens.items[9].type, TOKEN_WHILE);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("switch (x) { case 1: break; default: x = 0; }", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_SWITCH);
    EXPECT_INT_EQ(ctx, tokens.items[5].type, TOKEN_CASE);
    EXPECT_INT_EQ(ctx, tokens.items[10].type, TOKEN_DEFAULT);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("const x = 3;", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_CONST);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("let greeting = \"hi\\nthere\";", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[3].type, TOKEN_STRING);
    EXPECT_STRING_EQ(ctx, tokens.items[3].lexeme, "hi\nthere");
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("this; new Ctor();", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_THIS);
    EXPECT_INT_EQ(ctx, tokens.items[2].type, TOKEN_NEW);
    EXPECT_INT_EQ(ctx, tokens.items[3].type, TOKEN_IDENTIFIER);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("class Box extends Base { constructor() { super(); } }", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_CLASS);
    EXPECT_INT_EQ(ctx, tokens.items[2].type, TOKEN_EXTENDS);
    EXPECT_INT_EQ(ctx, tokens.items[9].type, TOKEN_SUPER);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("import { value as local } from \"./m.js\"; export const out = local;", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_IMPORT);
    EXPECT_INT_EQ(ctx, tokens.items[3].type, TOKEN_AS);
    EXPECT_INT_EQ(ctx, tokens.items[6].type, TOKEN_FROM);
    EXPECT_INT_EQ(ctx, tokens.items[9].type, TOKEN_EXPORT);
    token_array_free(&tokens);

    EXPECT_FALSE(ctx, lex_source("let x = @;", &tokens, &error));
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_TRUE(ctx, strstr(error, "^") != NULL);
    if (error != NULL) {
        free(error);
    }

    EXPECT_FALSE(ctx, lex_source("let s = \"oops", &tokens, &error));
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_TRUE(ctx, strstr(error, "unterminated string literal") != NULL);
    EXPECT_TRUE(ctx, strstr(error, "^") != NULL);
    if (error != NULL) {
        free(error);
    }

    EXPECT_TRUE(ctx, lex_source("/* one */ let x = 1; /* a\nmulti-line comment */ x;", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_LET);
    EXPECT_INT_EQ(ctx, tokens.items[1].line, 1);
    token_array_free(&tokens);

    EXPECT_FALSE(ctx, lex_source("let x = 1; /* oops", &tokens, &error));
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_TRUE(ctx, strstr(error, "unterminated block comment") != NULL);
    if (error != NULL) {
        free(error);
    }

    {
        const char *source =
            "let a = \"line\\rbreak\";\n"
            "let b = \"a\\bb\";\n"
            "let c = \"\\u0041\\x42\";\n"
            "let d = 'single';\n";

        EXPECT_TRUE(ctx, lex_source(source, &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_STRING_EQ(ctx, tokens.items[3].lexeme, "line\rbreak");
        EXPECT_STRING_EQ(ctx, tokens.items[8].lexeme, "a\bb");
        EXPECT_STRING_EQ(ctx, tokens.items[13].lexeme, "AB");
        EXPECT_STRING_EQ(ctx, tokens.items[18].lexeme, "single");
        token_array_free(&tokens);
    }

    EXPECT_FALSE(ctx, lex_source("let x = \"\\xZZ\";", &tokens, &error));
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_TRUE(ctx, strstr(error, "invalid hexadecimal escape sequence") != NULL);
    if (error != NULL) {
        free(error);
    }

    EXPECT_FALSE(ctx, lex_source("let x = \"\\uD800\";", &tokens, &error));
    EXPECT_TRUE(ctx, error != NULL);
    EXPECT_TRUE(ctx, strstr(error, "surrogate escape sequences are unsupported") != NULL);
    if (error != NULL) {
        free(error);
    }
}

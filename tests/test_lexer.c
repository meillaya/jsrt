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

    EXPECT_TRUE(ctx, lex_source("this; new Thing();", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[0].type, TOKEN_THIS);
    EXPECT_INT_EQ(ctx, tokens.items[2].type, TOKEN_NEW);
    token_array_free(&tokens);

    EXPECT_TRUE(ctx, lex_source("let greeting = \"hi\\nthere\";", &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_INT_EQ(ctx, tokens.items[3].type, TOKEN_STRING);
    EXPECT_STRING_EQ(ctx, tokens.items[3].lexeme, "hi\nthere");
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
}

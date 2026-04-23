#include <stdlib.h>
#include <string.h>

#include "jsrt/lexer.h"
#include "jsrt/parser.h"
#include "test.h"

static StmtArray parse_program_or_fail(TestContext *ctx, const char *source) {
    TokenArray tokens;
    StmtArray program;
    char *error = NULL;
    stmt_array_init(&program);
    EXPECT_TRUE(ctx, lex_source(source, &tokens, &error));
    if (error != NULL) {
        free(error);
    }
    EXPECT_TRUE(ctx, parse_tokens(&tokens, source, &program, &error));
    if (error != NULL) {
        free(error);
    }
    token_array_free(&tokens);
    return program;
}

void run_parser_tests(TestContext *ctx) {
    StmtArray program = parse_program_or_fail(ctx,
        "let values = [1, 2, 3];\n"
        "let meta = {name: \"jsrt\", third: values[2]};\n"
        "let remaining = meta.third;\n"
        "while (remaining > 0 && values[0] < 10) { remaining = remaining - 1; }\n"
        "meta.name || \"fallback\";\n"
    );

    EXPECT_SIZE_EQ(ctx, program.count, 5U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_VAR);
    EXPECT_INT_EQ(ctx, program.items[0]->as.var.initializer->kind, EXPR_ARRAY);
    EXPECT_SIZE_EQ(ctx, program.items[0]->as.var.initializer->as.array.elements.count, 3U);
    EXPECT_INT_EQ(ctx, program.items[1]->as.var.initializer->kind, EXPR_OBJECT);
    EXPECT_SIZE_EQ(ctx, program.items[1]->as.var.initializer->as.object.properties.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[2]->kind, STMT_VAR);
    EXPECT_INT_EQ(ctx, program.items[3]->kind, STMT_WHILE);
    EXPECT_INT_EQ(ctx, program.items[3]->as.while_stmt.condition->kind, EXPR_LOGICAL);
    EXPECT_INT_EQ(ctx, program.items[4]->as.expr.expression->kind, EXPR_LOGICAL);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "let arr = [1, 2];\n"
        "let obj = {nested: {value: 1}};\n"
        "arr[1] = 7;\n"
        "obj.nested.value = arr[1];\n"
        "obj[\"name\"] = \"jsrt\";\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 5U);
    EXPECT_INT_EQ(ctx, program.items[2]->kind, STMT_EXPR);
    EXPECT_INT_EQ(ctx, program.items[2]->as.expr.expression->kind, EXPR_ASSIGN);
    EXPECT_INT_EQ(ctx, program.items[2]->as.expr.expression->as.assign.target->kind, EXPR_INDEX);
    EXPECT_INT_EQ(ctx, program.items[3]->as.expr.expression->kind, EXPR_ASSIGN);
    EXPECT_INT_EQ(ctx, program.items[3]->as.expr.expression->as.assign.target->kind, EXPR_PROPERTY);
    EXPECT_INT_EQ(ctx, program.items[4]->as.expr.expression->kind, EXPR_ASSIGN);
    EXPECT_INT_EQ(ctx, program.items[4]->as.expr.expression->as.assign.target->kind, EXPR_INDEX);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "for (let i = 0; i < 3; i = i + 1) { continue; }\n"
        "for (; ; ) break;\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_FOR);
    EXPECT_TRUE(ctx, program.items[0]->as.for_stmt.init != NULL);
    EXPECT_TRUE(ctx, program.items[0]->as.for_stmt.condition != NULL);
    EXPECT_TRUE(ctx, program.items[0]->as.for_stmt.increment != NULL);
    EXPECT_INT_EQ(ctx, program.items[0]->as.for_stmt.body->kind, STMT_BLOCK);
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_FOR);
    EXPECT_TRUE(ctx, program.items[1]->as.for_stmt.init == NULL);
    EXPECT_TRUE(ctx, program.items[1]->as.for_stmt.condition == NULL);
    EXPECT_TRUE(ctx, program.items[1]->as.for_stmt.increment == NULL);
    EXPECT_INT_EQ(ctx, program.items[1]->as.for_stmt.body->kind, STMT_BREAK);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "do { let x = 1; } while (false);\n"
        "do break; while (true);\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_DO_WHILE);
    EXPECT_INT_EQ(ctx, program.items[0]->as.do_while_stmt.body->kind, STMT_BLOCK);
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_DO_WHILE);
    EXPECT_INT_EQ(ctx, program.items[1]->as.do_while_stmt.body->kind, STMT_BREAK);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "switch (x) {\n"
        "  case 1:\n"
        "    let y = 2;\n"
        "    break;\n"
        "  default:\n"
        "    y = 3;\n"
        "}\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 1U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_SWITCH);
    EXPECT_SIZE_EQ(ctx, program.items[0]->as.switch_stmt.cases.count, 2U);
    EXPECT_FALSE(ctx, program.items[0]->as.switch_stmt.cases.items[0].is_default);
    EXPECT_TRUE(ctx, program.items[0]->as.switch_stmt.cases.items[1].is_default);
    EXPECT_SIZE_EQ(ctx, program.items[0]->as.switch_stmt.cases.items[0].statements.count, 2U);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "let add = function (a, b) { return a + b; };\n"
        "let named = function inner(n) { return n; };\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_VAR);
    EXPECT_INT_EQ(ctx, program.items[0]->as.var.initializer->kind, EXPR_FUNCTION);
    EXPECT_TRUE(ctx, program.items[0]->as.var.initializer->as.function.name == NULL);
    EXPECT_SIZE_EQ(ctx, program.items[0]->as.var.initializer->as.function.params.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_VAR);
    EXPECT_INT_EQ(ctx, program.items[1]->as.var.initializer->kind, EXPR_FUNCTION);
    EXPECT_STRING_EQ(ctx, program.items[1]->as.var.initializer->as.function.name, "inner");
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "this;\n"
        "new Thing(1, 2);\n"
        "new registry.builders[0](seed);\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 3U);
    EXPECT_INT_EQ(ctx, program.items[0]->as.expr.expression->kind, EXPR_THIS);
    EXPECT_INT_EQ(ctx, program.items[1]->as.expr.expression->kind, EXPR_NEW);
    EXPECT_INT_EQ(ctx, program.items[1]->as.expr.expression->as.new_expr.callee->kind, EXPR_IDENTIFIER);
    EXPECT_SIZE_EQ(ctx, program.items[1]->as.expr.expression->as.new_expr.arguments.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[2]->as.expr.expression->kind, EXPR_NEW);
    EXPECT_INT_EQ(ctx, program.items[2]->as.expr.expression->as.new_expr.callee->kind, EXPR_INDEX);
    EXPECT_SIZE_EQ(ctx, program.items[2]->as.expr.expression->as.new_expr.arguments.count, 1U);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "const x = 3;\n"
        "{ const x = 4; }\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_VAR);
    EXPECT_TRUE(ctx, program.items[0]->as.var.is_const);
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_BLOCK);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "function Box(value) { this.value = value; }\n"
        "new Box(7);\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_FUNCTION);
    EXPECT_INT_EQ(ctx, program.items[0]->as.function.body.items[0]->kind, STMT_EXPR);
    EXPECT_INT_EQ(ctx, program.items[0]->as.function.body.items[0]->as.expr.expression->kind, EXPR_ASSIGN);
    EXPECT_INT_EQ(
        ctx,
        program.items[0]->as.function.body.items[0]->as.expr.expression->as.assign.target->as.property.target->kind,
        EXPR_THIS
    );
    EXPECT_INT_EQ(ctx, program.items[1]->as.expr.expression->kind, EXPR_NEW);
    EXPECT_INT_EQ(ctx, program.items[1]->as.expr.expression->as.new_expr.callee->kind, EXPR_IDENTIFIER);
    EXPECT_SIZE_EQ(ctx, program.items[1]->as.expr.expression->as.new_expr.arguments.count, 1U);
    stmt_array_free(&program);

    {
        TokenArray tokens;
        char *error = NULL;
        EXPECT_TRUE(ctx, lex_source("let x = [1, 2;", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "let x = [1, 2;", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "^") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("let obj = {name 1};", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "let obj = {name 1};", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("while x { x = x - 1; }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "while x { x = x - 1; }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source(
            "let broken = [1, 2;\n"
            "while nope { let x = 1; }\n"
            "let ok = 3;\n",
            &tokens,
            &error
        ));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(
            &tokens,
            "let broken = [1, 2;\nwhile nope { let x = 1; }\nlet ok = 3;\n",
            &program,
            &error
        ));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected ']' after array literal") != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected '(' after 'while'") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("for (let i = 0 i < 3; i = i + 1) {}", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "for (let i = 0 i < 3; i = i + 1) {}", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected ';' after variable declaration") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("do { let x = 1; } (true);", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "do { let x = 1; } (true);", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected 'while' after do-while body") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("switch (x) { case 1 break; }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "switch (x) { case 1 break; }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected ':' after case expression") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("const x;", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "const x;", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "const declarations require an initializer") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("new Thing;", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "new Thing;", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected '(' after constructor target") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("new factory()(1);", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "new factory()(1);", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "calling the result of 'new' is unsupported in L3") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("new (Thing)();", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "new (Thing)();", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected constructor target after 'new'") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);
    }
}

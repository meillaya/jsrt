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
        "const x = 3;\n"
        "{ const x = 4; }\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_VAR);
    EXPECT_TRUE(ctx, program.items[0]->as.var.is_const);
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_BLOCK);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "this;\n"
        "new Ctor(1);\n"
        "new obj[\"Ctor\"](2);\n"
        "this.value = 3;\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 4U);
    EXPECT_INT_EQ(ctx, program.items[0]->as.expr.expression->kind, EXPR_THIS);
    EXPECT_INT_EQ(ctx, program.items[1]->as.expr.expression->kind, EXPR_NEW);
    EXPECT_INT_EQ(ctx, program.items[1]->as.expr.expression->as.new_expr.callee->kind, EXPR_IDENTIFIER);
    EXPECT_SIZE_EQ(ctx, program.items[1]->as.expr.expression->as.new_expr.arguments.count, 1U);
    EXPECT_INT_EQ(ctx, program.items[2]->as.expr.expression->kind, EXPR_NEW);
    EXPECT_INT_EQ(ctx, program.items[2]->as.expr.expression->as.new_expr.callee->kind, EXPR_INDEX);
    EXPECT_INT_EQ(ctx, program.items[3]->as.expr.expression->kind, EXPR_ASSIGN);
    EXPECT_INT_EQ(ctx, program.items[3]->as.expr.expression->as.assign.target->kind, EXPR_PROPERTY);
    EXPECT_INT_EQ(ctx, program.items[3]->as.expr.expression->as.assign.target->as.property.target->kind, EXPR_THIS);
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "class Box {\n"
        "  constructor(v) { this.value = v; }\n"
        "  get() { return this.value; }\n"
        "  set() { return 1; }\n"
        "}\n"
        "class Empty { value() { return 1; } }\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 2U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_CLASS);
    EXPECT_STRING_EQ(ctx, program.items[0]->as.class_stmt.name, "Box");
    EXPECT_SIZE_EQ(ctx, program.items[0]->as.class_stmt.methods.count, 3U);
    EXPECT_TRUE(ctx, program.items[0]->as.class_stmt.methods.items[0].is_constructor);
    EXPECT_STRING_EQ(ctx, program.items[0]->as.class_stmt.methods.items[1].name, "get");
    EXPECT_FALSE(ctx, program.items[0]->as.class_stmt.methods.items[1].is_constructor);
    EXPECT_STRING_EQ(ctx, program.items[0]->as.class_stmt.methods.items[2].name, "set");
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_CLASS);
    EXPECT_SIZE_EQ(ctx, program.items[1]->as.class_stmt.methods.count, 1U);
    EXPECT_FALSE(ctx, program.items[1]->as.class_stmt.methods.items[0].is_constructor);
    EXPECT_STRING_EQ(ctx, program.items[1]->as.class_stmt.methods.items[0].name, "value");
    stmt_array_free(&program);

    program = parse_program_or_fail(ctx,
        "import { value, renamed as localName } from \"./module.js\";\n"
        "export const answer = value;\n"
        "export function read() { return localName; }\n"
        "export class Thing { get() { return answer; } }\n"
    );
    EXPECT_SIZE_EQ(ctx, program.count, 4U);
    EXPECT_INT_EQ(ctx, program.items[0]->kind, STMT_IMPORT);
    EXPECT_SIZE_EQ(ctx, program.items[0]->as.import_stmt.specifiers.count, 2U);
    EXPECT_STRING_EQ(ctx, program.items[0]->as.import_stmt.specifiers.items[0].imported_name, "value");
    EXPECT_STRING_EQ(ctx, program.items[0]->as.import_stmt.specifiers.items[0].local_name, "value");
    EXPECT_STRING_EQ(ctx, program.items[0]->as.import_stmt.specifiers.items[1].imported_name, "renamed");
    EXPECT_STRING_EQ(ctx, program.items[0]->as.import_stmt.specifiers.items[1].local_name, "localName");
    EXPECT_STRING_EQ(ctx, program.items[0]->as.import_stmt.module_path, "./module.js");
    EXPECT_INT_EQ(ctx, program.items[1]->kind, STMT_EXPORT);
    EXPECT_INT_EQ(ctx, program.items[1]->as.export_stmt.declaration->kind, STMT_VAR);
    EXPECT_INT_EQ(ctx, program.items[2]->kind, STMT_EXPORT);
    EXPECT_INT_EQ(ctx, program.items[2]->as.export_stmt.declaration->kind, STMT_FUNCTION);
    EXPECT_INT_EQ(ctx, program.items[3]->kind, STMT_EXPORT);
    EXPECT_INT_EQ(ctx, program.items[3]->as.export_stmt.declaration->kind, STMT_CLASS);
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

        EXPECT_TRUE(ctx, lex_source("new Ctor;", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "new Ctor;", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected '(' after constructor in 'new' expression") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("new Ctor()(1);", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "new Ctor()(1);", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "postfix chaining after 'new' is unsupported for constructor calls") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("new this();", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "new this();", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected constructor target after 'new'") != NULL);
        if (error != NULL) {
            free(error);
        }
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("class Box extends Base {}", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "class Box extends Base {}", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "extends is unsupported in jsrt class declarations") != NULL);
        free(error);
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("class Box { constructor() {} constructor(v) {} }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "class Box { constructor() {} constructor(v) {} }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "duplicate constructor definitions are unsupported") != NULL);
        free(error);
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("class Box { static foo() {} }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "class Box { static foo() {} }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "static class members are unsupported in jsrt class declarations") != NULL);
        free(error);
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("class Box { get foo() {} }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "class Box { get foo() {} }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "getter syntax is unsupported in jsrt class declarations") != NULL);
        free(error);
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("class Box { value; }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "class Box { value; }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "class fields are unsupported in jsrt class declarations") != NULL);
        free(error);
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("class Box { [name]() {} }", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "class Box { [name]() {} }", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "computed method names are unsupported in jsrt class declarations") != NULL);
        free(error);
        token_array_free(&tokens);

        EXPECT_TRUE(ctx, lex_source("let Box = class {};", &tokens, &error));
        if (error != NULL) {
            free(error);
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "let Box = class {};", &program, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "class expressions are unsupported in jsrt class declarations") != NULL);
        free(error);
        token_array_free(&tokens);
    }

    {
        TokenArray tokens;
        StmtArray nested;
        char *error = NULL;

        /* A final statement needs no terminator at end of file or before a
         * closing brace. Two statements on separate lines still do. */
        program = parse_program_or_fail(ctx, "let x = 1;\nx");
        EXPECT_SIZE_EQ(ctx, program.count, 2U);
        stmt_array_free(&program);

        program = parse_program_or_fail(ctx, "function f() { return 1 }\nf();");
        EXPECT_SIZE_EQ(ctx, program.count, 2U);
        stmt_array_free(&program);

        EXPECT_TRUE(ctx, lex_source("let x = 1\nlet y = 2;", &tokens, &error));
        if (error != NULL) {
            free(error);
            error = NULL;
        }
        EXPECT_FALSE(ctx, parse_tokens(&tokens, "let x = 1\nlet y = 2;", &nested, &error));
        EXPECT_TRUE(ctx, error != NULL);
        EXPECT_TRUE(ctx, strstr(error, "expected ';' after variable declaration") != NULL);
        free(error);
        token_array_free(&tokens);
    }

    {
        TokenArray tokens;
        StmtArray incomplete;
        char *error = NULL;
        const char *sources[] = {
            "export default 1;",
            "default;",
            "case 1:",
            "{ default; }",
            "function f() { default; }"
        };
        size_t index;

        /* Error recovery used to stall on tokens that synchronize stops at
         * without consuming, which made these inputs hang instead of fail. */
        for (index = 0; index < sizeof(sources) / sizeof(sources[0]); ++index) {
            EXPECT_TRUE(ctx, lex_source(sources[index], &tokens, &error));
            if (error != NULL) {
                free(error);
                error = NULL;
            }
            EXPECT_FALSE(ctx, parse_tokens(&tokens, sources[index], &incomplete, &error));
            EXPECT_TRUE(ctx, error != NULL);
            free(error);
            error = NULL;
            token_array_free(&tokens);
        }
    }

    {
        TokenArray tokens;
        StmtArray nested;
        char *error = NULL;
        size_t depth = JSRT_MAX_PARSE_DEPTH + 20U;
        char *source = malloc(depth * 2U + 16U);
        size_t index = 0;
        size_t cursor = 0;

        EXPECT_TRUE(ctx, source != NULL);
        if (source != NULL) {
            memcpy(source + cursor, "let x = ", 8U);
            cursor += 8U;
            for (index = 0; index < depth; ++index) {
                source[cursor++] = '(';
            }
            source[cursor++] = '1';
            for (index = 0; index < depth; ++index) {
                source[cursor++] = ')';
            }
            source[cursor++] = ';';
            source[cursor] = '\0';

            EXPECT_TRUE(ctx, lex_source(source, &tokens, &error));
            if (error != NULL) {
                free(error);
                error = NULL;
            }
            EXPECT_FALSE(ctx, parse_tokens(&tokens, source, &nested, &error));
            EXPECT_TRUE(ctx, error != NULL);
            EXPECT_TRUE(ctx, strstr(error, "source nesting is too deep") != NULL);
            free(error);
            token_array_free(&tokens);
            free(source);
        }
    }

    {
        /* Reserved words are legal property names, and trailing commas are
         * allowed in literals and parameter lists. */
        program = parse_program_or_fail(
            ctx,
            "let o = {class: 1, default: 2,};\n"
            "let v = o.class + o.default;\n"
            "let a = [1, 2, 3,];\n"
            "function f(x, y,) { return x + y; }\n"
        );
        EXPECT_SIZE_EQ(ctx, program.count, 4U);
        stmt_array_free(&program);
    }

    {
        size_t count = JSRT_MAX_EXPR_CHAIN + 10U;
        char *source = malloc(count * 2U + 4U);
        size_t index;
        size_t cursor = 0;
        TokenArray tokens;
        StmtArray stmts;
        char *error = NULL;

        EXPECT_TRUE(ctx, source != NULL);
        if (source != NULL) {
            source[cursor++] = '1';
            for (index = 0; index < count; ++index) {
                source[cursor++] = '+';
                source[cursor++] = '1';
            }
            source[cursor] = '\0';

            EXPECT_TRUE(ctx, lex_source(source, &tokens, &error));
            if (error != NULL) {
                free(error);
                error = NULL;
            }
            EXPECT_FALSE(ctx, parse_tokens(&tokens, source, &stmts, &error));
            EXPECT_TRUE(ctx, error != NULL);
            EXPECT_TRUE(ctx, strstr(error, "expression chain is too long") != NULL);
            free(error);
            token_array_free(&tokens);
            free(source);
        }
    }
}

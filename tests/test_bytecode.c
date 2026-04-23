#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/ast.h"
#include "jsrt/compiler.h"
#include "jsrt/lexer.h"
#include "jsrt/parser.h"
#include "test.h"

static bool compile_source_text(const char *source, BytecodeProgram *program, char **error_message) {
    TokenArray tokens;
    StmtArray ast;
    bool ok;

    stmt_array_init(&ast);
    if (!lex_source(source, &tokens, error_message)) {
        return false;
    }
    if (!parse_tokens(&tokens, source, &ast, error_message)) {
        token_array_free(&tokens);
        return false;
    }
    ok = compile_program(&ast, source, program, error_message);
    stmt_array_free(&ast);
    token_array_free(&tokens);
    return ok;
}

static char *read_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long size;
    size_t count;
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
    buffer = malloc((size_t)size + 1U);
    count = fread(buffer, 1U, (size_t)size, file);
    buffer[count] = '\0';
    fclose(file);
    return buffer;
}

void run_bytecode_tests(TestContext *ctx) {
    {
        const char *source =
            "function twice(n) { return n * 2; }\n"
            "let x = 3;\n"
            "if (x < 10) { x = twice(x + 1); } else { x = 0; }\n";
        BytecodeProgram program;
        char *error = NULL;
        char *disassembly;

        EXPECT_TRUE(ctx, compile_source_text(source, &program, &error));
        EXPECT_TRUE(ctx, error == NULL);
        disassembly = bytecode_disassemble_program(&program);
        EXPECT_STRING_EQ(
            ctx,
            disassembly,
            "function <script> arity=0 locals=0\n"
            "0000 MAKE_FUNCTION  1 (twice)\n"
            "0001 DEFINE_GLOBAL  0 (twice)\n"
            "0002 CONST          0 (3)\n"
            "0003 DEFINE_GLOBAL  1 (x)\n"
            "0004 LOAD_GLOBAL    1 (x)\n"
            "0005 CONST          1 (10)\n"
            "0006 LT\n"
            "0007 JUMP_IF_FALSE  18\n"
            "0008 POP\n"
            "0009 LOAD_GLOBAL    0 (twice)\n"
            "0010 LOAD_GLOBAL    1 (x)\n"
            "0011 CONST          2 (1)\n"
            "0012 ADD\n"
            "0013 CALL           1\n"
            "0014 DUP\n"
            "0015 STORE_GLOBAL   1 (x)\n"
            "0016 POP\n"
            "0017 JUMP           23\n"
            "0018 POP\n"
            "0019 CONST          3 (0)\n"
            "0020 DUP\n"
            "0021 STORE_GLOBAL   1 (x)\n"
            "0022 POP\n"
            "0023 NULL\n"
            "0024 RETURN\n"
            "\n"
            "function twice arity=1 locals=1\n"
            "0000 LOAD_LOCAL     0\n"
            "0001 CONST          0 (2)\n"
            "0002 MUL\n"
            "0003 RETURN\n"
            "0004 NULL\n"
            "0005 RETURN\n"
        );
        free(disassembly);
        bytecode_program_free(&program);
    }

    {
        char *source = read_text_file("examples/bytecode_fixture.js");
        BytecodeProgram program;
        char *error = NULL;
        char *disassembly;

        EXPECT_TRUE(ctx, source != NULL);
        if (source != NULL) {
            EXPECT_TRUE(ctx, compile_source_text(source, &program, &error));
            EXPECT_TRUE(ctx, error == NULL);
            disassembly = bytecode_disassemble_program(&program);
            EXPECT_TRUE(ctx, strstr(disassembly, "function <script> arity=0 locals=1") != NULL);
            EXPECT_TRUE(ctx, strstr(disassembly, "MAKE_FUNCTION  1 (bump)") != NULL);
            EXPECT_TRUE(ctx, strstr(disassembly, "CALL           1") != NULL);
            EXPECT_TRUE(ctx, strstr(disassembly, "DEFINE_GLOBAL  1 (value)") != NULL);
            free(disassembly);
            bytecode_program_free(&program);
            free(source);
        }
    }

    {
        const char *source =
            "function outer(n) {\n"
            "  function inner(x) { return x + n; }\n"
            "  return inner(1);\n"
            "}\n";
        BytecodeProgram program;
        char *error = NULL;
        char *disassembly;

        EXPECT_TRUE(ctx, compile_source_text(source, &program, &error));
        EXPECT_TRUE(ctx, error == NULL);
        disassembly = bytecode_disassemble_program(&program);
        EXPECT_TRUE(ctx, strstr(disassembly, "function inner arity=1") != NULL);
        EXPECT_TRUE(ctx, strstr(disassembly, "LOAD_UPVALUE") != NULL);
        free(disassembly);
        bytecode_program_free(&program);
    }

    {
        const char *source = "let f = function (n) { return n; };\n";
        BytecodeProgram program;
        char *error = NULL;
        char *disassembly;

        EXPECT_TRUE(ctx, compile_source_text(source, &program, &error));
        EXPECT_TRUE(ctx, error == NULL);
        disassembly = bytecode_disassemble_program(&program);
        EXPECT_TRUE(ctx, strstr(disassembly, "MAKE_FUNCTION") != NULL);
        free(disassembly);
        bytecode_program_free(&program);
    }

    {
        const char *source =
            "let arr = [1, 2, 3];\n"
            "let obj = {name: \"jsrt\", third: arr[2]};\n"
            "arr[1] = 9;\n"
            "obj.name;\n";
        BytecodeProgram program;
        char *error = NULL;
        char *disassembly;

        EXPECT_TRUE(ctx, compile_source_text(source, &program, &error));
        EXPECT_TRUE(ctx, error == NULL);
        disassembly = bytecode_disassemble_program(&program);
        EXPECT_TRUE(ctx, strstr(disassembly, "MAKE_ARRAY") != NULL);
        EXPECT_TRUE(ctx, strstr(disassembly, "MAKE_OBJECT") != NULL);
        EXPECT_TRUE(ctx, strstr(disassembly, "GET_INDEX") != NULL);
        EXPECT_TRUE(ctx, strstr(disassembly, "SET_INDEX") != NULL);
        free(disassembly);
        bytecode_program_free(&program);
    }

    {
        const char *source =
            "let total = 0;\n"
            "do {\n"
            "  total = total + 1;\n"
            "} while (total < 3);\n"
            "switch (total) {\n"
            "  case 3:\n"
            "    total = total + 4;\n"
            "    break;\n"
            "  default:\n"
            "    total = 0;\n"
            "}\n";
        BytecodeProgram program;
        char *error = NULL;
        char *disassembly;

        EXPECT_TRUE(ctx, compile_source_text(source, &program, &error));
        EXPECT_TRUE(ctx, error == NULL);
        disassembly = bytecode_disassemble_program(&program);
        EXPECT_TRUE(ctx, strstr(disassembly, "LOOP") != NULL);
        EXPECT_TRUE(ctx, strstr(disassembly, "JUMP_IF_FALSE") != NULL);
        EXPECT_TRUE(ctx, strstr(disassembly, "JUMP") != NULL);
        free(disassembly);
        bytecode_program_free(&program);
    }
}

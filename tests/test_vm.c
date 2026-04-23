#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/ast.h"
#include "jsrt/compiler.h"
#include "jsrt/eval.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/parser.h"
#include "jsrt/vm.h"
#include "test.h"

static bool parse_source_text(const char *source, TokenArray *tokens, StmtArray *program, char **error_message) {
    stmt_array_init(program);
    if (!lex_source(source, tokens, error_message)) {
        return false;
    }
    if (!parse_tokens(tokens, source, program, error_message)) {
        token_array_free(tokens);
        return false;
    }
    return true;
}

static EvalResult eval_treewalk_source(const char *source, char **error_message) {
    TokenArray tokens;
    StmtArray program;
    Env *env;
    EvalResult result;

    *error_message = NULL;
    if (!parse_source_text(source, &tokens, &program, error_message)) {
        EvalResult failure;
        failure.had_error = true;
        failure.is_return = false;
        failure.is_break = false;
        failure.is_continue = false;
        failure.value = value_null();
        failure.runtime_error_kind = RUNTIME_ERROR_NONE;
        failure.error_line = 0;
        failure.error_column = 0;
        failure.error_width = 0;
        failure.error_message = *error_message;
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

static EvalResult eval_vm_source(const char *source, char **error_message) {
    TokenArray tokens;
    StmtArray program;
    BytecodeProgram bytecode;
    EvalResult result;

    *error_message = NULL;
    if (!parse_source_text(source, &tokens, &program, error_message)) {
        EvalResult failure;
        failure.had_error = true;
        failure.is_return = false;
        failure.is_break = false;
        failure.is_continue = false;
        failure.value = value_null();
        failure.runtime_error_kind = RUNTIME_ERROR_NONE;
        failure.error_line = 0;
        failure.error_column = 0;
        failure.error_width = 0;
        failure.error_message = *error_message;
        return failure;
    }

    if (!compile_program(&program, source, &bytecode, error_message)) {
        EvalResult failure;
        stmt_array_free(&program);
        token_array_free(&tokens);
        failure.had_error = true;
        failure.is_return = false;
        failure.is_break = false;
        failure.is_continue = false;
        failure.value = value_null();
        failure.runtime_error_kind = RUNTIME_ERROR_INTERNAL;
        failure.error_line = 0;
        failure.error_column = 0;
        failure.error_width = 0;
        failure.error_message = *error_message;
        return failure;
    }

    result = vm_execute_program(&bytecode, source);
    bytecode_program_free(&bytecode);
    stmt_array_free(&program);
    token_array_free(&tokens);
    return result;
}

static void assert_backend_parity(TestContext *ctx, const char *source) {
    char *error = NULL;
    EvalResult tree = eval_treewalk_source(source, &error);
    EvalResult vm;
    GcTempRoot tree_root;

    if (!tree.had_error) {
        gc_temp_root_push_value(&tree_root, &tree.value);
    }
    vm = eval_vm_source(source, &error);
    if (!tree.had_error) {
        gc_temp_root_pop(&tree_root);
    }

    EXPECT_FALSE(ctx, tree.had_error);
    EXPECT_FALSE(ctx, vm.had_error);
    if (!tree.had_error && !vm.had_error) {
        EXPECT_TRUE(ctx, value_equals(tree.value, vm.value));
    }
    value_free(tree.value);
    value_free(vm.value);
    eval_result_free(&tree);
    eval_result_free(&vm);
}

static void assert_backend_error_contains(TestContext *ctx, const char *source, const char *expected_substring) {
    char *error = NULL;
    EvalResult tree = eval_treewalk_source(source, &error);
    EvalResult vm;
    GcTempRoot tree_root;

    if (!tree.had_error) {
        gc_temp_root_push_value(&tree_root, &tree.value);
    }
    vm = eval_vm_source(source, &error);
    if (!tree.had_error) {
        gc_temp_root_pop(&tree_root);
    }

    EXPECT_TRUE(ctx, tree.had_error);
    EXPECT_TRUE(ctx, vm.had_error);
    EXPECT_TRUE(ctx, tree.error_message != NULL);
    EXPECT_TRUE(ctx, vm.error_message != NULL);
    if (tree.error_message != NULL) {
        EXPECT_TRUE(ctx, strstr(tree.error_message, expected_substring) != NULL);
    }
    if (vm.error_message != NULL) {
        EXPECT_TRUE(ctx, strstr(vm.error_message, expected_substring) != NULL);
    }
    eval_result_free(&tree);
    eval_result_free(&vm);
}

static void assert_backend_value_text(TestContext *ctx, const char *source, const char *expected_text) {
    char *error = NULL;
    EvalResult tree = eval_treewalk_source(source, &error);
    EvalResult vm;
    GcTempRoot tree_root;

    if (!tree.had_error) {
        gc_temp_root_push_value(&tree_root, &tree.value);
    }
    vm = eval_vm_source(source, &error);
    if (!tree.had_error) {
        gc_temp_root_pop(&tree_root);
    }

    EXPECT_FALSE(ctx, tree.had_error);
    EXPECT_FALSE(ctx, vm.had_error);
    if (!tree.had_error && !vm.had_error) {
        char *tree_text = value_to_string(tree.value);
        char *vm_text = value_to_string(vm.value);
        EXPECT_STRING_EQ(ctx, tree_text, expected_text);
        EXPECT_STRING_EQ(ctx, vm_text, expected_text);
        free(tree_text);
        free(vm_text);
    }
    value_free(tree.value);
    value_free(vm.value);
    eval_result_free(&tree);
    eval_result_free(&vm);
}

void run_vm_tests(TestContext *ctx) {
    assert_backend_parity(ctx, "let x = 1 + 2 * 3; x;\n");
    assert_backend_parity(
        ctx,
        "let x = 10;\n"
        "if (x > 5) { x = x * 2; } else { x = 0; }\n"
        "x;\n"
    );
    assert_backend_parity(
        ctx,
        "function add(a, b) { return a + b; }\n"
        "let value = add(3, 4);\n"
        "value;\n"
    );
    assert_backend_parity(
        ctx,
        "let total = 0;\n"
        "let i = 0;\n"
        "while (i < 4) { total = total + i; i = i + 1; }\n"
        "typeOf(total) + \":\" + len(\"hello\") + \":\" + total;\n"
    );
    assert_backend_parity(
        ctx,
        "function bump(n) { return n + 1; }\n"
        "let value = 0;\n"
        "for (let i = 0; i < 3; i = i + 1) { value = bump(value); }\n"
        "value;\n"
    );
    assert_backend_parity(
        ctx,
        "function makeAdder(base) {\n"
        "  function add(n) { return base + n; }\n"
        "  return add;\n"
        "}\n"
        "let add3 = makeAdder(3);\n"
        "add3(4);\n"
    );
    assert_backend_parity(
        ctx,
        "let makeAdder = function (base) {\n"
        "  return function (n) { return base + n; };\n"
        "};\n"
        "let add3 = makeAdder(3);\n"
        "add3(8);\n"
    );
    assert_backend_parity(
        ctx,
        "function makeCounter(start) {\n"
        "  let current = start;\n"
        "  return function (step) {\n"
        "    current = current + step;\n"
        "    return current;\n"
        "  };\n"
        "}\n"
        "let counter = makeCounter(2);\n"
        "counter(3) + counter(4);\n"
    );
    assert_backend_parity(
        ctx,
        "let values = [1, 2, 3, 4];\n"
        "let meta = {name: \"jsrt\", third: values[2], nested: {label: \"runtime\"}};\n"
        "meta.name + \":\" + meta.third + \":\" + meta.nested.label;\n"
    );
    assert_backend_parity(
        ctx,
        "let arr = [1, 2, 3];\n"
        "let obj = {name: \"old\", nested: {value: arr[0]}};\n"
        "arr[2] = 7;\n"
        "obj.name = \"new\";\n"
        "obj[\"nested\"].value = arr[2];\n"
        "arr[0] + \":\" + obj.name + \":\" + obj.nested.value;\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {name: \"base\", nested: {value: 0}};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.nested.value = 1;\n"
        "obj.name = \"child\";\n"
        "len(obj) + \":\" + obj.name + \":\" + proto.name + \":\" + proto.nested.value;\n"
    );
    assert_backend_parity(
        ctx,
        "let out = 0;\n"
        "switch (2) {\n"
        "  case 1:\n"
        "    out = 1;\n"
        "    break;\n"
        "  case 2:\n"
        "    out = out + 2;\n"
        "  case 3:\n"
        "    out = out + 3;\n"
        "    break;\n"
        "  default:\n"
        "    out = 99;\n"
        "}\n"
        "out;\n"
    );
    assert_backend_parity(
        ctx,
        "let i = 0;\n"
        "let total = 0;\n"
        "do {\n"
        "  i = i + 1;\n"
        "  if (i == 2) { continue; }\n"
        "  total = total + i;\n"
        "} while (i < 4);\n"
        "total;\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {name: \"base\"};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.name;\n"
    );
    assert_backend_error_contains(
        ctx,
        "let proto = {name: \"base\"};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "setPrototypeOf(obj, null);\n"
        "obj.name;\n",
        "object key 'name' not found"
    );
    assert_backend_parity(
        ctx,
        "let grand = {name: \"root\"};\n"
        "let proto = {};\n"
        "let obj = {};\n"
        "setPrototypeOf(proto, grand);\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.name;\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {name: \"base\"};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.name = \"child\";\n"
        "proto.name + \":\" + obj.name;\n"
    );
    assert_backend_parity(
        ctx,
        "let obj = {name: \"receiver\"};\n"
        "let proto = {name: \"proto\"};\n"
        "let returned = setPrototypeOf(obj, proto);\n"
        "returned.extra = 7;\n"
        "obj.extra + \":\" + returned.extra + \":\" + proto.name;\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {nested: {value: 0}};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.nested.value = 1;\n"
        "len(obj) + \":\" + proto.nested.value + \":\" + obj.nested.value;\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {speak: function () { return 7; }};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.speak();\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {hidden: 1};\n"
        "let obj = {shown: 2};\n"
        "setPrototypeOf(obj, proto);\n"
        "len(obj);\n"
    );
    assert_backend_parity(
        ctx,
        "let protoA = {kind: \"a\"};\n"
        "let protoB = {kind: \"b\"};\n"
        "let left = {name: \"same\"};\n"
        "let right = {name: \"same\"};\n"
        "setPrototypeOf(left, protoA);\n"
        "setPrototypeOf(right, protoB);\n"
        "left == right;\n"
    );
    assert_backend_value_text(
        ctx,
        "let proto = {hidden: 1};\n"
        "let obj = {shown: 2};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj;\n",
        "{shown: 2}"
    );
    assert_backend_error_contains(
        ctx,
        "let proto = {name: \"base\"};\n"
        "let obj = {};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.missing;\n",
        "object key 'missing' not found"
    );
    assert_backend_error_contains(ctx, "setPrototypeOf(1, {});\n", "setPrototypeOf() expects an object receiver");
    assert_backend_error_contains(ctx, "setPrototypeOf({}, 1);\n", "setPrototypeOf() expects an object or null prototype");
    assert_backend_error_contains(
        ctx,
        "let obj = {};\n"
        "setPrototypeOf(obj, obj);\n",
        "setPrototypeOf() cannot set an object as its own prototype"
    );
    assert_backend_error_contains(
        ctx,
        "let a = {};\n"
        "let b = {};\n"
        "setPrototypeOf(a, b);\n"
        "setPrototypeOf(b, a);\n",
        "setPrototypeOf() cannot create a prototype cycle"
    );
    assert_backend_parity(ctx, "this == null;\n");
    assert_backend_parity(
        ctx,
        "function Person(name) { this.name = name; }\n"
        "Person.prototype = {\n"
        "  describe: function () {\n"
        "    if (this == null) { return \"null\"; }\n"
        "    return this.name;\n"
        "  }\n"
        "};\n"
        "let ada = new Person(\"Ada\");\n"
        "let extracted = ada.describe;\n"
        "ada.describe() + \":\" + extracted() + \":\" + (this == null);\n"
    );
    assert_backend_parity(
        ctx,
        "function Person(name) { this.name = name; }\n"
        "let proto = {\n"
        "  greet: function () { return this.name; }\n"
        "};\n"
        "Person.prototype = proto;\n"
        "let ada = new Person(\"Ada\");\n"
        "ada.greet() + \":\" + ada[\"greet\"]();\n"
    );
    assert_backend_parity(
        ctx,
        "function Keep(n) { this.n = n; return 7; }\n"
        "function Replace() { return {ok: 9}; }\n"
        "let kept = new Keep(4);\n"
        "let replaced = new Replace();\n"
        "kept.n + \":\" + replaced.ok;\n"
    );
    assert_backend_parity(
        ctx,
        "function Box() {}\n"
        "let firstProto = {label: \"first\"};\n"
        "let secondProto = {label: \"second\"};\n"
        "Box.prototype = firstProto;\n"
        "let first = new Box();\n"
        "Box[\"prototype\"] = secondProto;\n"
        "let second = new Box();\n"
        "Box.prototype.label + \":\" + first.label + \":\" + second.label;\n"
    );
    assert_backend_parity(
        ctx,
        "let order = \"\";\n"
        "function mark(label, value) { order = order + label; return value; }\n"
        "let obj = {\n"
        "  value: 0,\n"
        "  method: function (value) { this.value = value; return this.value; }\n"
        "};\n"
        "function getObj() { order = order + \"b\"; return obj; }\n"
        "getObj()[mark(\"k\", \"method\")](mark(\"a\", 7));\n"
        "order + \":\" + obj.value;\n"
    );
    assert_backend_error_contains(
        ctx,
        "let ctor = 1;\n"
        "new ctor(2);\n",
        "new expects a constructor function"
    );
    assert_backend_parity(ctx, "typeOf(print.prototype);\n");
    assert_backend_error_contains(ctx, "new print(1);\n", "native functions are not constructors");
    assert_backend_error_contains(ctx, "print.prototype = {};\n", "native functions are not constructors");
    assert_backend_error_contains(
        ctx,
        "function Box() {}\n"
        "Box.prototype = 1;\n",
        "function prototype must be an object or null"
    );
    assert_backend_error_contains(
        ctx,
        "function Box() {}\n"
        "Box.extra;\n",
        "function properties are limited to 'prototype' in L3"
    );
    assert_backend_error_contains(
        ctx,
        "function Box() {}\n"
        "Box[0];\n",
        "function properties are limited to 'prototype' in L3"
    );

    {
        char *error = NULL;
        EvalResult result = eval_vm_source(
            "let proto = {name: \"base\", nested: {value: 0}};\n"
            "let obj = {};\n"
            "setPrototypeOf(obj, proto);\n"
            "obj.nested.value = 1;\n"
            "obj.name = \"child\";\n"
            "len(obj) + \":\" + obj.name + \":\" + proto.name + \":\" + proto.nested.value;\n",
            &error
        );
        EXPECT_FALSE(ctx, result.had_error);
        EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
        EXPECT_STRING_EQ(ctx, result.value.as.string, "1:child:base:1");
        value_free(result.value);
        eval_result_free(&result);
    }

    {
        char *error = NULL;
        EvalResult result = eval_vm_source(
            "function outer(n) {\n"
            "  function inner(x) { return x - n; }\n"
            "  return inner;\n"
            "}\n"
            "let add = outer(3);\n"
            "add({bad: true});\n",
            &error
        );
        EXPECT_TRUE(ctx, result.had_error);
        EXPECT_TRUE(ctx, result.error_message != NULL);
        EXPECT_TRUE(ctx, strstr(result.error_message, "binary operator expects numbers") != NULL);
        eval_result_free(&result);
    }

    {
        FILE *file;
        char buffer[128];
        EXPECT_INT_EQ(ctx, system("JSRT_BACKEND=vm ./build/jsrt examples/vm_subset.js > build/vm-example.out"), 0);
        file = fopen("build/vm-example.out", "r");
        EXPECT_TRUE(ctx, file != NULL);
        if (file != NULL) {
            EXPECT_TRUE(ctx, fgets(buffer, sizeof(buffer), file) != NULL);
            EXPECT_STRING_EQ(ctx, buffer, "number:5:12\n");
            fclose(file);
        }
    }

    {
        FILE *file;
        char buffer[128];
        EXPECT_INT_EQ(ctx, system("JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js > build/vm-surface.out"), 0);
        file = fopen("build/vm-surface.out", "r");
        EXPECT_TRUE(ctx, file != NULL);
        if (file != NULL) {
            EXPECT_TRUE(ctx, fgets(buffer, sizeof(buffer), file) != NULL);
            EXPECT_STRING_EQ(ctx, buffer, "1:new:8:7\n");
            fclose(file);
        }
    }

    {
        FILE *file;
        char buffer[128];
        EXPECT_INT_EQ(ctx, system("JSRT_BACKEND=vm ./build/jsrt examples/constructors.js > build/constructors-example.out"), 0);
        file = fopen("build/constructors-example.out", "r");
        EXPECT_TRUE(ctx, file != NULL);
        if (file != NULL) {
            EXPECT_TRUE(ctx, fgets(buffer, sizeof(buffer), file) != NULL);
            EXPECT_STRING_EQ(ctx, buffer, "hello Ada\n");
            fclose(file);
        }
    }
}

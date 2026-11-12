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

static int vm_interrupt_calls = 0;

static bool vm_interrupt_after_three_calls(void *user_data) {
    (void)user_data;
    vm_interrupt_calls += 1;
    return vm_interrupt_calls > 3;
}

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
        "let x = 0;\n"
        "if (true) { x = 7; }\n"
        "x;\n"
    );
    assert_backend_parity(
        ctx,
        "let x = 1;\n"
        "if (false) { x = 2; }\n"
        "x;\n"
    );
    assert_backend_parity(
        ctx,
        "function add(a, b) { return a + b; }\n"
        "let value = add(3, 4);\n"
        "value;\n"
    );
    /* Four declarations grow the function table and move its storage. */
    assert_backend_parity(
        ctx,
        "function f1() { return 1; }\n"
        "function f2() { return 2; }\n"
        "function f3() { return 3; }\n"
        "function f4() { return 4; }\n"
        "f1() + f2() + f3() + f4();\n"
    );
    /* Nested declarations grow the table while parent compilers run. */
    assert_backend_parity(
        ctx,
        "function outer() {\n"
        "  function a() { return 1; }\n"
        "  function b() { return 2; }\n"
        "  function c() { return 3; }\n"
        "  function d() { return 4; }\n"
        "  return a() + b() + c() + d();\n"
        "}\n"
        "outer();\n"
    );
    {
        /* Twenty declarations force several growth steps. */
        char source[2048];
        size_t offset = 0U;
        size_t i;

        for (i = 1U; i <= 20U; ++i) {
            offset += (size_t)snprintf(
                source + offset,
                sizeof(source) - offset,
                "function many%zu() { return 1; }\n",
                i
            );
        }
        offset += (size_t)snprintf(source + offset, sizeof(source) - offset, "many1()");
        for (i = 2U; i <= 20U; ++i) {
            offset += (size_t)snprintf(source + offset, sizeof(source) - offset, " + many%zu()", i);
        }
        (void)snprintf(source + offset, sizeof(source) - offset, ";\n");
        assert_backend_value_text(ctx, source, "20");
    }
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
        "  return function () { return current; };\n"
        "}\n"
        "let read = makeCounter(2);\n"
        "read() + read();\n"
    );
    /* A captured object stays shared, so property writes remain visible. */
    assert_backend_parity(
        ctx,
        "function outer() {\n"
        "  let box = {value: 1};\n"
        "  function bump() { box.value = box.value + 1; }\n"
        "  bump();\n"
        "  return box.value;\n"
        "}\n"
        "outer();\n"
    );
    {
        const char *source =
            "function outer() {\n"
            "  let x = 1;\n"
            "  function inc() { x = x + 1; }\n"
            "  inc();\n"
            "  return x;\n"
            "}\n"
            "outer();\n";
        char *error = NULL;
        EvalResult tree = eval_treewalk_source(source, &error);
        EvalResult vm = eval_vm_source(source, &error);

        EXPECT_FALSE(ctx, tree.had_error);
        if (!tree.had_error) {
            value_free(tree.value);
        }
        EXPECT_TRUE(ctx, vm.had_error);
        EXPECT_TRUE(ctx, vm.error_message != NULL);
        if (vm.error_message != NULL) {
            EXPECT_TRUE(ctx, strstr(vm.error_message, "Compiler error") != NULL);
            EXPECT_TRUE(ctx, strstr(vm.error_message, "does not support assigning to a captured variable") != NULL);
        }
        eval_result_free(&tree);
        eval_result_free(&vm);
    }
    assert_backend_parity(
        ctx,
        "let values = [1, 2, 3, 4];\n"
        "let meta = {name: \"jsrt\", third: values[2], nested: {label: \"runtime\"}};\n"
        "meta.name + \":\" + meta.third + \":\" + meta.nested.label;\n"
    );
    /* A duplicate object key keeps the last value and one entry. */
    assert_backend_parity(
        ctx,
        "let o = {a: 1, a: 2};\n"
        "o.a + \":\" + len(o);\n"
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
    assert_backend_parity(ctx, "this;\n");
    assert_backend_parity(ctx, "function readThis() { return this; }\nreadThis();\n");
    assert_backend_parity(
        ctx,
        "let obj = {value: 4, get: function () { if (this == null) { return 1; } return this.value; }};\n"
        "let f = obj.get;\n"
        "f();\n"
    );
    assert_backend_parity(
        ctx,
        "let obj = {value: 4, get: function () { return this.value; }};\n"
        "obj.get() + obj[\"get\"]();\n"
    );
    assert_backend_parity(
        ctx,
        "let proto = {get: function () { return this.value; }};\n"
        "let obj = {value: 7};\n"
        "setPrototypeOf(obj, proto);\n"
        "obj.get() + obj[\"get\"]();\n"
    );
    assert_backend_parity(
        ctx,
        "function Box(v) { this.value = v; }\n"
        "let proto = {get: function () { return this.value; }};\n"
        "Box.prototype = proto;\n"
        "let box = new Box(9);\n"
        "box.get();\n"
    );
    assert_backend_parity(
        ctx,
        "function Box(v) { this.value = v; }\n"
        "let proto = {get: function () { return this.value; }};\n"
        "Box[\"prototype\"] = proto;\n"
        "let box = new Box(6);\n"
        "Box[\"prototype\"] == proto && box[\"get\"]() == 6;\n"
    );
    assert_backend_parity(
        ctx,
        "function Replace() { return {value: 8}; }\n"
        "let out = new Replace();\n"
        "out.value;\n"
    );
    assert_backend_parity(
        ctx,
        "function Keep() { this.value = 3; return 0; }\n"
        "let out = new Keep();\n"
        "out.value;\n"
    );
    assert_backend_parity(
        ctx,
        "function KeepNull() { this.value = 5; return null; }\n"
        "let out = new KeepNull();\n"
        "out.value;\n"
    );
    assert_backend_parity(
        ctx,
        "class Box {\n"
        "  constructor(v) { this.value = v; }\n"
        "  get() { return this.value; }\n"
        "  twice() { return this.get() + this.get(); }\n"
        "}\n"
        "let box = new Box(3);\n"
        "box.twice();\n"
    );
    assert_backend_parity(
        ctx,
        "class Empty {\n"
        "  get() { return 7; }\n"
        "}\n"
        "let box = new Empty();\n"
        "box.get();\n"
    );
    assert_backend_parity(
        ctx,
        "class Box {\n"
        "  constructor(v) { this.value = v; }\n"
        "  get() { if (this == null) { return 1; } return this.value; }\n"
        "}\n"
        "let box = new Box(4);\n"
        "let f = box.get;\n"
        "f();\n"
    );
    assert_backend_parity(
        ctx,
        "class Box {\n"
        "  constructor(v) { this.value = v; }\n"
        "  get() { return 0; }\n"
        "}\n"
        "let proto = {get: function () { return this.value; }};\n"
        "Box[\"prototype\"] = proto;\n"
        "let box = new Box(9);\n"
        "Box[\"prototype\"] == proto && box.get() == 9;\n"
    );
    assert_backend_error_contains(
        ctx,
        "class Box { constructor(v) { this.value = v; } }\n"
        "Box(1);\n",
        "class constructors must be called with 'new'"
    );
    assert_backend_error_contains(
        ctx,
        "Box;\n"
        "class Box {}\n",
        "undefined variable 'Box'"
    );
    assert_backend_error_contains(
        ctx,
        "{ class Local {} }\n"
        "Local;\n",
        "undefined variable 'Local'"
    );
    assert_backend_error_contains(
        ctx,
        "class Box {}\n"
        "class Box {}\n",
        "redeclaration of 'Box' in the same scope"
    );
    assert_backend_error_contains(ctx, "class Box extends Base {}\n", "extends is unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "class Box { super(); }\n", "super is unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "class Box { static foo() {} }\n", "static class members are unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "class Box { get value() {} }\n", "getter syntax is unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "class Box { set value(v) {} }\n", "setter syntax is unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "class Box { field = 1; }\n", "class fields are unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "class Box { [\"value\"]() {} }\n", "computed method names are unsupported in jsrt class declarations");
    assert_backend_error_contains(ctx, "let Box = class {};\n", "class expressions are unsupported in jsrt class declarations");
    assert_backend_parity(ctx, "print.prototype;\n");
    assert_backend_parity(
        ctx,
        "let log = \"\";\n"
        "function note(label, value) { log = log + label; return value; }\n"
        "let holder = {value: 5, get: function (x) { return log + \":\" + this.value + \":\" + x; }};\n"
        "note(\"A\", holder)[note(\"B\", \"get\")](note(\"C\", 7));\n"
    );
    assert_backend_parity(
        ctx,
        "let log = \"\";\n"
        "function note(label, value) { log = log + label; return value; }\n"
        "function Box(v) { this.value = v; }\n"
        "let holder = {Box: Box};\n"
        "let out = new holder[note(\"B\", \"Box\")](note(\"C\", 4));\n"
        "log + \":\" + out.value;\n"
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
    assert_backend_error_contains(ctx, "new print();\n", "native functions are not constructors");
    assert_backend_error_contains(
        ctx,
        "let NotCtor = 1;\n"
        "new NotCtor();\n",
        "attempted to construct a non-function value"
    );
    assert_backend_error_contains(ctx, "function Box() {}\nBox.prototype = 1;\n", "function prototype must be an object or null");
    assert_backend_error_contains(ctx, "print.prototype = {};\n", "native function prototypes are read-only for constructor/prototype behavior");
    assert_backend_error_contains(ctx, "function Box() {}\nBox.x;\n", "only function.prototype is supported for constructor/prototype behavior");
    assert_backend_error_contains(ctx, "function Box() {}\nBox[\"x\"];\n", "only function.prototype is supported for constructor/prototype behavior");
    assert_backend_error_contains(ctx, "function Box() {}\nBox[0];\n", "only function.prototype is supported for constructor/prototype behavior");
    assert_backend_error_contains(ctx, "function Box() {}\nBox.x = 1;\n", "only function.prototype is supported for constructor/prototype behavior");
    assert_backend_error_contains(ctx, "function Box() {}\nBox[\"x\"] = 1;\n", "only function.prototype is supported for constructor/prototype behavior");
    assert_backend_parity(
        ctx,
        "class Box {\n"
        "  constructor(v) { this.value = v; }\n"
        "  get() { return this.value; }\n"
        "  read() { return this.get(); }\n"
        "}\n"
        "let box = new Box(3);\n"
        "box.read();\n"
    );
    assert_backend_parity(
        ctx,
        "class Empty { get() { return this.value; } }\n"
        "let box = new Empty();\n"
        "box.value = 4;\n"
        "box.get == Empty.prototype.get && box.get() == 4;\n"
    );
    assert_backend_parity(
        ctx,
        "class Box { get() { if (this == null) { return 1; } return this.value; } }\n"
        "let box = new Box();\n"
        "box.value = 4;\n"
        "let f = box.get;\n"
        "f();\n"
    );
    assert_backend_parity(
        ctx,
        "class Box {\n"
        "  constructor(v) { this.value = v; }\n"
        "  get() { return this.value; }\n"
        "}\n"
        "let proto = {get: function () { return 8; }};\n"
        "Box[\"prototype\"] = proto;\n"
        "let box = new Box(2);\n"
        "Box.prototype == proto && box.get() == 8;\n"
    );
    assert_backend_error_contains(ctx, "class Box {}\nBox();\n", "class constructors must be called with 'new'");
    assert_backend_error_contains(ctx, "Box;\nclass Box {}\n", "undefined variable 'Box'");
    assert_backend_error_contains(ctx, "if (true) { class Box {} }\nBox;\n", "undefined variable 'Box'");
    assert_backend_error_contains(ctx, "class Box {}\nclass Box {}\n", "redeclaration of 'Box' in the same scope");

    {
        char *error = NULL;
        EvalResult tree = eval_treewalk_source("new Ctor;\n", &error);
        EvalResult vm_result = eval_vm_source("new Ctor;\n", &error);

        EXPECT_TRUE(ctx, tree.had_error);
        EXPECT_TRUE(ctx, vm_result.had_error);
        EXPECT_TRUE(ctx, strstr(tree.error_message, "expected '(' after constructor in 'new' expression") != NULL);
        EXPECT_TRUE(ctx, strstr(vm_result.error_message, "expected '(' after constructor in 'new' expression") != NULL);
        eval_result_free(&tree);
        eval_result_free(&vm_result);
    }

    {
        char *error = NULL;
        EvalResult tree = eval_treewalk_source("new this();\n", &error);
        EvalResult vm_result = eval_vm_source("new this();\n", &error);

        EXPECT_TRUE(ctx, tree.had_error);
        EXPECT_TRUE(ctx, vm_result.had_error);
        EXPECT_TRUE(ctx, strstr(tree.error_message, "expected constructor target after 'new'") != NULL);
        EXPECT_TRUE(ctx, strstr(vm_result.error_message, "expected constructor target after 'new'") != NULL);
        eval_result_free(&tree);
        eval_result_free(&vm_result);
    }

    {
        char *error = NULL;
        EvalResult tree = eval_treewalk_source("let NotCtor = 1;\nnew NotCtor();\n", &error);
        EvalResult vm_result = eval_vm_source("let NotCtor = 1;\nnew NotCtor();\n", &error);

        EXPECT_TRUE(ctx, tree.had_error);
        EXPECT_TRUE(ctx, vm_result.had_error);
        EXPECT_STRING_EQ(ctx, tree.error_message, vm_result.error_message);
        eval_result_free(&tree);
        eval_result_free(&vm_result);
    }

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
        char *error = NULL;
        EvalResult tree = eval_treewalk_source(
            "let f = function(n) { if (n <= 0) { return 0; } return f(n - 1) + 1; };\n"
            "f(300);\n",
            &error
        );
        EvalResult vm_result = eval_vm_source(
            "let f = function(n) { if (n <= 0) { return 0; } return f(n - 1) + 1; };\n"
            "f(300);\n",
            &error
        );

        EXPECT_TRUE(ctx, tree.had_error);
        EXPECT_TRUE(ctx, vm_result.had_error);
        EXPECT_TRUE(ctx, strstr(tree.error_message, "maximum call depth exceeded") != NULL);
        EXPECT_TRUE(ctx, strstr(vm_result.error_message, "maximum call depth exceeded") != NULL);
        eval_result_free(&tree);
        eval_result_free(&vm_result);
    }

    {
        char *error = NULL;
        EvalResult result;

        /* The VM checks the host interrupt hook between instructions. */
        vm_interrupt_calls = 0;
        eval_set_interrupt_check(vm_interrupt_after_three_calls, NULL);
        result = eval_vm_source("let i = 0; while (i < 100000) { i = i + 1; } i;", &error);
        eval_set_interrupt_check(NULL, NULL);
        EXPECT_TRUE(ctx, result.had_error);
        EXPECT_TRUE(ctx, strstr(result.error_message, "execution interrupted") != NULL);
        eval_result_free(&result);
    }

    {
        /* A corrupted instruction stream trips the stack underflow guard. */
        BytecodeInstruction instructions[2];
        BytecodeFunction function;
        BytecodeFunction *functions[1];
        BytecodeProgram program;
        EvalResult result;

        memset(&function, 0, sizeof(function));
        memset(instructions, 0, sizeof(instructions));
        instructions[0].op = BC_POP;
        instructions[0].width = 1;
        instructions[1].op = BC_RETURN;
        instructions[1].width = 1;
        function.instructions = instructions;
        function.instruction_count = 2U;
        functions[0] = &function;
        bytecode_program_init(&program);
        program.functions = functions;
        program.function_count = 1U;
        program.function_capacity = 1U;
        result = vm_execute_program(&program, "underflow");
        EXPECT_TRUE(ctx, result.had_error);
        EXPECT_TRUE(ctx, result.error_message != NULL);
        if (result.error_message != NULL) {
            EXPECT_TRUE(ctx, strstr(result.error_message, "vm stack underflow") != NULL);
        }
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
        EXPECT_INT_EQ(ctx, system("JSRT_BACKEND=vm ./build/jsrt examples/class_syntax.js > build/class-example.out"), 0);
        file = fopen("build/class-example.out", "r");
        EXPECT_TRUE(ctx, file != NULL);
        if (file != NULL) {
            EXPECT_TRUE(ctx, fgets(buffer, sizeof(buffer), file) != NULL);
            EXPECT_STRING_EQ(ctx, buffer, "7:true\n");
            fclose(file);
        }
    }
}

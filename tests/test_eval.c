#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/ast.h"
#include "jsrt/env.h"
#include "jsrt/eval.h"
#include "jsrt/gc.h"
#include "jsrt/lexer.h"
#include "jsrt/parser.h"
#include "jsrt/value.h"
#include "test.h"

static EvalResult evaluate_source(const char *source, char **parse_error) {
    TokenArray tokens;
    StmtArray program;
    Env *env;
    EvalResult result;

    *parse_error = NULL;
    if (!lex_source(source, &tokens, parse_error)) {
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
        failure.error_message = *parse_error;
        return failure;
    }

    if (!parse_tokens(&tokens, source, &program, parse_error)) {
        EvalResult failure;
        token_array_free(&tokens);
        failure.had_error = true;
        failure.is_return = false;
        failure.is_break = false;
        failure.is_continue = false;
        failure.value = value_null();
        failure.runtime_error_kind = RUNTIME_ERROR_NONE;
        failure.error_line = 0;
        failure.error_column = 0;
        failure.error_width = 0;
        failure.error_message = *parse_error;
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

static void assert_example_output(TestContext *ctx, const char *path, const char *expected_output) {
    char command[256];
    FILE *file;
    char buffer[128];

    snprintf(command, sizeof(command), "./build/jsrt %s > build/example.out", path);
    EXPECT_INT_EQ(ctx, system(command), 0);
    file = fopen("build/example.out", "r");
    EXPECT_TRUE(ctx, file != NULL);
    if (file != NULL) {
        EXPECT_TRUE(ctx, fgets(buffer, sizeof(buffer), file) != NULL);
        EXPECT_STRING_EQ(ctx, buffer, expected_output);
        fclose(file);
    }
}

void run_eval_tests(TestContext *ctx) {
    char *error = NULL;
    EvalResult result = evaluate_source(
        "let x = 3;\n"
        "function twice(n) { return n * 2; }\n"
        "if (x < 10) { x = twice(x + 4); } else { x = 0; }\n"
        "x;\n",
        &error
    );

    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 14.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("\"hello\" + \" world\";\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
    EXPECT_STRING_EQ(ctx, result.value.as.string, "hello world");
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("[1, 2, 3][1];\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 2.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("let obj = {name: \"jsrt\", count: 3}; obj.name;\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
    EXPECT_STRING_EQ(ctx, result.value.as.string, "jsrt");
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("let obj = {name: \"jsrt\", nested: {value: 7}}; obj.nested.value;\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("let obj = {name: \"jsrt\"}; obj[\"name\"];\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
    EXPECT_STRING_EQ(ctx, result.value.as.string, "jsrt");
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("len(\"hello\");\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 5.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("len([1, 2, 3]);\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 3.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("len({name: \"jsrt\", count: 2});\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 2.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("typeOf([1, 2]);\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
    EXPECT_STRING_EQ(ctx, result.value.as.string, "array");
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("typeOf(function (n) { return n; });\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
    EXPECT_STRING_EQ(ctx, result.value.as.string, "function");
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("[1, 2] == [1, 2];\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_BOOL);
    EXPECT_TRUE(ctx, result.value.as.boolean);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("let arr = [1]; arr[3];\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "array index out of bounds") != NULL);
    eval_result_free(&result);

    result = evaluate_source("let obj = {name: \"jsrt\"}; obj.missing;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "object key 'missing' not found") != NULL);
    eval_result_free(&result);

    result = evaluate_source("let obj = {name: \"jsrt\"}; obj[1];\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "object indexing expects a string key") != NULL);
    eval_result_free(&result);

    result = evaluate_source(
        "let i = 0;\n"
        "let total = 0;\n"
        "while (i < 5) { total = total + i; i = i + 1; }\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 10.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let i = 0;\n"
        "do { i = i + 1; } while (false);\n"
        "i;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 1.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let i = 0;\n"
        "let total = 0;\n"
        "do {\n"
        "  i = i + 1;\n"
        "  if (i == 2) { continue; }\n"
        "  total = total + i;\n"
        "} while (i < 4);\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 8.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let i = 0;\n"
        "do {\n"
        "  i = i + 1;\n"
        "  if (i == 3) { break; }\n"
        "} while (true);\n"
        "i;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 3.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
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
        "out;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 5.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let out = 0;\n"
        "switch (9) {\n"
        "  case 1:\n"
        "    out = 1;\n"
        "    break;\n"
        "  default:\n"
        "    out = 7;\n"
        "}\n"
        "out;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let i = 0;\n"
        "let out = 0;\n"
        "while (true) {\n"
        "  i = i + 1;\n"
        "  switch (i) {\n"
        "    case 2:\n"
        "      out = out + 10;\n"
        "      break;\n"
        "    case 3:\n"
        "      break;\n"
        "  }\n"
        "  out = out + i;\n"
        "  if (i == 3) { break; }\n"
        "}\n"
        "out;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 16.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("0 || 7;\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("\"hi\" && 3;\n", &error);
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 3.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let arr = [1, 2, 3];\n"
        "arr[1] = 9;\n"
        "arr[1];\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 9.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let obj = {name: \"old\"};\n"
        "obj.name = \"new\";\n"
        "obj[\"count\"] = 3;\n"
        "obj.count;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 3.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let obj = {nested: {values: [1, 2, 3]}};\n"
        "obj.nested.values[0] = 7;\n"
        "obj.nested.values[0];\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let arr = [1, 2];\n"
        "let alias = arr;\n"
        "alias[0] = 99;\n"
        "arr[0];\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 99.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let obj = {nested: {value: 1}};\n"
        "let alias = obj.nested;\n"
        "alias.value = 7;\n"
        "obj.nested.value;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function bump(items) {\n"
        "  items[0] = items[0] + 4;\n"
        "}\n"
        "let values = [1, 2];\n"
        "bump(values);\n"
        "values[0];\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 5.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function identity(value) { return value; }\n"
        "let values = [1, 2];\n"
        "let alias = identity(values);\n"
        "alias[1] = 9;\n"
        "values[1];\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 9.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let obj = {};\n"
        "obj.self = obj;\n"
        "obj == obj;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_BOOL);
    EXPECT_TRUE(ctx, result.value.as.boolean);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let obj = {};\n"
        "obj.self = obj;\n"
        "\"cycle:\" + obj;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_STRING);
    EXPECT_STRING_EQ(ctx, result.value.as.string, "cycle:{self: <cycle>}");
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("let arr = [1]; arr[2] = 7;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "array assignment index out of bounds") != NULL);
    eval_result_free(&result);

    result = evaluate_source("let n = 3; n.x = 1;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "property assignment requires an object value") != NULL);
    eval_result_free(&result);

    result = evaluate_source("let n = 3; n[0] = 1;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "index assignment requires an array or object value") != NULL);
    eval_result_free(&result);

    result = evaluate_source("len(3);\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "len() expects a string, array, or object") != NULL);
    eval_result_free(&result);

    result = evaluate_source("print();\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "expected 1 arguments but got 0") != NULL);
    eval_result_free(&result);

    result = evaluate_source(
        "let total = 0;\n"
        "for (let i = 0; i < 5; i = i + 1) { total = total + i; }\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 10.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let total = 0;\n"
        "for (let i = 0; i < 5; i = i + 1) {\n"
        "  if (i == 2) { continue; }\n"
        "  total = total + i;\n"
        "}\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 8.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let i = 0;\n"
        "let total = 0;\n"
        "while (true) {\n"
        "  i = i + 1;\n"
        "  if (i == 2) { continue; }\n"
        "  total = total + i;\n"
        "  if (i == 4) { break; }\n"
        "}\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 8.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let total = 0;\n"
        "for (let i = 0; ; i = i + 1) {\n"
        "  if (i == 4) { break; }\n"
        "  total = total + i;\n"
        "}\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 6.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source("break;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "break is only valid inside a loop or switch") != NULL);
    eval_result_free(&result);

    result = evaluate_source("continue;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "continue is only valid inside a loop") != NULL);
    eval_result_free(&result);

    result = evaluate_source("return 1;\n", &error);
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "return is only valid inside a function body") != NULL);
    eval_result_free(&result);

    result = evaluate_source(
        "function makeAdder(base) {\n"
        "  function add(n) { return base + n; }\n"
        "  return add;\n"
        "}\n"
        "let add3 = makeAdder(3);\n"
        "add3(4);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let add = function (a, b) { return a + b; };\n"
        "add(5, 6);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 11.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let makeAdder = function (base) {\n"
        "  return function (n) { return base + n; };\n"
        "};\n"
        "let add3 = makeAdder(3);\n"
        "add3(8);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 11.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function makeCounter(start) {\n"
        "  let current = start;\n"
        "  return function (step) {\n"
        "    current = current + step;\n"
        "    return current;\n"
        "  };\n"
        "}\n"
        "let counter = makeCounter(2);\n"
        "counter(3) + counter(4);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 14.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function buildPair(n) {\n"
        "  let pair = [{value: n}, {value: n + 1}];\n"
        "  return pair[0].value + pair[1].value;\n"
        "}\n"
        "let total = 0;\n"
        "for (let i = 0; i < 4; i = i + 1) {\n"
        "  total = total + buildPair(i);\n"
        "}\n"
        "total;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 16.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function bumpBox(input) {\n"
        "  let local = input;\n"
        "  local[0].box.count = local[0].box.count + 5;\n"
        "  return input[0].box.count;\n"
        "}\n"
        "let original = [{box: {count: 2}}];\n"
        "bumpBox(original) + original[0].box.count;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 14.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let sumDown = function self(n, acc) {\n"
        "  if (n <= 0) { return acc[0].value; }\n"
        "  let next = acc;\n"
        "  next[0].value = next[0].value + n;\n"
        "  return self(n - 1, next);\n"
        "};\n"
        "sumDown(4, [{value: 0}]);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 10.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "let fact = function self(n) {\n"
        "  if (n <= 1) { return 1; }\n"
        "  return n * self(n - 1);\n"
        "};\n"
        "fact(5);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 120.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function makeBox(n) {\n"
        "  return [n];\n"
        "}\n"
        "makeBox(1) == makeBox(1);\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_BOOL);
    EXPECT_TRUE(ctx, result.value.as.boolean);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function makeBox(n) {\n"
        "  return [n];\n"
        "}\n"
        "function sumThree(a, b, c) {\n"
        "  return a[0] + b[0] + c[0];\n"
        "}\n"
        "sumThree(makeBox(1), makeBox(2), makeBox(3));\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 6.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function id(n) { return n; }\n"
        "let arr = [1, id(2), 3];\n"
        "let obj = {first: id(4), nested: {value: id(5)}};\n"
        "arr[1] + obj.first + obj.nested.value;\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 11.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "function pick() { return 1; }\n"
        "let obj = {nested: [0, [1]]};\n"
        "obj.nested[pick()] = [7];\n"
        "obj.nested[1][0];\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 7.0);
    value_free(result.value);
    eval_result_free(&result);

    result = evaluate_source(
        "const x = 3;\n"
        "x = 4;\n",
        &error
    );
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "cannot assign to const variable 'x'") != NULL);
    eval_result_free(&result);

    result = evaluate_source(
        "let x = 1;\n"
        "let x = 2;\n",
        &error
    );
    EXPECT_TRUE(ctx, result.had_error);
    EXPECT_TRUE(ctx, strstr(result.error_message, "redeclaration of 'x' in the same scope") != NULL);
    eval_result_free(&result);

    result = evaluate_source(
        "const x = 3;\n"
        "{ const x = 4; x; }\n",
        &error
    );
    EXPECT_FALSE(ctx, result.had_error);
    EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
    EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 4.0);
    value_free(result.value);
    eval_result_free(&result);

    {
        const char *source =
            "let i = 0;\n"
            "while (i < 5) {\n"
            "  {\n"
            "    function temp() { return i; }\n"
            "  }\n"
            "  i = i + 1;\n"
            "}\n"
            "i;\n";
        TokenArray tokens;
        StmtArray program;
        Env *env;

        EXPECT_TRUE(ctx, lex_source(source, &tokens, &error));
        if (error != NULL) {
            free(error);
            error = NULL;
        }
        EXPECT_TRUE(ctx, parse_tokens(&tokens, source, &program, &error));
        if (error != NULL) {
            free(error);
            error = NULL;
        }

        env = env_create(NULL);
        result = eval_program(&program, source, env);
        EXPECT_FALSE(ctx, result.had_error);
        EXPECT_INT_EQ(ctx, result.value.type, VALUE_NUMBER);
        EXPECT_DOUBLE_EQ(ctx, result.value.as.number, 5.0);
        EXPECT_SIZE_EQ(ctx, gc_live_function_count(), 3U);
        EXPECT_SIZE_EQ(ctx, gc_live_env_count(), 1U);

        value_free(result.value);
        eval_result_free(&result);
        env_destroy(env);
        gc_collect_with_reason(NULL, NULL, "test-finalize");
        EXPECT_SIZE_EQ(ctx, gc_live_function_count(), 0U);
        EXPECT_SIZE_EQ(ctx, gc_live_env_count(), 0U);
        stmt_array_free(&program);
        token_array_free(&tokens);
    }

    {
        const char *source =
            "function makeState(seed) {\n"
            "  let box = {value: seed};\n"
            "  return function(step) {\n"
            "    box.value = box.value + step;\n"
            "    return [{box: box}, {next: box.value + 1}];\n"
            "  };\n"
            "}\n"
            "let worker = makeState(1);\n"
            "worker(2);\n";
        TokenArray tokens;
        StmtArray program;
        Env *env;
        GcStatsSnapshot stats;

        gc_reset_stats();
        EXPECT_TRUE(ctx, lex_source(source, &tokens, &error));
        if (error != NULL) {
            free(error);
            error = NULL;
        }
        EXPECT_TRUE(ctx, parse_tokens(&tokens, source, &program, &error));
        if (error != NULL) {
            free(error);
            error = NULL;
        }

        env = env_create(NULL);
        result = eval_program(&program, source, env);
        EXPECT_FALSE(ctx, result.had_error);
        EXPECT_INT_EQ(ctx, result.value.type, VALUE_ARRAY);

        gc_get_stats(&stats);
        EXPECT_TRUE(ctx, stats.collections_total > 0U);
        EXPECT_STRING_EQ(ctx, stats.last_reason, "eval-safe-point");
        EXPECT_TRUE(ctx, stats.temp_roots_high_water > 0U);
        EXPECT_TRUE(ctx, stats.live_envs >= 2U);
        EXPECT_TRUE(ctx, stats.live_functions >= 1U);
        EXPECT_TRUE(ctx, stats.live_arrays >= 1U);
        EXPECT_TRUE(ctx, stats.live_objects >= 1U);
        EXPECT_SIZE_EQ(ctx, gc_live_array_count(), stats.live_arrays);
        EXPECT_SIZE_EQ(ctx, gc_live_object_count(), stats.live_objects);

        value_free(result.value);
        eval_result_free(&result);
        env_destroy(env);
        gc_collect_with_reason(NULL, NULL, "test-finalize");
        gc_get_stats(&stats);
        EXPECT_STRING_EQ(ctx, stats.last_reason, "test-finalize");
        EXPECT_SIZE_EQ(ctx, stats.live_envs, 0U);
        EXPECT_SIZE_EQ(ctx, stats.live_functions, 0U);
        EXPECT_SIZE_EQ(ctx, stats.live_arrays, 0U);
        EXPECT_SIZE_EQ(ctx, stats.live_objects, 0U);
        EXPECT_TRUE(ctx, stats.swept_envs > 0U);
        EXPECT_TRUE(ctx, stats.swept_functions > 0U);
        EXPECT_TRUE(ctx, stats.swept_arrays > 0U);
        EXPECT_TRUE(ctx, stats.swept_objects > 0U);
        stmt_array_free(&program);
        token_array_free(&tokens);
    }

    assert_example_output(ctx, "examples/arithmetic.js", "7\n");
    assert_example_output(ctx, "examples/branching.js", "20\n");
    assert_example_output(ctx, "examples/functions.js", "7\n");
    assert_example_output(ctx, "examples/strings.js", "hello, runtime!\n");
    assert_example_output(ctx, "examples/arrays_objects.js", "jsrt:3:runtime\n");
    assert_example_output(ctx, "examples/loops_logic.js", "jsrt\n");
    assert_example_output(ctx, "examples/assignment.js", "1:new:7\n");
    assert_example_output(ctx, "examples/for_loops.js", "8:6\n");
    assert_example_output(ctx, "examples/do_while.js", "8\n");
    assert_example_output(ctx, "examples/switch.js", "5\n");
    assert_example_output(ctx, "examples/function_expressions.js", "11\n");
    assert_example_output(ctx, "examples/const.js", "7\n");
    assert_example_output(ctx, "examples/references.js", "5:9:7\n");
    assert_example_output(ctx, "examples/builtins.js", "object:5:3\n");
    assert_example_output(ctx, "examples/gc_stress.js", "85\n");
}

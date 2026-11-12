#include <stdlib.h>
#include <string.h>

#include "jsrt/runtime.h"
#include "test.h"

static int runtime_interrupt_calls = 0;

static bool runtime_interrupt_after_two_calls(void *user_data) {
    (void)user_data;
    runtime_interrupt_calls += 1;
    return runtime_interrupt_calls > 2;
}

static void write_runtime_test_file(TestContext *ctx, const char *path, const char *source) {
    FILE *file = fopen(path, "w");
    EXPECT_TRUE(ctx, file != NULL);
    if (file != NULL) {
        fputs(source, file);
        fclose(file);
    }
}

void run_runtime_tests(TestContext *ctx) {
    JsrtRuntime *runtime = jsrt_runtime_create();
    JsrtRuntime *second;
    JsrtRunResult result;

    EXPECT_TRUE(ctx, runtime != NULL);
    EXPECT_INT_EQ(ctx, JSRT_VERSION_MAJOR, 0);
    EXPECT_INT_EQ(ctx, JSRT_VERSION_MINOR, 2);
    EXPECT_INT_EQ(ctx, JSRT_VERSION_PATCH, 0);
    EXPECT_STRING_EQ(ctx, jsrt_runtime_version(), "0.2.0");

    result = jsrt_runtime_eval_source(runtime, "let x = 40; x + 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.has_value);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    /* Bindings survive between eval calls on one runtime. */
    result = jsrt_runtime_eval_source(runtime, "x + 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.has_value);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    result = jsrt_runtime_eval_source(runtime, "missing;\n", "embedded-error.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.error_message != NULL);
    EXPECT_TRUE(ctx, strstr(result.error_message, "undefined variable 'missing'") != NULL);
    EXPECT_INT_EQ(ctx, result.runtime_error_kind, RUNTIME_ERROR_NAME);
    EXPECT_INT_EQ(ctx, result.error_line, 1);
    EXPECT_INT_EQ(ctx, result.error_column, 1);
    jsrt_run_result_free(&result);

    /* A failed eval leaves the environment usable. */
    result = jsrt_runtime_eval_source(runtime, "x * 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "80");
    jsrt_run_result_free(&result);

    /* Functions defined by an earlier eval stay callable. The parsed program
     * has to outlive the eval call for the function body pointers to remain
     * valid. */
    result = jsrt_runtime_eval_source(runtime, "function add(a, b) { return a + b; }\n", "define.js");
    EXPECT_TRUE(ctx, result.ok);
    jsrt_run_result_free(&result);

    result = jsrt_runtime_eval_source(runtime, "add(20, 22);\n", "call.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    /* Closures keep their captured environment across evals too. */
    result = jsrt_runtime_eval_source(
        runtime,
        "function makeCounter() {\n"
        "  let count = 0;\n"
        "  return function() { count = count + 1; return count; };\n"
        "}\n"
        "let counter = makeCounter();\n",
        "counter.js"
    );
    EXPECT_TRUE(ctx, result.ok);
    jsrt_run_result_free(&result);
    result = jsrt_runtime_eval_source(runtime, "counter();\n", "counter.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "1");
    jsrt_run_result_free(&result);
    result = jsrt_runtime_eval_source(runtime, "counter();\n", "counter.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "2");
    jsrt_run_result_free(&result);

    result = jsrt_runtime_eval_source(NULL, "1;\n", "embedded.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.error_message != NULL);
    jsrt_run_result_free(&result);

    result = jsrt_runtime_eval_source(runtime, NULL, "embedded.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.error_message != NULL);
    jsrt_run_result_free(&result);

    result = jsrt_runtime_eval_file(runtime, "build/missing-runtime-file.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_INT_EQ(ctx, result.runtime_error_kind, RUNTIME_ERROR_LOOKUP);
    jsrt_run_result_free(&result);

    write_runtime_test_file(ctx, "build/runtime_module.js", "export const value = 7;\n");
    write_runtime_test_file(ctx, "build/runtime_main.js", "import { value } from \"./runtime_module.js\";\nvalue * 6;\n");
    result = jsrt_runtime_eval_file(runtime, "build/runtime_main.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.has_value);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    /* A function exported by a module keeps working after the eval that
     * imported it returns. The module record is gone by then, so the function
     * has to keep its parsed program alive through its environment. */
    write_runtime_test_file(ctx, "build/runtime_exports.js", "export function twice(value) { return value * 2; }\n");
    write_runtime_test_file(
        ctx,
        "build/runtime_import.js",
        "import { twice } from \"./runtime_exports.js\";\ntwice(1);\n"
    );
    result = jsrt_runtime_eval_file(runtime, "build/runtime_import.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "2");
    jsrt_run_result_free(&result);
    result = jsrt_runtime_eval_source(runtime, "twice(21);\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    /* Importing one module twice in the same eval reuses the cached module
     * record. Its environment has to survive the garbage collection that runs
     * between the two import statements. */
    write_runtime_test_file(ctx, "build/runtime_shared.js", "export const n = 1;\n");
    write_runtime_test_file(
        ctx,
        "build/runtime_double_import.js",
        "import { n as c } from \"./runtime_shared.js\";\n"
        "import { n as d } from \"./runtime_shared.js\";\n"
        "c + d;\n"
    );
    result = jsrt_runtime_eval_file(runtime, "build/runtime_double_import.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "2");
    jsrt_run_result_free(&result);

    /* Cyclic module graphs fail with the documented diagnostic instead of
     * re-reading files until a path limit stops the process. */
    write_runtime_test_file(ctx, "build/runtime_cycle_a.js", "import { b } from \"./runtime_cycle_b.js\";\nexport const a = 1;\n");
    write_runtime_test_file(ctx, "build/runtime_cycle_b.js", "import { a } from \"./runtime_cycle_a.js\";\nexport const b = 2;\n");
    result = jsrt_runtime_eval_file(runtime, "build/runtime_cycle_a.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.error_message != NULL);
    EXPECT_TRUE(ctx, strstr(result.error_message, "cyclic module import is unsupported") != NULL);
    jsrt_run_result_free(&result);

    /* A second runtime must not sweep the first runtime's globals when it
     * allocates and collects garbage. */
    second = jsrt_runtime_create();
    EXPECT_TRUE(ctx, second != NULL);
    result = jsrt_runtime_eval_source(second, "let y = 1; let z = [1, 2, 3];\n", "second.js");
    EXPECT_TRUE(ctx, result.ok);
    jsrt_run_result_free(&result);
    result = jsrt_runtime_eval_source(runtime, "x + 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    jsrt_runtime_destroy(second);
    result = jsrt_runtime_eval_source(runtime, "x - 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "38");
    jsrt_run_result_free(&result);

    /* The runtime API exposes the host interrupt hook. */
    runtime_interrupt_calls = 0;
    jsrt_runtime_set_interrupt(runtime, runtime_interrupt_after_two_calls, NULL);
    result = jsrt_runtime_eval_source(runtime, "let i = 0; while (i < 100000) { i = i + 1; } i;\n", "loop.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.error_message != NULL);
    EXPECT_TRUE(ctx, strstr(result.error_message, "execution interrupted") != NULL);
    EXPECT_INT_EQ(ctx, result.runtime_error_kind, RUNTIME_ERROR_INTERNAL);
    jsrt_run_result_free(&result);

    jsrt_runtime_set_interrupt(runtime, NULL, NULL);
    result = jsrt_runtime_eval_source(runtime, "x + 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    jsrt_runtime_destroy(runtime);
    jsrt_runtime_destroy(NULL);
}

#include <stdlib.h>
#include <string.h>

#include "jsrt/runtime.h"
#include "test.h"

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
    JsrtRunResult result;

    EXPECT_TRUE(ctx, runtime != NULL);
    EXPECT_INT_EQ(ctx, JSRT_VERSION_MAJOR, 0);
    EXPECT_INT_EQ(ctx, JSRT_VERSION_MINOR, 1);
    EXPECT_INT_EQ(ctx, JSRT_VERSION_PATCH, 0);
    EXPECT_STRING_EQ(ctx, jsrt_runtime_version(), "0.1.0");

    result = jsrt_runtime_eval_source(runtime, "let x = 40; x + 2;\n", "embedded.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.has_value);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    result = jsrt_runtime_eval_source(runtime, "missing;\n", "embedded-error.js");
    EXPECT_FALSE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.error_message != NULL);
    EXPECT_TRUE(ctx, strstr(result.error_message, "undefined variable 'missing'") != NULL);
    EXPECT_INT_EQ(ctx, result.runtime_error_kind, RUNTIME_ERROR_NAME);
    jsrt_run_result_free(&result);

    write_runtime_test_file(ctx, "build/runtime_module.js", "export const value = 7;\n");
    write_runtime_test_file(ctx, "build/runtime_main.js", "import { value } from \"./runtime_module.js\";\nvalue * 6;\n");
    result = jsrt_runtime_eval_file(runtime, "build/runtime_main.js");
    EXPECT_TRUE(ctx, result.ok);
    EXPECT_TRUE(ctx, result.has_value);
    EXPECT_STRING_EQ(ctx, result.value_text, "42");
    jsrt_run_result_free(&result);

    jsrt_runtime_destroy(runtime);
}

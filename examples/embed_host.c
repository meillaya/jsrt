#include <stdio.h>

#include "jsrt/runtime.h"

int main(void) {
    JsrtRuntime *runtime = jsrt_runtime_create();
    JsrtRunResult result = jsrt_runtime_eval_source(runtime, "let x = 40; x + 2;\n", "embedded.js");
    int exit_code = result.ok ? 0 : 1;

    if (result.ok && result.has_value) {
        printf("%s\n", result.value_text);
    } else if (!result.ok) {
        fprintf(stderr, "%s\n", result.error_message);
    }

    jsrt_run_result_free(&result);
    jsrt_runtime_destroy(runtime);
    return exit_code;
}

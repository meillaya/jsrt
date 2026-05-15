#ifndef JSRT_RUNTIME_H
#define JSRT_RUNTIME_H

#include <stdbool.h>

#include "jsrt/eval.h"

#define JSRT_VERSION_MAJOR 0
#define JSRT_VERSION_MINOR 1
#define JSRT_VERSION_PATCH 0

typedef struct JsrtRuntime JsrtRuntime;

const char *jsrt_runtime_version(void);

typedef struct {
    bool ok;
    bool has_value;
    char *value_text;
    char *error_message;
    RuntimeErrorKind runtime_error_kind;
} JsrtRunResult;

JsrtRuntime *jsrt_runtime_create(void);
void jsrt_runtime_destroy(JsrtRuntime *runtime);

JsrtRunResult jsrt_runtime_eval_source(JsrtRuntime *runtime, const char *source, const char *module_path);
JsrtRunResult jsrt_runtime_eval_file(JsrtRuntime *runtime, const char *path);
void jsrt_run_result_free(JsrtRunResult *result);

#endif

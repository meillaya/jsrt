#ifndef JSRT_RUNTIME_H
#define JSRT_RUNTIME_H

#include <stdbool.h>

#include "jsrt/eval.h"

#define JSRT_VERSION_MAJOR 0
#define JSRT_VERSION_MINOR 2
#define JSRT_VERSION_PATCH 0

typedef struct JsrtRuntime JsrtRuntime;

const char *jsrt_runtime_version(void);

typedef struct {
    bool ok;
    bool has_value;
    char *value_text;
    char *error_message;
    RuntimeErrorKind runtime_error_kind;
    int error_line;
    int error_column;
} JsrtRunResult;

typedef bool (*JsrtInterruptCallback)(void *user_data);

JsrtRuntime *jsrt_runtime_create(void);
void jsrt_runtime_destroy(JsrtRuntime *runtime);

/* Install a cancellation callback for the next eval call. The callback runs
 * between statements and between VM instructions; returning true stops the
 * program with an "execution interrupted" error. Pass NULL to clear it. The
 * callback runs on the eval thread and must not call back into jsrt. */
void jsrt_runtime_set_interrupt(JsrtRuntime *runtime, JsrtInterruptCallback callback, void *user_data);

JsrtRunResult jsrt_runtime_eval_source(JsrtRuntime *runtime, const char *source, const char *module_path);
JsrtRunResult jsrt_runtime_eval_file(JsrtRuntime *runtime, const char *path);
void jsrt_run_result_free(JsrtRunResult *result);

#endif

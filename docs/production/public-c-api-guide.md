# Public C API Guide (D3)

Status: current for the bounded source-level API on 2026-05-15.

## Version checks

```c
#include "jsrt/runtime.h"

if (JSRT_VERSION_MAJOR != 0) {
    /* incompatible major */
}
puts(jsrt_runtime_version());
```

## Lifecycle

1. Create a runtime with `jsrt_runtime_create()`.
2. Evaluate source or a file with `jsrt_runtime_eval_source()` / `jsrt_runtime_eval_file()`.
3. Read `JsrtRunResult` fields.
4. Release result-owned strings with `jsrt_run_result_free()`.
5. Destroy the runtime with `jsrt_runtime_destroy()`.

## Ownership

- `JsrtRuntime *` is owned by the caller after creation.
- `value_text` and `error_message` inside `JsrtRunResult` are owned by the result and released by `jsrt_run_result_free()`.
- Do not free result strings manually.
- Do not share one runtime across threads.

## Errors

`ok == false` means evaluation failed. `error_message` is caller-readable text. `runtime_error_kind` is meaningful for runtime failures and may be `RUNTIME_ERROR_NONE` for lex/parse failures.

## Current limits

No opaque value handles, direct object accessors, native callback registration, cancellation/fuel controls, custom allocator, host module loader, or stable binary ABI are promised in the current production subset.

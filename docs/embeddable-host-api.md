# L6 Embeddable Host API

This document records the bounded **L6 — Embeddable host API** surface.

## Public header

The public API is declared in `include/jsrt/runtime.h`.

```c
JsrtRuntime *jsrt_runtime_create(void);
void jsrt_runtime_destroy(JsrtRuntime *runtime);

JsrtRunResult jsrt_runtime_eval_source(JsrtRuntime *runtime, const char *source, const char *module_path);
JsrtRunResult jsrt_runtime_eval_file(JsrtRuntime *runtime, const char *path);
void jsrt_run_result_free(JsrtRunResult *result);
```

`JsrtRunResult` owns `value_text` on success and `error_message` on failure. Callers must release either with `jsrt_run_result_free`.

## Semantics

- The API executes the existing tree-walk evaluator surface.
- `jsrt_runtime_eval_source` accepts a virtual `module_path` so relative L5 imports can resolve from embedded source.
- `jsrt_runtime_eval_file` reads and evaluates a file path and uses that path as the module-resolution base.
- Successful non-`null` final values are returned as `value_text` using the runtime's existing stringification rules.
- Successful final `null` values set `ok = true` and `has_value = false`.
- Lexer/parser errors set `ok = false`, `error_message`, and `runtime_error_kind = RUNTIME_ERROR_NONE`.
- Runtime errors set `ok = false`, `error_message`, and the structured `RuntimeErrorKind` from the evaluator.

## Boundaries

- The L6 API is intentionally source/file evaluation only; it is not a stable ABI promise.
- The API does not expose mutable `Value` handles, native host callbacks, bytecode compilation, VM backend selection, or custom allocators.
- CLI behavior is unchanged; `src/main.c` still owns command-line execution and backend selection.

## Verification evidence

- `tests/test_runtime.c` covers source evaluation, runtime diagnostics, result ownership, and file/module evaluation.
- `examples/embed_host.c` shows a minimal embedding program.

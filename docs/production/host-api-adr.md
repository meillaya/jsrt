# Host API Production ADR (H1-H8)

Status: accepted on 2026-05-15.

## H1 — Versioning / ABI stance

`include/jsrt/runtime.h` is a source-level C API for the bounded runtime. It now exposes `JSRT_VERSION_MAJOR`, `JSRT_VERSION_MINOR`, `JSRT_VERSION_PATCH`, and `jsrt_runtime_version()` for source compatibility checks. The current project does not promise stable binary ABI across releases.

Thread-safety stance: a `JsrtRuntime` is single-thread-affine unless a future host milestone documents synchronization. Callers own result strings and must release them with `jsrt_run_result_free`.

## H2 — Opaque value handles and ownership

Opaque mutable `JsrtValue` handles are not part of the current supported API. They remain a Future surface because direct handles must define GC rooting, pinning, unwind behavior, and cross-call lifetime. Until then, host results are textual and caller-owned.

## H3 — Primitive / object / string accessors

Direct host accessors/mutators are Future. The accepted design constraint is that each accessor must return deterministic status/diagnostics and must not expose internal `Value` layout.

## H4 — Controlled eval entrypoints

Current supported entrypoints are `jsrt_runtime_eval_source` and `jsrt_runtime_eval_file`, with module path used for source location and relative module resolution. Future options structs may add fuel, backend selection, and host module callbacks without changing the current simple calls.

## H5 — Host module resolver / loader callbacks

Future. Resolver/loader callbacks must align with `docs/production/module-linker-adr.md`, preserve deterministic diagnostics, and avoid package-resolution claims unless accepted by a separate ADR.

## H6 — Native callback registration and errors

Future. Native functions must define argument/value marshaling, callback lifetime, error propagation, and GC rooting before becoming Supported.

## H7 — Cancellation / fuel / resource limits

Future. Fuel and cancellation must be checked at deterministic evaluator/VM safe points and must report a structured runtime error instead of terminating the process.

## H8 — Host job pump / ABI v1 candidate

Future. The host job pump depends on the A-series job/microtask model. An ABI v1 candidate requires hostile embedding tests, sanitizer evidence, and a frozen source compatibility guide.

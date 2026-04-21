# GC Observability and Stress Rig

This note is the execution-facing companion for roadmap milestone **M1 — GC observability and stress rig**.

## What M1 adds

The runtime now exposes a small GC observability surface without changing collection policy:
- per-collection `GcStatsSnapshot` counters in `include/jsrt/gc.h`
- live-count helpers for envs, functions, arrays, and objects
- temp-root current/high-water tracking
- optional trace output gated by `JSRT_GC_TRACE`
- a repeatable end-to-end stress fixture in `examples/gc_stress.js`

## C API summary

`GcStatsSnapshot` reports:
- `collections_total`
- `live_envs`, `live_functions`, `live_arrays`, `live_objects`
- `swept_envs`, `swept_functions`, `swept_arrays`, `swept_objects`
- `temp_roots_current`, `temp_roots_high_water`
- `last_reason`

Useful helpers:
- `gc_reset_stats()` — clears counters before a focused scenario
- `gc_get_stats(&snapshot)` — reads the latest snapshot
- `gc_collect_with_reason(root_env, root_value, reason)` — runs collection with a human-readable reason tag

## CLI trace mode

Trace mode is off by default.

Build and run:

```sh
make clean && make debug
JSRT_GC_TRACE=1 ./build/jsrt examples/gc_stress.js 2> build/gc-trace.log
cat build/gc-trace.log
```

Trace lines look like:

```text
[gc] reason=eval-safe-point collections=7 live(env=...,function=...,array=...,object=...) swept(env=...,function=...,array=...,object=...) temp_roots(current=...,high_water=...)
```

The reason tag is for debugging only. Current built-in tags include:
- `eval-safe-point`
- `test-result-cleanup`
- `process-cleanup`
- `test-finalize`

## Stress coverage in repo

M1 keeps the stress rig narrow and grounded in currently supported semantics:
- closure churn
- nested array/object allocation
- loop-driven safe-point collection
- explicit cleanup verification that all managed objects can drain back to zero live counts

Coverage lives in:
- `tests/test_eval.c`
- `examples/gc_stress.js`

## Guardrails

M1 does **not**:
- change collection policy
- add allocation-triggered GC
- change aggregate copy semantics
- widen GC beyond `Env`, `JsFunction`, `JsArray`, and `JsObject`

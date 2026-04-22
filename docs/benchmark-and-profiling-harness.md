# Benchmark and Profiling Harness

This note is the execution-facing companion for roadmap milestone **L1 — Benchmark and profiling harness**.

## What L1 adds

L1 adds a repeatable timing workflow around the existing CLI without changing runtime semantics:
- end-to-end CLI timing for the tree-walk path and the VM path
- a checked-in manifest that describes which fixtures exist, what they measure, and which backends they support
- one frozen CSV schema for live runs and the baseline artifact
- a baseline capture that future work can compare against on the same machine
- a small smoke mode that checks discovery, filtering, schema shape, and failure behavior

L1 does **not** add internal phase timing, parser/bytecode instrumentation, or runtime profiling hooks.

## Manifest contract

The benchmark manifest is the source of truth for discovery and filtering.

Required fields:
- `fixture` — repo-relative JavaScript file path
- `category` — benchmark group such as `arithmetic_control`, `calls_closures`, `aggregates`, or `gc_churn`
- `supported_backends` — `treewalk`, `vm`, or `treewalk|vm`
- `compare` — `yes` or `no`
- `expected_stdout` — exact stdout used when a smoke check needs output matching
- `smoke_expectation` — a lighter fallback such as `exit_zero` when exact stdout is not the contract

Practical rules:
- Only fixtures whose manifest entry explicitly allows a backend may run on that backend.
- Only fixtures whose manifest entry explicitly allows comparison may appear in compare runs.
- If benchmark file names evolve, keep the manifest fields and filtering rules stable.
- The manifest should stay dependency-free and easy to read by shell tooling.

## CSV schema

All benchmark output uses this exact header and column order:

```text
mode,fixture,category,backend,compare,iterations,warmups,total_ms,mean_ms,min_ms,max_ms,status,stdout_match
```

Column meaning:
- `mode` — `bench`, `bench-vm`, or `bench-compare`
- `fixture` — repo-relative JS fixture path
- `category` — manifest category
- `backend` — `treewalk` or `vm`
- `compare` — manifest value, unchanged
- `iterations` — measured iterations
- `warmups` — warmup iterations excluded from timing
- `total_ms` — total elapsed time in milliseconds
- `mean_ms` — mean measured run time in milliseconds
- `min_ms` — minimum measured run time in milliseconds
- `max_ms` — maximum measured run time in milliseconds
- `status` — `ok` or `error`
- `stdout_match` — `yes`, `no`, or `na`

Notes:
- All timing values are milliseconds.
- The harness emits one row per executed fixture/backend pair.
- The checked-in baseline artifact must use the same schema.

## Sample commands

These commands describe the intended L1 workflow:

```sh
make bench-smoke
make bench ITERATIONS=5 WARMUPS=1
make bench-vm ITERATIONS=5 WARMUPS=1
make bench-compare ITERATIONS=5 WARMUPS=1
```

Use the command that matches the question you are answering:
- `bench` for tree-walk timing
- `bench-vm` for VM timing
- `bench-compare` for same-fixture backend comparison only
- `bench-smoke` for harness validation, not timing claims

## Sample output

```text
mode,fixture,category,backend,compare,iterations,warmups,total_ms,mean_ms,min_ms,max_ms,status,stdout_match
bench,benchmarks/arithmetic-control.js,arithmetic_control,treewalk,no,5,1,23.40,4.68,4.61,4.75,ok,na
```

## Guardrails

L1 is intentionally narrow:
- coarse end-to-end CLI timing only
- same-machine relative baselines only
- no phase attribution and no cross-machine comparison claims
- no runtime instrumentation in the interpreter or VM
- no timing assertions in `make test`
- no benchmark dependency stack beyond the existing shell and CLI

Profiling stays external to the harness. L1 standardizes repeatable timing capture; it does not promise a portable profiler workflow.

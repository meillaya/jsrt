# jsrt Ownership and Lifetime Guardrails

This document records the current ownership and lifetime model used by `jsrt`. It describes what is true today so later milestones can change it deliberately instead of implicitly.

## Core rule

`jsrt` now uses a **bounded managed heap** for selected runtime objects, but it is still **not** a full general collector for every allocation in the process.

Managed-heap scope in N8:
- `Env`
- `JsFunction`
- `JsArray`
- `JsObject`

Still outside GC scope:
- source buffers
- tokens / token lexemes
- AST nodes / parser-owned strings
- diagnostic strings
- ordinary runtime strings stored in `Value`

## Token / source / AST ownership

| Structure | Owner | Release path |
|---|---|---|
| Source buffer | caller / `main` | `free(source)` in `src/main.c` |
| `TokenArray` | token array owns token lexemes | `token_array_free` |
| AST expressions/statements | AST containers own nested nodes and strings | `expr_destroy`, `stmt_destroy`, `stmt_array_free` |

## Value ownership

| Value kind | Ownership behavior |
|---|---|
| Number / bool / null | plain value, no heap payload |
| String | `Value` owns heap string and frees it in `value_free` |
| Function | managed-heap object traced through reachable env/value roots; built-ins are native functions with no captured closure |
| Array | managed-heap aggregate traced through reachable env/value roots |
| Object | managed-heap aggregate traced through reachable env/value roots |

Important current rule:
- `value_clone` deep-copies strings.
- `value_clone` preserves function, array, and object identity instead of deep-copying managed aggregates.

## Environment ownership

- `Env` instances form lexical parent chains.
- `env_define` and `env_assign` still store cloned strings/scalars, but preserve array/object identity.
- `env_get` still returns an owned `Value`, but arrays/objects keep shared identity instead of being deep-copied.
- `env_destroy` releases the caller's explicit handle; actual reclamation happens on the next collector pass.

Consequence:
- environment reads/writes no longer isolate array/object aliases; aggregate mutation is shared intentionally.

## Closure / GC interaction

- Functions retain the environment where they were declared.
- The collector now owns a bounded managed heap for `Env`, `JsFunction`, `JsArray`, and `JsObject`.
- `gc_collect(root_env, root_value)` clears marks, traces from the active env plus explicit temporary/value roots, then sweeps unreachable managed objects.
- Collection still runs only at explicit safe points; there is no allocation-triggered GC or conservative C-stack scanning.
- Evaluator code uses a small explicit temporary-root stack when a managed value must survive nested evaluation before it is stored in an env/object/array.
- Current runtime shape: `src/eval.c` still has a small `collect_garbage` helper so active lexical scopes can be pushed as temporary roots, but it delegates the real collection pass into `gc_collect_with_reason(...)` in `src/gc.c`.

## N8 execution guardrails

The source of truth for the bounded general-heap milestone is:
- `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md`
- `.omx/plans/prd-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`

When N8 changes this document, it must keep these constraints explicit:
- only `Env`, `JsFunction`, `JsArray`, and `JsObject` move onto the managed path
- AST/source/tokens/diagnostic buffers stay manually owned
- aggregate copy semantics stay unchanged during N8
- collection stays at explicit evaluator safe points
- any temporary-root mechanism stays bounded and explicit

## Evaluator ownership guardrails

- `EvalResult.value` is owned by the caller and must be released with `value_free`.
- `eval_result_free` only frees the error message, not the value payload.
- Control-flow helpers (`is_return`, `is_break`, `is_continue`) piggyback on `EvalResult`.
- Runtime code must free intermediate values on all success and error branches.

## CLI cleanup baseline

Normal execution currently cleans up in this order:
1. free final `EvalResult.value`
2. free `EvalResult.error_message`
3. destroy root env if present
4. run `gc_collect(NULL, NULL)` to sweep remaining unreachable managed-heap objects
5. free AST
6. free tokens
7. free source / parse error buffers

## What later milestones must not assume

Later work must not assume:
- all runtime heap objects are GC-managed,
- environment mutation can safely bypass the existing `Value` ownership conventions,
- VM/backend work can ignore current ownership invariants.

If any milestone changes one of those assumptions, it should update this document and add matching regression evidence.

# N8 General Heap GC MVP Notes

This document is the implementation-facing review and documentation companion for Roadmap milestone **N8 — General Heap GC MVP**. It does not replace the ADR/PRD/test-spec; it condenses the repo-local facts those artifacts depend on.

## Source of truth

- Architecture: `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md`
- Delivery contract: `.omx/plans/prd-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
- Verification contract: `.omx/plans/test-spec-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
- Context snapshot: `.omx/context/javascript-runtime-n8-gc-mvp-20260421T034700Z.md`

## Current code-review findings

### 1. Collector entry is still split, but no longer diverges on mark/sweep
- `src/gc.c` provides the canonical `gc_collect_with_reason(root_env, root_value, reason)` flow.
- `src/eval.c` still keeps a small `collect_garbage(evaluator, root_value)` helper so active lexical scopes can be pushed as temporary GC roots.
- That helper now delegates into the canonical collector instead of duplicating clear/mark/sweep logic.
- The remaining split is about evaluator root preparation, not a second collector implementation.

### 2. The root model is already explicit and bounded
- Active lexical scopes are already tracked through `Evaluator.active_envs` in `src/eval.c`.
- Both the evaluator-local helper and `gc_collect` already accept an optional transient `root_value`.
- That means N8 can stay within the ADR guardrail of explicit safe-point rooting instead of widening into conservative stack scanning.

### 3. Arrays/objects already participate in reachability traversal, but not managed ownership
- `value_mark_reachable()` in `src/value.c` already traverses arrays and objects recursively.
- `value_clone()` still deep-copies arrays/objects, and environment reads/writes still clone aggregates.
- This is the current semantics baseline N8 must preserve while moving ownership onto a bounded managed path.

## Documentation guardrails for N8

Any user-facing docs update for N8 should keep these points explicit:
- N8 is **not** full heap GC for the whole runtime.
- The bounded managed path covers only `Env`, `JsFunction`, `JsArray`, and `JsObject`.
- AST/source/tokens/diagnostic buffers remain under explicit/manual ownership.
- Aggregate behavior remains copy-isolated across environment boundaries during N8.
- If bounded temporary roots are not sufficient, the milestone must stop and report against the ADR abort criteria.

## Verification shape to keep paired with doc changes

```sh
make clean && make test
./build/jsrt examples/arithmetic.js
./build/jsrt examples/branching.js
./build/jsrt examples/functions.js
./build/jsrt examples/strings.js
./build/jsrt examples/arrays_objects.js
./build/jsrt examples/loops_logic.js
./build/jsrt examples/assignment.js
./build/jsrt examples/for_loops.js
./build/jsrt examples/do_while.js
./build/jsrt examples/switch.js
./build/jsrt examples/function_expressions.js
./build/jsrt examples/const.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Review summary

The repo already has the right bounded root model and aggregate semantics baseline for N8, but it still needs the canonical collector-entry cleanup identified by the ADR. Documentation should describe N8 as a bounded migration, not as broad/full heap collection.

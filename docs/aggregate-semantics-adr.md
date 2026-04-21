# Aggregate Semantics Decision (M2)

This note is the execution-facing companion for roadmap milestone **M2 — aggregate semantics ADR (`copy` vs `reference`)**.

## Decision summary
The roadmap now chooses **shared-reference semantics for arrays and objects** as the target direction for future runtime work.

Current runtime behavior is still unchanged today:
- arrays/objects are copied on environment read/write boundaries
- alias mutation is intentionally isolated
- property/index assignment rewrites and rebinds the aggregate root

That current behavior remains the active baseline until **M3** implements the migration.

## Why M2 chooses reference-style aggregates
The decision is based on repo-local evidence:
- `src/env.c` clones values on define/assign/get
- `src/value.c` deep-clones arrays/objects in `value_clone`
- `src/eval.c` implements aggregate assignment as read-clone-mutate-write-back
- `tests/test_eval.c` explicitly proves alias isolation today
- N8/M1 already established the bounded managed heap and GC observability needed to revisit aggregate identity safely

## Compatibility matrix

| Topic | Current baseline | Chosen direction for M3 |
|---|---|---|
| aliasing (`let alias = arr`) | isolated copy | shared aggregate identity |
| nested mutation | local to rewritten root binding | visible through aliases |
| env read/write behavior | clone arrays/objects | preserve aggregate identity |
| property/index assignment | clone/mutate/rebind root | mutate shared aggregate in place |
| equality | deep structural | keep structural initially unless M3 proves otherwise |

## Guardrails
M2 does **not**:
- change runtime code
- widen GC beyond `Env`, `JsFunction`, `JsArray`, and `JsObject`
- redefine string ownership or AST/token/source ownership
- promise full JavaScript compatibility

## M3 handoff
M3 is now the required follow-on milestone.
It should:
- preserve aggregate identity across env/binding flow
- update aliasing and nested-mutation tests/examples/docs
- keep the migration bounded to aggregate semantics rather than broad language redesign

The source of truth for the decision is:
- `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md`
- `.omx/plans/prd-javascript-runtime-roadmap-m2-aggregate-semantics-adr.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m2-aggregate-semantics-adr.md`

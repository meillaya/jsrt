# PRD — Roadmap N8: General Heap GC MVP

## Goal
Implement a bounded managed-heap MVP that extends jsrt beyond closure/environment cycle collection while preserving current language semantics and keeping rollback risk low.

## Architectural source of truth
Use `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md` as the decision record for this milestone.

## In Scope
- canonicalize GC orchestration behind one collector entry flow
- introduce a managed-heap path for selected runtime objects only:
  - `Env`
  - `JsFunction`
  - `JsArray`
  - `JsObject`
- preserve current copy-isolated aggregate semantics
- keep collection at explicit evaluator safe points
- add the smallest explicit temporary-root mechanism needed for correctness
- tests and stress-style fixtures sufficient to prove correctness
- docs updates for ownership/GC behavior

## Out of Scope
- allocation-triggered collection
- conservative stack scanning
- frontend object management (AST, source buffers, tokens, diagnostic strings)
- aggregate semantic changes (`copy` vs `reference`)
- VM/backend work
- string interning or string GC migration

## Deliverables
1. Canonical collector entry path used consistently by evaluator/runtime.
2. Managed-heap infrastructure for the selected runtime object set.
3. Correct reachability/root handling for envs, closures, arrays, objects, and transient values required by current evaluation.
4. Updated ownership/GC docs.
5. Regression/stress evidence proving current semantics still hold.

## Acceptance Criteria
1. `Env`, `JsFunction`, `JsArray`, and `JsObject` are on the bounded managed path chosen by the ADR.
2. Existing semantics remain unchanged, especially aggregate copy behavior.
3. Collector entry/sweep logic is no longer duplicated/divergent.
4. `make clean && make test` passes.
5. ASan verification passes.
6. Existing examples stay green unless explicitly updated.

## Required constraints
- do not widen GC scope beyond the selected runtime objects
- do not change aggregate aliasing/reference semantics
- do not pull AST/source/tokens into managed-heap scope
- if temporary-root coverage cannot be made correct with a bounded design, stop and report against the ADR abort criteria

## Suggested Execution Mode
- Preferred: `team`
- Rationale: cross-cutting runtime change spanning value/env/gc/eval/tests/docs with high rollback sensitivity

## Suggested Team Lanes
- **Lane A — runtime core**: collector entry path + heap object model
- **Lane B — env/function migration**: closure/env correctness and root safety
- **Lane C — array/object migration**: managed path while preserving copy semantics
- **Lane D — verification**: regression, ASan, stress evidence

## Verification Shape
- fresh `make clean && make test`
- fresh example sweep of visible runtime programs
- fresh ASan run via existing CFLAGS override path
- targeted stress fixtures for nested closures / nested aggregates / mutation via assignment / recursive function calls
- explicit comparison against N1 semantics baseline and N7 ADR constraints

## Risks
- missing temporary roots for partially constructed arrays/objects/function-call arguments
- accidental semantic drift in aggregate copy behavior
- incomplete rollback boundary if too many runtime paths change at once

## Risk Mitigations
- phase the work by object family
- keep strings/manual frontend buffers out of scope
- validate against examples and semantics baseline after each major slice
- stop if N7 abort criteria are hit

## Exit Condition
jsrt has a bounded managed-heap MVP for the selected runtime object set, verified green, and still semantically aligned with the N1 baseline.

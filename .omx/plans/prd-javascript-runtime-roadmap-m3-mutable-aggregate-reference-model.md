# PRD — Roadmap M3: Mutable Aggregate/Reference Model

## Goal
Implement the M2 decision so arrays and objects behave as shared-reference aggregates instead of copy-isolated values.

## Why this milestone exists
M2 explicitly chose shared-reference aggregate semantics as the target direction. Until M3, the runtime still clones arrays/objects across environment boundaries, which keeps aliasing intentionally isolated and leaves the M2 decision unimplemented.

## In Scope
- preserve array/object identity across environment define/assign/get flows
- make alias-visible mutation work for variables, nested property/index access, function arguments, and function returns
- keep strings/scalars value-like and functions identity-preserving
- keep GC scope bounded to `Env`, `JsFunction`, `JsArray`, and `JsObject`
- update current-facing docs/tests/examples to describe and prove the new semantics
- harden aggregate equality/stringification enough that self-referential aggregates do not recurse forever

## Out of Scope
- built-in library work
- broader ECMAScript equality/coercion changes beyond what is required to keep M3 safe
- new GC object families or allocation-triggered collection
- prototype/class/module behavior
- VM/backend work

## Deliverables
1. Runtime changes in `env`/`value`/`eval` that preserve aggregate identity instead of cloning arrays/objects.
2. Regression coverage proving alias-visible mutation through bindings, nested properties, and function call/return flow.
3. Example coverage demonstrating the new reference semantics.
4. Updated current-facing docs/baseline artifacts.

## Acceptance Criteria
1. `let alias = arr; alias[0] = 99; arr[0];` now evaluates to `99`.
2. Shared aggregate identity survives function argument passing and returns.
3. Nested object/array aliases reflect mutation through every shared reference.
4. Aggregate stringification/equality no longer recurse forever on self-referential structures.
5. `make clean && make test` passes.
6. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: implementation-heavy but still small enough for a single-owner inspect/edit/verify loop

## Suggested Lanes
- `executor` — runtime + docs/test implementation
- `debugger` — cycle/ownership regression diagnosis if aliasing breaks GC assumptions
- `verifier` — final evidence pass

## Primary Risks
- introducing aggregate aliasing bugs that break existing assignment/property/index behavior
- exposing cyclic aggregate cases that previously could not occur
- accidentally widening M3 into a broader equality or GC redesign

## Risk Mitigations
- keep strings/scalars unchanged
- preserve the current bounded GC/root model
- add narrow regression coverage for aliasing and self-reference behavior
- stop escalation if M3 starts requiring broader GC/runtime architecture changes

## Dependencies
- `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md`
- current baseline artifacts in `docs/`

## Exit Condition
The runtime's current semantics match the M2 decision: arrays/objects are shared-reference aggregates, and the repo has tests/docs/examples proving it.

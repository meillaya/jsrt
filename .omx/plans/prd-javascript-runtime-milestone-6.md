# PRD — JavaScript Runtime Milestone 6: Closure Cycle Collection

## Goal
Replace the current shutdown-only force-destroy workaround with a small tracing collector that can reclaim unreachable closure/environment cycles while preserving the interpreter's learning-first architecture.

## In Scope
- tracing collection for `Env` / `JsFunction` graphs
- safe reachability marking from active environments and live statement results
- collector-trigger points after statement/block/call execution
- removal of the explicit final force-destroy sweep from normal runtime/test cleanup
- tests that prove unreachable closure cycles are reclaimed while escaping closures still work

## Out of Scope
- full heap GC for every allocation in the interpreter
- moving AST/token/source buffers under GC control
- object/property mutation semantics
- bytecode / VM work

## Acceptance Criteria
1. Unreachable closure/environment cycles are reclaimed without relying on `env_destroy_all()`.
2. Escaping closures remain alive and callable after intermediate GC passes.
3. Existing language behavior remains unchanged.
4. `make clean && make test` passes.

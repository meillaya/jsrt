# PRD — Roadmap M4: Built-in Library Foundation

## Goal
Introduce a minimal built-in function mechanism so the runtime can expose a tiny standard library without broad host embedding.

## Why this milestone exists
After M3, the runtime has a clearer value model for aggregates and functions. M4 uses that footing to add a bounded native-call surface for a few globally available helpers before larger host/runtime work begins.

## In Scope
- a native-call abstraction for globally defined built-in functions
- `print(value)` for observable output during evaluation
- `len(value)` for strings, arrays, and objects
- `typeOf(value)` for basic runtime introspection
- source-based diagnostics for wrong arity or unsupported built-in arguments
- tests, example coverage, and current-facing docs

## Out of Scope
- general host embedding APIs
- I/O beyond the bounded `print` helper
- mutation-oriented built-ins or aggregate iteration helpers
- module loading, prototypes/classes, or broader standard library breadth

## Deliverables
1. Built-in function representation and call path integrated into the current evaluator.
2. Globally available `print`, `len`, and `typeOf`.
3. Tests and example coverage for successful calls and misuse diagnostics.
4. Updated docs/baseline text describing the built-in surface.

## Acceptance Criteria
1. Built-in functions can be called like ordinary functions from user code.
2. `print(value)` writes a stringified line and returns `null`.
3. `len(value)` works for strings, arrays, and objects.
4. `typeOf(value)` returns stable type labels for current runtime values.
5. Wrong arity and invalid built-in inputs fail with source-based runtime diagnostics.
6. `make clean && make test` passes.
7. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded feature slice with a tight implementation/verification loop

## Suggested Lanes
- `executor` — runtime/tests/docs implementation
- `writer` — doc polish if needed
- `verifier` — final acceptance-evidence pass

## Primary Risks
- mixing the native-call path too tightly with user-defined function semantics
- broadening built-ins beyond the intended minimal foundation
- letting built-in output break CLI/example expectations

## Risk Mitigations
- keep built-ins as ordinary callable globals with explicit fixed arity
- add only the three bounded helpers needed for M4
- keep `print` returning `null` so CLI final-value behavior stays predictable

## Dependencies
- `.omx/plans/prd-javascript-runtime-roadmap.md`
- `.omx/plans/prd-javascript-runtime-roadmap-m3-mutable-aggregate-reference-model.md`

## Exit Condition
The runtime has a small, well-tested built-in library foundation that future milestones can extend without introducing a full host-embedding surface.

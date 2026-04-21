# PRD — Roadmap M10: VM Closure + Function-Expression Parity

## Goal
Close the biggest remaining VM/backend gap by adding closure capture and function-expression support for the current learning-oriented surface.

## Why this milestone exists
M9 widened the VM to cover aggregates and structured control flow. The largest remaining backend gaps were closure/upvalue support and function expressions, which blocked parity for several existing examples/tests.

## In Scope
- compiler support for function expressions
- compiler upvalue resolution for nested functions
- VM support for captured values in compiled functions
- parity coverage for closure-producing declarations and function expressions
- updated docs/README describing the widened backend scope and its remaining caveats

## Out of Scope
- broad/full closure semantics auditing across every imaginable sibling-closure aliasing pattern
- making the VM the default backend
- optimization work

## Deliverables
1. Upvalue-aware compiler lowering.
2. VM runtime support for captured values in compiled functions.
3. Parity tests for existing closure/function-expression examples.
4. Updated roadmap/docs handoff.

## Acceptance Criteria
1. Existing closure examples such as `makeAdder` / `makeCounter` work on the VM backend.
2. Function expressions compile and execute on the VM backend for the supported subset.
3. `make clean && make test` passes.
4. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`

## Exit Condition
The VM backend no longer defers closure/function-expression parity for the current supported examples/tests.

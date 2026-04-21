# PRD — Roadmap M9: VM Surface Widening I

## Goal
Widen the opt-in VM/backend subset to cover more of the current language surface without crossing into closure/upvalue support.

## Why this milestone exists
M8 proved the VM backend can run the initial bytecode subset. The next highest-value step is to cover more real programs by adding aggregate lowering plus structured control-flow parity before attempting closure capture.

## In Scope
- bytecode/compiler lowering for:
  - array literals
  - object literals
  - index/property access
  - property/index assignment
  - `do...while`
  - `switch`
  - `break`
  - `continue`
- VM execution support for the new aggregate opcodes
- parity tests and CLI example coverage for the widened surface
- updated docs/README describing the widened VM subset and remaining deferrals

## Out of Scope
- closure capture / upvalues
- function-expression parity for capturing cases
- full-language parity
- optimization/performance work
- making VM the default backend

## Deliverables
1. Widened compiler lowering for aggregates and deferred control flow.
2. VM opcode/runtime support for aggregate creation/access/mutation.
3. Parity tests proving the widened subset matches tree-walk behavior.
4. Current-facing docs recording the new subset and what still remains deferred.

## Acceptance Criteria
1. Arrays/objects can be created, accessed, and mutated through the VM backend.
2. `do...while`, `switch`, `break`, and `continue` work on the VM backend for the supported subset.
3. Parity tests cover the widened subset against tree-walk.
4. Unsupported closure/upvalue cases still fail explicitly.
5. `make clean && make test` passes.
6. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded widening pass with direct parity verification

## Suggested Lanes
- `executor` — compiler/VM/tests/docs
- `debugger` — parity mismatch diagnosis
- `verifier` — final evidence pass

## Primary Risks
- widening too much at once and blurring the remaining closure boundary
- introducing aggregate mutation drift between tree-walk and VM
- loop/switch control patching bugs in the compiler

## Risk Mitigations
- keep closure/upvalue cases explicit and still unsupported
- rely on parity tests for every newly widened surface
- keep tree-walk as semantic authority

## Dependencies
- `.omx/plans/prd-javascript-runtime-roadmap-m8-vm-interpreter-parity-backend.md`
- `docs/vm-parity-backend.md`

## Exit Condition
The VM backend covers a meaningfully larger slice of the existing language while still drawing a clear line around closure/upvalue work.

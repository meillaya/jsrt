# PRD — Roadmap M8: VM Interpreter Parity Backend

## Goal
Execute the M7 bytecode subset on a stack VM and prove parity against the tree-walk evaluator for that bounded surface.

## Why this milestone exists
M7 established the compiler and disassembler. M8 closes the backend loop by executing the compiled subset while keeping tree-walk as the semantic authority.

## In Scope
- stack VM execution for the M7 bytecode subset
- call frames, locals, globals, and built-in calls for the supported subset
- parity tests comparing tree-walk and VM results on representative programs
- an explicit backend switch for CLI/runtime validation
- docs/README updates describing current VM scope and deferrals

## Out of Scope
- closures/upvalues
- aggregate lowering/parity
- full-language parity
- optimization/JIT work
- making VM the default backend

## Deliverables
1. VM execution engine for the bounded compiler subset.
2. Tree-walk vs VM parity tests.
3. CLI/backend switch for bounded VM execution.
4. Current-facing docs describing the VM subset and current deferrals.

## Acceptance Criteria
1. Supported subset programs execute successfully on the VM backend.
2. Parity tests prove matching results against tree-walk for the bounded subset.
3. Unsupported backend-surface features still fail explicitly rather than misbehave silently.
4. `make clean && make test` passes.
5. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded implementation with direct verification

## Suggested Lanes
- `executor` — VM/runtime/tests/docs
- `debugger` — parity mismatch diagnosis
- `verifier` — final parity evidence pass

## Primary Risks
- drift between tree-walk and VM semantics
- scope creep into unsupported backend surface
- runtime memory/stack cleanup errors in the VM path

## Risk Mitigations
- keep tree-walk authoritative
- constrain M8 to the M7 subset
- add parity tests and ASan coverage before widening surface

## Dependencies
- `.omx/plans/prd-javascript-runtime-roadmap-m7-bytecode-compiler-mvp.md`
- `docs/bytecode-compiler-mvp.md`

## Exit Condition
The repo has a bounded, working VM backend with parity evidence for the initial subset and clear guards around what remains deferred.

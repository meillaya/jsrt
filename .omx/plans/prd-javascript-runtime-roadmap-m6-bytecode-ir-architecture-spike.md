# PRD — Roadmap M6: Bytecode / IR Architecture Spike

## Goal
Define a credible, bounded path from the current tree-walk evaluator to a compiled backend.

## Why this milestone exists
The runtime is now mature enough to support backend planning:
- GC/heap scope is bounded and explicit
- aggregate semantics are clarified
- built-ins exist as a small callable surface
- runtime error reporting is more structured

M6 exists to decide whether and how M7/M8 should proceed without forcing implementation prematurely.

## In Scope
- backend architecture choice
- instruction set draft
- compiler boundary between AST and backend
- runtime stack/call-frame model
- explicit first-parity subset
- adoption/abort/rollback conditions for M7/M8
- repo-facing companion docs / README handoff

## Out of Scope
- implementing the compiler
- implementing the VM
- parser/AST redesign
- full-language parity guarantees
- optimization/JIT work

## Deliverables
1. Accepted ADR for backend direction.
2. Companion design note with instruction coverage matrix and AST-to-bytecode mapping samples.
3. M7/M8 go/no-go decision with bounded subset and rollback path.
4. README handoff pointing future work at the M6 artifact set.

## Acceptance Criteria
1. The repo contains an ADR choosing the backend direction.
2. The artifact set defines a concrete instruction set draft.
3. The artifact set defines the initial parity subset and its explicit deferrals.
4. The artifact set records adoption criteria, abort criteria, rollback path, and whether M7/M8 proceed.
5. Repo docs point future execution at the M6 artifacts.
6. Fresh verification confirms the repo still passes unchanged code/tests after the documentation-only spike.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: design-heavy but bounded and repository-local

## Suggested Lanes
- `architect` — backend shape and guardrails
- `critic` — challenge parity/rollback assumptions
- `verifier` — acceptance evidence and artifact completeness

## Primary Risks
- selecting an over-ambitious backend target
- under-specifying the subset so M7 scope immediately balloons
- blurring architecture planning with implementation

## Risk Mitigations
- keep tree-walk as semantic authority
- define a deliberately narrow first backend subset
- defer closures/upvalues and full aggregate parity unless later evidence justifies expansion

## Dependencies
- `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md`
- `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md`
- `.omx/plans/prd-javascript-runtime-roadmap-m5-structured-runtime-error-model.md`

## Exit Condition
M7/M8 can begin from a single explicit backend plan instead of rediscovering the VM/compiler shape from scratch.

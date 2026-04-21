# PRD — Roadmap N4: `switch` / `case` / `default` MVP

## Goal
Add a small, teachable `switch` statement to the current interpreter so structured multi-branch control flow is supported without jumping to broad ECMAScript compatibility.

## Why this milestone exists
The runtime already supports `if` / `else`, loops, `break`, and `continue`. `switch` is a natural next control-flow feature that is visible to users, modest in scope, and useful for validating how statement dispatch and `break` should behave in nested control-flow contexts.

## In Scope
- `switch (expr) { ... }` statement parsing and evaluation
- `case <expr>:` labels
- `default:` label
- `break;` support inside `switch`
- explicit decision and documentation for fallthrough behavior in the MVP
- parser/evaluator tests and one end-to-end example
- diagnostics for malformed switch syntax

## Out of Scope
- labels
- `continue` targeting switch
- pattern matching
- JS-complete coercion semantics
- duplicate-case static analysis beyond simple parser/runtime handling
- switch optimizations or bytecode work

## Required Design Decision
Before implementation starts, choose and document one of these MVP semantics:
1. **C/JS-like fallthrough enabled by default**
2. **No implicit fallthrough** unless an explicit future mechanism is added

The choice must be reflected consistently in parser/eval tests, docs, and example output.

### Chosen MVP semantics
- **C/JS-like fallthrough enabled by default**
- Rationale: this keeps `switch` meaningfully distinct from `if`/`else`, aligns with user expectations for a JavaScript-like language, and gives existing `break` semantics a clear switch-specific role.

## Deliverables
1. Switch/case/default AST and parser support.
2. Evaluator support with correct `break` interaction.
3. Regression tests for normal flow, default flow, nested control-flow interaction, and invalid syntax.
4. One example script proving the chosen semantics.
5. Brief docs update describing the MVP switch behavior.

## Acceptance Criteria
1. `switch` statements parse successfully for supported syntax.
2. `case` matching and `default` execution work according to the documented MVP semantics.
3. `break` exits the `switch` body correctly without breaking outer loops unless intended by nesting rules.
4. Existing non-switch behavior remains green.
5. `make clean && make test` passes.
6. Example output is stable and documented.

## Suggested Execution Mode
- Preferred: `team`
- Rationale: cross-cutting parser/AST/evaluator/test change with a semantic decision that benefits from staged review

## Suggested Lanes
- `architect` — finalize MVP fallthrough semantics and edge-case guardrails
- `executor` — AST/parser/eval implementation
- `test-engineer` — regression matrix and example fixture
- `verifier` — fresh proof that switch semantics and existing behavior are green

## Verification Shape
- fresh `make clean && make test`
- fresh switch example run
- targeted nested control-flow tests (`switch` inside loops, loops inside switch branches where supported)
- ASan run if implementation touches sensitive evaluator/control-flow cleanup paths

## Risks
- ambiguous fallthrough semantics causing later rework
- incorrect interaction between `switch`-local `break` and loop `break`
- parser recovery regressions around `case` / `default` blocks

## Risk Mitigations
- lock semantics in the PRD before coding
- add narrow nested-control-flow tests
- prefer a minimal MVP over clever syntax sugar

## Dependencies
- N2 diagnostics harness should ideally be complete or partially available
- current `break` support from milestone 8

## Exit Condition
The project has a documented, tested switch MVP with stable semantics and no regressions to existing control-flow behavior.

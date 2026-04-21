# PRD — Roadmap N3: `do...while` Support

## Goal
Add `do...while` loops as a low-risk control-flow feature that fits the current tree-walk interpreter architecture.

## In Scope
- `do { ... } while (condition);`
- single-statement body support through existing statement parsing
- correct loop interaction with `break` and `continue`
- tests and one end-to-end example
- docs update

## Out of Scope
- labels
- `switch`
- new loop semantics beyond `do...while`
- optimizer/backend work

## Acceptance Criteria
1. `do...while` parses successfully.
2. Body executes at least once before condition check.
3. `break` and `continue` behave correctly inside `do...while`.
4. Existing loop/control-flow behavior remains green.
5. `make clean && make test` passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded syntax/eval/test/doc milestone

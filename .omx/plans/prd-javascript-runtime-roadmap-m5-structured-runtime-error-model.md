# PRD — Roadmap M5: Exceptionless Structured Runtime Error Model

## Goal
Normalize runtime error creation, categorization, and reporting so the evaluator can grow without ad hoc failure handling.

## Why this milestone exists
The runtime already reports source-based diagnostics, but runtime failures are still mostly anonymous strings assembled at many call sites. As built-ins and aggregate semantics have expanded, the evaluator now needs a clearer internal result/error shape so future milestones can reason about runtime failures consistently.

## In Scope
- add structured runtime error metadata to `EvalResult`
- categorize runtime failures with a bounded enum (type, name, arity, lookup, assignment, control-flow, index, math, internal)
- normalize evaluator diagnostics so runtime errors consistently include category labels
- add targeted tests for both the formatted diagnostics and the structured error fields
- update current-facing docs/README to describe the runtime error model

## Out of Scope
- parser or lexer error model redesign
- exception/throw/catch language features
- broad recovery after runtime failure
- VM/backend-specific error transport

## Deliverables
1. Structured runtime error metadata in `EvalResult`.
2. Categorized runtime error helpers used throughout evaluator failure paths.
3. Diagnostic regressions proving category-consistent runtime failures.
4. Updated docs describing the model and its current categories.

## Acceptance Criteria
1. Runtime failures carry a structured category in addition to the formatted diagnostic text.
2. Source-based diagnostics consistently label runtime errors with their category.
3. Existing runtime behavior remains unchanged apart from the improved error labeling/metadata.
4. `make clean && make test` passes.
5. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded internal refactor with a direct verify loop

## Suggested Lanes
- `executor` — evaluator/test/doc implementation
- `verifier` — acceptance audit and diagnostic evidence review

## Primary Risks
- accidental message churn that breaks too many tests at once
- adding categories without using them consistently
- expanding the milestone into language-level exception features

## Risk Mitigations
- keep the category set small and explicit
- preserve existing message text where possible beneath the new category label
- add direct tests for both structured fields and formatted diagnostics

## Dependencies
- `.omx/plans/prd-javascript-runtime-roadmap-m4-built-in-library-foundation.md`
- `docs/diagnostics-golden-harness.md`

## Exit Condition
Runtime errors are consistently categorized, easier to reason about internally, and still reported with source-based diagnostics externally.

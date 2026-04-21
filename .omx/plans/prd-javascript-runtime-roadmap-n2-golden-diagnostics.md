# PRD — Roadmap N2: Golden Diagnostics Harness

## Goal
Lock the current source-based lexer, parser, parser-recovery, and runtime diagnostics behind a compact golden-style harness so later milestones can change behavior deliberately instead of accidentally.

## Why this milestone exists
The runtime already emits structured diagnostics with source lines and caret markers, and parser recovery can aggregate multiple syntax failures in one run. Current tests mostly assert substrings, which is useful but does not fully freeze the user-facing diagnostics contract.

## In Scope
- add a compact diagnostics-specific test lane that checks exact or near-exact golden outputs for representative lexer/parser/runtime failures
- lock parser recovery aggregation for at least one multi-error input
- keep the harness lightweight and C-native; no external snapshot framework
- update docs/README only as needed to mention the diagnostics baseline/harness

## Out of Scope
- changing diagnostic wording or formatting unless required to make current behavior testable/consistent
- broad testing framework refactors
- new language features
- IDE/editor integration

## Deliverables
1. Milestone-specific PRD/test-spec artifacts.
2. Dedicated diagnostics regression test coverage.
3. Exact golden checks for representative lexer, parser, parser-recovery, and runtime diagnostics.
4. Minimal README or docs note if the new harness needs to be discoverable.

## Acceptance Criteria
1. There is a dedicated diagnostics test lane in the C test suite.
2. The test suite locks at least one exact lexer diagnostic, one exact parser diagnostic, one aggregated parser-recovery diagnostic, and one exact runtime diagnostic.
3. Existing tests and examples stay green.
4. `make clean && make test` passes.
5. ASan verification passes via the existing CFLAGS override path.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded hardening work with tight implementation/verification loop

## Suggested Lanes
- `executor` — add harness/tests and any minimal supporting plumbing
- `test-engineer` — validate case selection and coverage completeness
- `verifier` — confirm outputs are actually frozen and evidence is fresh

## Primary Risks
- overfitting too many diagnostics and making future intentional wording changes noisy
- underfitting and missing parser-recovery aggregation behavior
- introducing a test harness more complex than the runtime itself needs

## Risk Mitigations
- keep the golden set small and representative
- prefer exact matching only for stable human-facing diagnostics we intend to preserve
- reuse existing lex/parse/eval helpers patterns where possible

## Dependencies
- N1 semantics baseline complete

## Exit Condition
The repo has a lightweight, high-signal diagnostics harness that makes future diagnostic changes explicit and reviewable.

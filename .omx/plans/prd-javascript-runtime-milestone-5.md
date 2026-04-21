# PRD — JavaScript Runtime Milestone 5: Better Parser Recovery

## Goal
Improve parser usability by recovering after syntax errors, surfacing multiple parser diagnostics in one run, and continuing far enough to report later independent mistakes.

## In Scope
- synchronize after parse errors to the next likely statement boundary
- aggregate multiple parser diagnostics into one returned error report
- recovery inside blocks/function bodies as well as top-level parsing
- tests for multi-error parser output

## Out of Scope
- full AST partial-execution after parse failure
- IDE-quality fix suggestions
- lexer or runtime recovery changes beyond existing diagnostics

## Acceptance Criteria
1. Parser reports multiple independent syntax errors from one source input.
2. Recovery works across top-level statements and inside block parsing.
3. Existing valid-program parsing stays green.
4. `make clean && make test` passes.

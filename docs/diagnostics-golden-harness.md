# jsrt Golden Diagnostics Harness

Roadmap milestone **N2** adds a compact golden diagnostics lane to the C test suite.

## Intent

The goal is not to snapshot every possible error, but to freeze a small, representative set of user-facing diagnostics so future wording/formatting changes are deliberate.

## Golden cases currently locked

1. **Lexer**
   - invalid character diagnostic
2. **Parser**
   - single syntax error diagnostic
3. **Parser recovery**
   - aggregated multi-error diagnostic from one source input
4. **Runtime**
   - invalid top-level loop-control diagnostic

## Where it lives

- `tests/test_diagnostics.c`

## Guardrail

If a later milestone intentionally changes diagnostic wording or formatting:
1. update the relevant golden expectation,
2. explain the change in that milestone’s PRD/test-spec,
3. keep the set small and representative.

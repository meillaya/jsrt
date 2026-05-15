# Unsupported Diagnostics Policy (P3)

Status: accepted on 2026-05-15.

Unsupported ECMAScript surfaces are expected in `jsrt`; ambiguous or silent behavior is not. Diagnostics must make unsupported status actionable without pretending to be full ECMAScript compatibility.

## Policy

1. Unsupported syntax should fail in the lexer/parser when it can be recognized locally.
2. Unsupported backend surfaces should fail in the compiler/VM path with deterministic messages rather than tree-walk fallback.
3. Unsupported runtime operations should return structured runtime errors with source context where available.
4. Host API failures must surface a stable status code plus caller-owned diagnostic text.
5. Diagnostics must avoid compatibility claims such as "valid JavaScript"; use bounded-subset language.

## Message conventions

- Parser/compiler unsupported feature: `unsupported <feature> in jsrt production subset` or a milestone-specific equivalent.
- Backend unsupported feature: `unsupported <feature> on VM backend`.
- Runtime unsupported operation: structured category plus source caret through the existing diagnostics path.
- Host API unsupported operation: `JSRT_STATUS_RUNTIME_ERROR` or a more specific future status with diagnostic text.

## Regression expectation

Representative unsupported cases must be covered by at least one of:

- unit tests under `tests/`
- negative conformance fixture under `conformance/negative/`
- per-feature gate evidence in a milestone closure record

Current retained examples include const reassignment, division by zero, unexported imports, unsupported VM module syntax, arity/type/index diagnostics, and parser rejection tests.

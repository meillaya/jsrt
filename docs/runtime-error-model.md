# Runtime Error Model

This note is the execution-facing companion for roadmap milestone **M5 — exceptionless structured runtime error model**.

## Current shape

`EvalResult` now carries both:

- the formatted source-based diagnostic string shown to users, and
- structured runtime error metadata for internal reasoning and tests

Current runtime categories:

- `type`
- `name`
- `arity`
- `lookup`
- `assignment`
- `control-flow`
- `index`
- `math`
- `internal`

## Reporting contract

Runtime diagnostics now use this format:

```text
Runtime error [category] at line:column: message
```

The source line and caret marker remain unchanged.

## Guardrails

M5 is intentionally exceptionless:

- runtime failures still stop evaluation immediately
- there is no `throw` / `catch` surface
- parser/lexer diagnostics remain separate from runtime diagnostics

The purpose of the model is consistency and internal structure, not recovery semantics.

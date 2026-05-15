# Module Graph / Linker ADR (M1)

Status: accepted for production-subset planning on 2026-05-15.

## Decision

`jsrt` keeps the tree-walk module evaluator as the semantic authority for the bounded static named import/export subset. A future VM module implementation must introduce an explicit module graph/linker layer instead of compiling each file independently.

## Bounded module model

- Source form: `import { name[, imported as local] } from "./relative.js";` and declaration exports.
- Resolution: relative paths from the importing module's path; package resolution is out of scope.
- Cache: one module record per resolved path per evaluation session.
- Cycles: deterministic rejection until a partially initialized binding model is designed.
- Exports: named declaration exports only.

## Future VM lowering strategy

A VM-compatible linker must produce:

1. module initializer bytecode for each graph node,
2. an export table keyed by exported names,
3. import binding slots that read from linked module exports,
4. deterministic cycle and missing-export diagnostics before execution where possible,
5. parity fixtures that compare tree and VM behavior for the supported module subset.

## Rejected alternatives

- Silent VM fallback to tree-walk modules: rejected because it hides backend divergence.
- Dynamic import/package resolution in this lane: rejected because it changes host and filesystem authority.
- Partially initialized cyclic modules now: rejected until binding semantics and diagnostics are designed.

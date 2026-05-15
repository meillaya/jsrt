# jsrt Production Subset Specification (P1)

Status: accepted for the bounded production-subset roadmap on 2026-05-15.

This specification defines what `jsrt` may claim as production-supported. It is intentionally narrower than ECMAScript. Production-grade means deterministic behavior, documented limits, repeatable verification, explicit unsupported diagnostics, and embedder-safe ownership rules for the supported subset.

## Semantic authority

1. The tree-walk evaluator is the semantic oracle for source execution unless a later ADR explicitly promotes an implementation-independent spec or parity oracle.
2. The VM backend is opt-in and must match tree-walk observable behavior only for surfaces listed as VM-supported.
3. Unsupported syntax/runtime surfaces must fail deterministically; silent fallback or partial behavior is a release blocker.
4. Host-facing APIs must document ownership, lifetime, diagnostic, threading, and versioning rules before being promoted to supported.

## Supported language subset

The supported source subset is:

- literals: numbers, strings, booleans, and `null`
- variables: `let`, `const`, lexical blocks, assignment, and const reassignment diagnostics
- expressions: arithmetic, comparison, equality, logical operators, unary `!`/`-`, calls, property access, indexing
- control flow: `if`, `while`, `for`, `do...while`, `switch`, `break`, `continue`, and `return`
- functions: declarations, expressions, closures, arity checks, lexical capture
- aggregates: arrays and objects with shared-reference mutation semantics
- object model: explicit prototype linkage, bounded method dispatch, bounded `this`, parenthesized `new`, bounded class declarations
- modules: static named imports from string-literal file specifiers and declaration exports on the tree-walk backend
- built-ins: `print`, `len`, `typeOf`, `setPrototypeOf`, `defer`, `runJobs`, `strContains`, `strSlice`, and `regexMatch`

## Supported runtime and host subset

- CLI source-file execution on the tree-walk backend.
- Opt-in VM execution for the VM-supported language subset.
- Source and file evaluation through the current C host API in `include/jsrt/runtime.h`.
- Caller-owned textual result and diagnostic strings for the public host API.
- Bounded managed heap for environments, functions, arrays, and objects; see `docs/ownership-guardrails.md`.

## Experimental surfaces

Experimental surfaces are usable for milestone development but are not v1 compatibility promises:

- callback job queue (`defer` / `runJobs`) until the A-series async milestones define the production job model
- tiny dependency-free `regexMatch` until the R-series RegExp subset is specified and hardened
- repo-local conformance harness until the P5/P6/C-series Test262 subset protocol is complete
- benchmark, fuzz-like, sanitizer, and regression gates while Q-series policy matures

## Rejected or deferred ECMAScript surfaces

The production subset does not claim support for:

- `eval`, `with`, generators, proxies, symbols, BigInt, typed arrays, `Date`, `Map`, `Set`, JSON, reflection, modules with dynamic import/default/namespace syntax, package resolution, top-level await, timers, I/O, workers, or web APIs
- full ECMAScript `Promise`, `async` / `await`, microtask, and rejection semantics before A-series milestones land
- full ECMAScript `RegExp`, regex literals, replacement APIs, Unicode classes, or unbounded backtracking before R-series milestones land
- ABI stability, native callbacks, opaque mutable value handles, custom allocators, resource limits, or host module callbacks before H-series milestones land

## Release acceptance gates

A production-subset release candidate must pass:

1. `make test`
2. `make conformance`
3. sanitizer lane for touched ownership/GC/host/regex/async/VM code
4. per-feature gate evidence for every newly supported feature
5. README/status docs aligned with `docs/production/support-tier-matrix.md`
6. no known stale support claims in current-facing docs
7. final cleanup/deslop and code-review gate from the ultragoal D4 milestone

## Maintenance rule

Any feature moving from Future/Experimental to Supported must update this document, the support-tier matrix, targeted tests, conformance evidence when applicable, and milestone ledger entries in the same change.

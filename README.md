# jsrt — Tiny JavaScript Interpreter in C

`jsrt` is a small JavaScript-like interpreter written in C as a learning exercise.

## Current Scope

The interpreter currently supports:
- hand-written lexer
- recursive-descent parser
- explicit AST
- tree-walk evaluator
- variables
- arithmetic and comparison operators
- string literals and string concatenation with `+`
- array literals and indexing
- object literals with dot / bracket access
- property and index assignment with explicit copy-on-write-ish binding semantics
- `for` loops plus `break` / `continue`
- `do...while` loops
- `switch` / `case` / `default` with JS/C-style fallthrough
- `while` loops
- logical operators `&&` and `||`
- `if` / `else`
- function declarations, calls, closures, and `return`
- function expressions and anonymous closures
- `const` declarations with initializer-required, no-reassign semantics
- source-based lexer/parser/runtime diagnostics with caret markers
- parser recovery to the next statement boundary with aggregated parse errors
- tracing collection for unreachable closure/environment cycles
- manual memory management with explicit destroy paths

## Explicit Non-goals

These are intentionally deferred past the current milestone:
- full heap garbage collection for every runtime allocation (N8 is explicitly bounded to `Env`, `JsFunction`, `JsArray`, and `JsObject`)
- bytecode VM / JIT
- async / event loop behavior
- modules, classes, prototypes
- browser or Node host APIs
- broad ECMAScript compatibility

## Ownership Rules

jsrt uses explicit ownership rules instead of a garbage collector:
- `TokenArray` owns token lexeme strings and must be released with `token_array_free`.
- `StmtArray` / AST nodes own all nested expressions, statements, parameter names, identifier strings, array elements, and object literal properties; release them with `stmt_array_free` / `stmt_destroy`.
- `Env` values form live lexical scope chains; function values retain the environment where they were declared.
- Function bindings can create reference cycles, so a small tracing collector reclaims unreachable closure/environment graphs at safe execution points and final cleanup.
- Arrays and objects are copied on environment read/write boundaries to keep milestone semantics simple and explicit.
- Property/index assignment mutates the current binding's aggregate value; copied aliases remain independent by design.
- `EvalResult.value` is owned by the caller and must be released with `value_free`.

## Build

```sh
make
```

For an explicit debug build target:

```sh
make debug
```

## Test

```sh
make test
```

## Run examples

```sh
./build/jsrt examples/arithmetic.js
./build/jsrt examples/branching.js
./build/jsrt examples/functions.js
./build/jsrt examples/strings.js
./build/jsrt examples/arrays_objects.js
./build/jsrt examples/loops_logic.js
./build/jsrt examples/assignment.js
./build/jsrt examples/for_loops.js
./build/jsrt examples/do_while.js
./build/jsrt examples/switch.js
./build/jsrt examples/function_expressions.js
./build/jsrt examples/const.js
```

## Repository layout

```text
include/jsrt/   public headers
src/            interpreter implementation
tests/          unit/integration-style tests
examples/       small end-to-end programs
docs/           baseline and process notes
```

## Baseline artifacts

Current baseline/process artifacts live in:
- `docs/semantics-baseline.md`
- `docs/ownership-guardrails.md`
- `docs/milestone-verification-checklist.md`
- `docs/diagnostics-golden-harness.md`
- `docs/gc-observability-and-stress.md`

## Current roadmap focus

The active runtime milestone is **M2 — aggregate semantics ADR (`copy` vs `reference`)**. Its execution contract lives in:
- `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md`
- `.omx/plans/prd-javascript-runtime-roadmap-m2-aggregate-semantics-adr.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m2-aggregate-semantics-adr.md`
- `docs/aggregate-semantics-adr.md`

M2 builds on the completed M1 observability work and stays intentionally design-heavy:
1. compare current copy-isolated aggregate behavior against a shared-reference direction
2. record the concrete breakage/compatibility map before any runtime rewrite starts
3. define adoption, abort, and rollback criteria for M3
4. decide whether M3 is required, deferred, or cancelled

M2's chosen direction is:
1. arrays/objects should move toward shared-reference semantics
2. current runtime behavior remains unchanged until M3 implements that migration
3. equality stays intentionally deferred beyond the minimum aggregate-identity decision

Likely follow-ons after M2:
1. M3 — mutable aggregate/reference model implementation
2. M4 — built-in library foundation on the clarified aggregate model
3. M6 — backend/IR planning on the cleaner heap model

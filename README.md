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
- built-in functions: `print`, `len`, `typeOf`
- array literals and indexing
- object literals with dot / bracket access
- property and index assignment with shared-reference aggregate semantics
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
- Arrays and objects preserve shared identity across environment reads/writes, property access, index access, function arguments, and returns.
- Property/index assignment mutates the shared aggregate, so aliases observe the same updates by design.
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
./build/jsrt examples/references.js
./build/jsrt examples/builtins.js
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
- `docs/built-in-library-foundation.md`
- `docs/bytecode-ir-architecture-spike.md`
- `docs/gc-observability-and-stress.md`
- `docs/runtime-error-model.md`

## Current roadmap focus

The active runtime milestone is **M6 — bytecode / IR architecture spike**. Its execution contract lives in:
- `.omx/plans/adr-javascript-runtime-m6-bytecode-ir-direction.md`
- `.omx/plans/prd-javascript-runtime-roadmap-m6-bytecode-ir-architecture-spike.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m6-bytecode-ir-architecture-spike.md`
- `docs/bytecode-ir-architecture-spike.md`

M6 stays intentionally design-heavy:
1. choose the backend direction before implementation
2. define a draft instruction set and compiler boundary
3. define the first parity subset and explicit deferrals
4. decide whether M7/M8 proceed and under what guardrails

Likely follow-ons after M6:
1. M7 — bytecode compiler MVP for the bounded subset
2. M8 — VM interpreter parity backend for the bounded subset
3. future backend expansion only after bounded parity evidence exists

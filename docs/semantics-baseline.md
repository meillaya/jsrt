# jsrt Semantics Baseline

This document freezes the current externally visible behavior of `jsrt` as of Roadmap milestone **N1**. It is a baseline for later milestones, not a promise of broad ECMAScript compatibility.

## Runtime contract

- The CLI executes a single script file: `./build/jsrt <script.js>`.
- `JSRT_BACKEND=vm ./build/jsrt <script.js>` opts into the bounded VM backend for the compiler-supported subset.
- On success, the CLI prints the final non-`null` program value as a string plus newline.
- On lexer/parser/runtime failure, the CLI prints a source-based diagnostic and exits non-zero.

## Supported values

| Category | Current behavior | Evidence |
|---|---|---|
| Numbers | numeric literals, arithmetic, comparison | `tests/test_eval.c`, `examples/arithmetic.js` |
| Booleans | `true`/`false`, truthiness in control flow | `tests/test_eval.c`, `examples/branching.js` |
| Null | `null` literal; falsy; final `null` is not printed by CLI | `src/main.c`, evaluator behavior |
| Strings | string literals with basic escapes, concatenation with `+`, equality | `tests/test_lexer.c`, `tests/test_eval.c`, `examples/strings.js` |
| Functions | declarations, calls, closures, `return` | `tests/test_eval.c`, `examples/functions.js` |
| Arrays | literals, indexing, equality by deep element comparison | `tests/test_eval.c`, `examples/arrays_objects.js` |
| Objects | literals, dot/bracket access, equality by ordered key/value comparison | `tests/test_eval.c`, `examples/arrays_objects.js` |

## Built-in library baseline

| Built-in | Current behavior | Evidence |
|---|---|---|
| `print(value)` | writes the value's string form plus newline, returns `null` | `tests/test_eval.c`, `examples/builtins.js` |
| `len(value)` | returns length/count for strings, arrays, and objects | `tests/test_eval.c`, `src/eval.c` |
| `typeOf(value)` | returns stable runtime type labels | `tests/test_eval.c`, `src/eval.c` |

## Control flow baseline

| Feature | Current behavior | Evidence |
|---|---|---|
| `if` / `else` | standard branch execution on truthiness | `tests/test_eval.c`, `examples/branching.js` |
| `while` | loops until condition becomes falsy | `tests/test_eval.c`, `examples/loops_logic.js` |
| `for` | supports optional initializer / condition / increment | `tests/test_parser.c`, `tests/test_eval.c`, `examples/for_loops.js` |
| `break` | valid only inside loops | `tests/test_eval.c` |
| `continue` | valid only inside loops; in `for`, increment still runs before next iteration | `tests/test_eval.c` |
| `return` | valid only inside function bodies | `tests/test_eval.c` |

## Logical operator baseline

| Feature | Current behavior | Evidence |
|---|---|---|
| `||` | short-circuits and returns the left operand when it is truthy; otherwise evaluates and returns the right operand | `tests/test_eval.c`, `src/eval.c` |
| `&&` | short-circuits and returns the left operand when it is falsy; otherwise evaluates and returns the right operand | `tests/test_eval.c`, `src/eval.c` |
| Truthiness | `false`, `null`, zero-ish numbers, and empty strings are falsy; functions/arrays/objects are truthy | `src/value.c`, `tests/test_eval.c` |

## Aggregate semantics baseline

- Arrays and objects preserve **shared identity** across environment read/write boundaries.
- Updating an array/object binding, property, or index **does** mutate every alias that points at the same aggregate.
- Property assignment may create a missing object key on the final write step.
- Array assignment is in-bounds only.
- Function argument passing and returns preserve aggregate identity instead of de-aliasing arrays/objects.
- Self-referential arrays/objects can now arise through assignment; stringification and equality remain cycle-safe instead of recursing forever.

Evidence:
- `tests/test_eval.c` covers alias-visible mutation, nested assignment, function-call/reference flow, property creation, cycle-safe output/equality, and array assignment bounds.

## Parser / diagnostics baseline

- Parser is recursive-descent and supports aggregated syntax diagnostics after recovery to later statement boundaries.
- Diagnostics include source lines and caret markers.
- Runtime diagnostics are categorized (`type`, `name`, `arity`, `lookup`, `assignment`, `control-flow`, `index`, `math`, `internal`) while preserving the underlying message text.
- Invalid top-level loop control and invalid assignment/index/property operations fail with runtime diagnostics, not silent fallback behavior.

Evidence:
- `tests/test_parser.c`
- `tests/test_lexer.c`
- `tests/test_eval.c`

## Explicitly unsupported or deferred

These are not part of the current baseline:

- modules / imports / exports
- prototype linkage / prototype-chain lookup / method dispatch
- classes / `this` / `new`
- async / event loop behavior
- broad ECMAScript coercion compatibility
- sparse arrays / array growth on assignment
- assignment through arbitrary non-variable roots like `makeObj().x = 1`

## Roadmap L2 update guardrail

When Roadmap L2 lands, this baseline should move only the bounded prototype-chain surface into the supported set:

- explicit prototype linkage via the milestone-approved builtin API
- own-first, then prototype-chain property reads
- method dispatch as prototype-aware lookup followed by an ordinary function call

The following must stay deferred to L3 even after the L2 baseline update:

- implicit `this` binding
- `new` / constructor behavior
- classes or class syntax

L2 should also keep these object-helper rules explicit in the baseline update:

- direct property writes stay receiver-only
- inherited intermediate-container assignment mutates the resolved inherited container rather than inventing an own receiver-local container
- `len(obj)`, object equality, and object stringification remain own-property-only

## Interpretation rule for later milestones

If a future milestone changes any item above, it should:
1. update this baseline,
2. add targeted regression coverage,
3. call out the change explicitly in its milestone PRD/test-spec.

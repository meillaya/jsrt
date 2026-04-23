# jsrt Semantics Baseline

This document freezes the current externally visible behavior of `jsrt` at the current roadmap state. It is a baseline for later milestones, not a promise of broad ECMAScript compatibility.

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
| Functions | declarations, calls, closures, `return`, receiver-bound `this`, bounded constructor `prototype` slots, and `new Ctor(args)` construction | `tests/test_eval.c`, `tests/test_vm.c`, `examples/functions.js`, `examples/constructors.js` |
| Arrays | literals, indexing, equality by deep element comparison | `tests/test_eval.c`, `examples/arrays_objects.js` |
| Objects | literals, dot/bracket access, explicit prototype linkage via `setPrototypeOf`, own-first then prototype-chain lookup, equality by ordered own key/value comparison | `tests/test_eval.c`, `src/eval.c`, `src/vm.c`, `src/value.c`, `examples/arrays_objects.js` |

## Built-in library baseline

| Built-in | Current behavior | Evidence |
|---|---|---|
| `print(value)` | writes the value's string form plus newline, returns `null` | `tests/test_eval.c`, `examples/builtins.js` |
| `len(value)` | returns length/count for strings, arrays, and objects | `tests/test_eval.c`, `src/eval.c` |
| `typeOf(value)` | returns stable runtime type labels | `tests/test_eval.c`, `src/eval.c` |
| `setPrototypeOf(object, protoOrNull)` | sets or clears an object's prototype, returns the receiver, and rejects non-object receivers, invalid prototype values, self-links, and prototype cycles | `tests/test_eval.c`, `tests/test_vm.c`, `src/eval.c`, `src/vm.c` |

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
- Object reads search own properties first, then walk the explicit prototype chain until a match is found or the chain ends.
- Object writes remain receiver-local on the final write step; they do not climb the prototype chain to rewrite ancestors.
- Inherited intermediate containers may still be resolved through prototypes during nested assignment (for example `obj.nested.value = 1` may mutate the inherited `nested` object).
- Function values found through object property lookup now receive the original receiver as `this`; extracted/plain calls still use `this = null`.
- Array assignment is in-bounds only.
- Function argument passing and returns preserve aggregate identity instead of de-aliasing arrays/objects.
- Array assignment is in-bounds only.
- Object helper behavior remains own-only in L2: `len(obj)` counts own keys only, object equality ignores prototype identity and compares own properties only, and stringification displays only own keys.
- Self-referential arrays/objects can now arise through assignment; stringification and equality remain cycle-safe instead of recursing forever.

Evidence:
- `tests/test_eval.c` covers alias-visible mutation, nested assignment, function-call/reference flow, property creation, cycle-safe output/equality, and array assignment bounds.

## Prototype-chain / `this` / constructor baseline

- Objects now carry an optional prototype reference.
- Prototype linkage is explicit through `setPrototypeOf(obj, protoOrNull)`; there is no object-literal `__proto__` syntax.
- Property reads check own properties first, then continue through the prototype chain until a match is found or lookup fails.
- Direct property writes remain receiver-only; setting `obj.name = "child"` updates or creates `obj.name` rather than mutating an inherited `name`.
- Nested assignment may resolve intermediate containers through the prototype chain, but the final write still mutates the resolved container value rather than selecting an ancestor as the write target.
- Object helpers remain own-only in L2: `len(obj)` counts own keys only, object equality compares own properties only, and object stringification displays own properties only.

Evidence:
- `include/jsrt/value.h`
- `src/eval.c`
- `src/vm.c`
- `src/value.c`

## L3 `this` / constructor baseline

- Bare/top-level `this` resolves to `null`.
- Property/index calls (`obj.method()` / `obj["method"]()`) bind `this` to the evaluated receiver object, even when the function is found through the prototype chain.
- Extracted/plain calls continue to run with `this = null`.
- `new Ctor(args...)` allocates a fresh object, binds `this` to it for the constructor body, and applies `Ctor.prototype` when that slot is an object.
- Constructor returns follow the bounded L3 rule: explicit object/array/function returns replace the default instance; primitive/null returns do not.
- Function property support remains intentionally narrow: only `prototype` / `["prototype"]` is supported; other function property reads/writes fail with runtime type diagnostics.
- Native functions are not constructors. Reading `builtin.prototype` yields `null`, writing it fails, and `new builtin(...)` raises a runtime type diagnostic.

Evidence:
- `tests/test_eval.c`
- `tests/test_vm.c`
- `tests/test_bytecode.c`
- `src/eval.c`
- `src/compiler.c`
- `src/vm.c`

## Parser / diagnostics baseline

- Parser is recursive-descent and supports aggregated syntax diagnostics after recovery to later statement boundaries.
- Diagnostics include source lines and caret markers.
- Runtime diagnostics are categorized (`type`, `name`, `arity`, `lookup`, `assignment`, `control-flow`, `index`, `math`, `internal`) while preserving the underlying message text.
- Invalid top-level loop control and invalid assignment/index/property operations fail with runtime diagnostics, not silent fallback behavior.
- `this` and `new` are parser keywords; bounded unsupported `new` forms (for example bare `new Ctor`) fail in the front-end with source diagnostics.

Evidence:
- `tests/test_parser.c`
- `tests/test_lexer.c`
- `tests/test_eval.c`

## Explicitly unsupported or deferred

These are not part of the current baseline:

- modules / imports / exports
- classes
- async / event loop behavior
- broad ECMAScript coercion compatibility
- sparse arrays / array growth on assignment
- assignment through arbitrary non-variable roots like `makeObj().x = 1`
- broad `new` callee shapes beyond the bounded L3 surface (for example bare `new Ctor` or chained `new foo()(1)`)

## Interpretation rule for later milestones

If a future milestone changes any item above, it should:
1. update this baseline,
2. add targeted regression coverage,
3. call out the change explicitly in its milestone PRD/test-spec.

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
| Functions | declarations, calls, closures, `return` | `tests/test_eval.c`, `examples/functions.js` |
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
- Function values found through object property lookup are still called as plain functions; this milestone does **not** add implicit receiver / `this` binding, constructor invocation, or function-level `prototype` behavior.
- Array assignment is in-bounds only.
- Function argument passing and returns preserve aggregate identity instead of de-aliasing arrays/objects.
- Array assignment is in-bounds only.
- Object helper behavior remains own-only in L2: `len(obj)` counts own keys only, object equality ignores prototype identity and compares own properties only, and stringification displays only own keys.
- Self-referential arrays/objects can now arise through assignment; stringification and equality remain cycle-safe instead of recursing forever.

Evidence:
- `tests/test_eval.c` covers alias-visible mutation, nested assignment, function-call/reference flow, property creation, cycle-safe output/equality, and array assignment bounds.

## Prototype-chain / method-dispatch baseline

- Objects now carry an optional prototype reference.
- Prototype linkage is explicit through `setPrototypeOf(obj, protoOrNull)`; there is no object-literal `__proto__` syntax.
- Property reads check own properties first, then continue through the prototype chain until a match is found or lookup fails.
- Direct property writes remain receiver-only; setting `obj.name = "child"` updates or creates `obj.name` rather than mutating an inherited `name`.
- Nested assignment may resolve intermediate containers through the prototype chain, but the final write still mutates the resolved container value rather than selecting an ancestor as the write target.
- `obj.method()` remains bounded L2 behavior: property lookup may find the function through the prototype chain, but the call still behaves like an ordinary function call with no implicit `this` binding.
- Function values are not object-like in the L2 baseline: no constructor calls, no `prototype` meta-property, and no arbitrary function property access are part of the supported surface yet.
- Object helpers remain own-only in L2: `len(obj)` counts own keys only, object equality compares own properties only, and object stringification displays own properties only.

Evidence:
- `include/jsrt/value.h`
- `src/eval.c`
- `src/vm.c`
- `src/value.c`

## `this`, constructor, and bounded function-prototype baseline

- Bare/top-level `this` evaluates to `null`.
- Plain function calls and extracted methods execute with `this = null`.
- Method calls through dot/bracket property lookup bind `this` to the evaluated receiver object.
- `new Ctor(args...)` allocates a fresh object, binds `this` to that object, and returns either the explicit aggregate/function result or the fresh instance when the constructor returns a primitive/`null`.
- User-defined functions expose a narrow `prototype` meta-property used by `new`; `Ctor.prototype` and `Ctor["prototype"]` read/write the same slot.
- Function `prototype` assignment accepts only an object or `null`.
- Non-`prototype` function property access/assignment remains explicitly unsupported and raises a runtime type diagnostic.
- Native functions are not constructors; `new print(...)` fails with a runtime type diagnostic, `print.prototype` reads as `null`, and writes to that slot fail.

Evidence:
- `tests/test_eval.c`
- `tests/test_vm.c`
- `tests/test_bytecode.c`
- `examples/constructors.js`
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
- receiver-bound `this`, constructor calls via `new`, and function-level `prototype` meta-properties
- classes / `super` / desugaring
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

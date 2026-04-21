# PRD — JavaScript Runtime Milestone 8: For Loops + Break/Continue

## Goal
Extend the interpreter with C/JavaScript-style `for` loops plus `break` and `continue`, while preserving the small tree-walk architecture and explicit control-flow handling.

## In Scope
- `for (initializer; condition; increment) statement`
- empty initializer / condition / increment parts
- `let` declarations in `for` initializers
- `break;` and `continue;` inside `while` and `for`
- runtime diagnostics when `break` / `continue` are used outside loops
- tests and one end-to-end example

## Out of Scope
- `for...in` / `for...of`
- labels
- `do...while`
- `switch`
- JS-complete scoping quirks beyond the current lexical environment model

## Acceptance Criteria
1. `for` loops parse and execute correctly.
2. `break` exits `while` and `for` loops.
3. `continue` skips to the next iteration in `while` and `for`, including running the `for` increment step.
4. `break` / `continue` outside loops fail with source-based runtime diagnostics.
5. `make clean && make test` passes.

# PRD — JavaScript Runtime Milestone 4: Loops + Logical Operators

## Goal
Extend the interpreter with `while` loops plus short-circuit logical operators `&&` and `||`, while preserving the small tree-walk architecture.

## In Scope
- `while (condition) { ... }`
- short-circuit `&&` and `||`
- parser precedence for logical operators
- tests and one end-to-end example

## Out of Scope
- `for` loops
- `break` / `continue`
- assignment to array/object members
- parser recovery beyond current diagnostics

## Acceptance Criteria
1. `while` loops parse and execute correctly.
2. `&&` and `||` parse and short-circuit correctly.
3. Loop + object/array interactions work for milestone demos.
4. `make clean && make test` passes.
5. A new end-to-end example runs successfully.

# PRD — Roadmap N5: Function Expressions and Anonymous Closures

## Goal
Add expression-form functions so the runtime can create anonymous closures inline, assign them to variables, pass them around, and call them while preserving the current closure model.

## In Scope
- anonymous function expressions: `function (...) { ... }`
- optional internal function-expression name support if needed for self-reference
- storing and calling function-expression values
- closure capture behavior identical to current function declarations
- tests, one example, and docs update

## Out of Scope
- arrow functions
- methods/classes/prototypes
- async functions
- broad new coercion or object-model work

## Acceptance Criteria
1. Function expressions parse successfully in expression position.
2. Anonymous closures can be assigned and called.
3. Closure capture semantics remain correct.
4. Existing function declaration behavior remains green.
5. `make clean && make test` passes.

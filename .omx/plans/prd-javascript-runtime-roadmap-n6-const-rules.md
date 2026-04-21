# PRD — Roadmap N6: `const` and Declaration-Rule Refinement

## Goal
Tighten variable semantics by adding `const` declarations and rejecting obvious same-scope redeclarations and invalid reassignment to const bindings.

## In Scope
- `const name = expr;`
- const requires an initializer
- assignment to const binding is a runtime error
- same-scope redeclaration rejection for variable/function declarations
- tests, one example, docs update

## Out of Scope
- temporal dead zone behavior
- destructuring
- var hoisting semantics
- module/class-level binding rules

## Acceptance Criteria
1. `const` declarations parse and evaluate correctly.
2. Missing const initializer is rejected.
3. Reassigning a const binding fails with a clear diagnostic.
4. Same-scope redeclaration is rejected.
5. Nested-scope shadowing remains allowed.
6. `make clean && make test` passes.

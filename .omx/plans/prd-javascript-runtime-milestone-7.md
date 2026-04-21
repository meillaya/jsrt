# PRD — JavaScript Runtime Milestone 7: Property + Index Assignment

## Goal
Extend the interpreter with assignment into object properties and array indexes while preserving the existing learning-first, copy-on-read/write runtime model.

## In Scope
- property assignment: `obj.name = value`
- bracket assignment on objects: `obj["name"] = value`
- bracket assignment on arrays: `arr[1] = value`
- nested assignment through existing structures: `obj.inner.count = 3`
- object property creation on final write when the property/key is missing
- tests and one end-to-end example
- README updates describing the copy-semantics behavior

## Out of Scope
- prototype / class semantics
- array resizing or sparse-array behavior beyond in-bounds replacement
- assignment through arbitrary non-variable roots like `makeObj().x = 1`
- reference-style aliasing semantics between variables

## Acceptance Criteria
1. `obj.key = value` and `obj["key"] = value` work.
2. `arr[index] = value` works for valid in-bounds integer indexes.
3. Nested assignment through existing objects/arrays works.
4. Invalid assignment targets or invalid aggregate/index types fail with source-based runtime/parser diagnostics.
5. Existing behavior remains green and `make clean && make test` passes.

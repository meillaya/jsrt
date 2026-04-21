# PRD — JavaScript Runtime Milestone 3: Arrays + Objects

## Goal
Extend the small C interpreter with first-class array and object literals plus read-only access, while preserving the learning-first architecture.

## In Scope
- array literals: `[1, 2, x]`
- object literals with identifier/string keys: `{name: "jsrt", "count": 3}`
- array indexing: `arr[0]`
- object access: `obj.name` and `obj["name"]`
- nested arrays/objects
- stringification/equality support good enough for milestone demos/tests
- tests and one end-to-end example

## Out of Scope
- property/index assignment
- methods / prototypes / classes
- arrays or objects as mutable references with JS semantics
- loops / logical operators / GC / VM

## Acceptance Criteria
1. Arrays parse and evaluate correctly.
2. Objects parse and evaluate correctly.
3. `arr[index]`, `obj.key`, and `obj["key"]` work.
4. Nested access works.
5. Invalid indexing/property lookups fail with source-based diagnostics.
6. `make clean && make test` passes.
7. A new arrays/objects example runs end to end.

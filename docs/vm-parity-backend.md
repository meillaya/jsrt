# VM Parity Backend

This note is the execution-facing companion for roadmap milestone **M8 — VM interpreter parity backend**.

## What M8 adds

The repo now includes:
- a stack VM that executes the M7 bytecode subset
- parity tests against the tree-walk evaluator
- an opt-in CLI backend switch via `JSRT_BACKEND=vm`

## Current VM-supported subset

Included:
- literals and primitive values
- identifier load/store
- `let` declarations in the compiler-supported subset
- arithmetic/comparison/equality operators
- logical operators
- blocks
- `if` / `else`
- `while`
- `for`
- `do...while`
- `switch`
- `break` / `continue`
- top-level and nested non-capturing function declarations/calls
- built-ins `print`, `len`, `typeOf`
- array literals
- object literals
- index/property access
- property/index assignment

Still deferred:
- closures/upvalues
- function expressions
- capturing nested functions

## CLI switch

Tree-walk remains the default backend.

Use the VM explicitly:

```sh
JSRT_BACKEND=vm ./build/jsrt examples/vm_subset.js
```

## Guardrails

- tree-walk remains the semantic authority
- VM support is bounded to the M7 subset
- unsupported features should still fail explicitly in the compiler/backend path

# Bytecode Compiler MVP

This note is the execution-facing companion for roadmap milestone **M7 — bytecode compiler MVP**.

## What M7 adds

The repo now includes:
- a bytecode program/function representation
- a compiler from AST to a bounded stack-bytecode subset
- a disassembler for stable test output
- explicit compiler diagnostics for deferred backend features

## Current supported compiler subset

Included:
- literals (`number`, `bool`, `null`, `string`)
- identifier load/store
- `let` / `const` declarations
- unary `-` / `!`
- arithmetic/comparison/equality operators
- logical `&&` / `||`
- expression statements
- blocks
- `if` / `else`
- `while`
- `for`
- top-level function declarations (nested declarations remain deferred)
- function calls
- built-in calls as ordinary global loads/calls

Deferred in M7:
- function expressions
- aggregate literals/access/mutation
- closures/upvalues
- `switch`
- `do...while`
- `break` / `continue`

## Testing shape

M7 relies on compiler/disassembly tests rather than runtime execution:
- golden-style disassembly for small representative programs
- fixture compilation from `examples/bytecode_fixture.js`
- explicit unsupported-feature compiler diagnostics

## Guardrails

M7 is compiler-only:
- no VM execution yet
- tree-walk remains the implemented runtime backend
- unsupported backend surface should fail loudly in the compiler

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
- `do...while`
- `switch`
- `break` / `continue`
- function declarations
- function expressions
- closure capture lowering via upvalues
- function calls
- array literals
- object literals
- index/property access
- property/index assignment
- built-in calls as ordinary global loads/calls

Deferred in M7:
- broad/full closure semantics auditing beyond the current parity-tested surface

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

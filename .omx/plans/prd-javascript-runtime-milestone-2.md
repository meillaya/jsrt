# PRD — JavaScript Runtime Milestone 2: Strings + Better Diagnostics

## Goal
Extend the milestone-1 interpreter with first-class string literals and substantially better source-based diagnostics, while keeping the codebase small and learning-oriented.

## In Scope
- string tokenization with basic escapes (`\n`, `\t`, `\"`, `\\`)
- string literal AST/runtime representation
- string concatenation via `+`
- string equality / inequality
- improved lexer, parser, and runtime diagnostics that include source lines and caret markers
- tests and examples for the new behavior
- a small `make debug` convenience target

## Out of Scope
- arrays, objects, loops, logical operators
- parser error recovery beyond improved messages
- full JavaScript coercion semantics
- GC / VM / host APIs

## Acceptance Criteria
1. String literals parse and evaluate correctly.
2. `+` supports string concatenation for milestone-2 demos.
3. Equality on strings works.
4. Example program for strings runs end-to-end.
5. Lexer/parser/runtime failures include source snippets and carets.
6. `make clean && make test` passes.

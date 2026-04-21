# Test Spec — JavaScript Runtime Milestone 2

## Coverage
- lexer: string tokens, escapes, unterminated strings
- parser: string literal expressions and source-based parse errors
- evaluator: string values, concatenation, equality, mixed primitive concatenation, runtime diagnostics
- integration: `examples/strings.js`

## Verification
```sh
make clean && make test
./build/jsrt examples/strings.js
```

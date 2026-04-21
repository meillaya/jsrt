# Test Spec — JavaScript Runtime Milestone 4

## Coverage
- lexer: `&&`, `||`, `while`
- parser: while statements and logical precedence
- evaluator: loops, short-circuit behavior, loop arithmetic
- integration: `examples/loops_logic.js`

## Verification
```sh
make clean && make test
./build/jsrt examples/loops_logic.js
```

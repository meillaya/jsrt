# Test Spec — JavaScript Runtime Milestone 8

## Coverage
- lexer: `for`, `break`, `continue`
- parser: `for` statements with optional clauses, `break`, `continue`
- evaluator: `for` execution, while/for break, while/for continue, continue still runs `for` increment
- diagnostics: `break` / `continue` outside loops
- integration: `examples/for_loops.js`

## Verification
```sh
make clean && make test
./build/jsrt examples/for_loops.js
```

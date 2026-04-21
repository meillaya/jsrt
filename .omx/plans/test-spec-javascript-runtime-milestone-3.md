# Test Spec — JavaScript Runtime Milestone 3

## Coverage
- lexer: brackets, colon, dot in array/object syntax
- parser: array literals, object literals, postfix index/property access
- evaluator: array/object values, nested access, equality, invalid lookup diagnostics
- integration: `examples/arrays_objects.js`

## Verification
```sh
make clean && make test
./build/jsrt examples/arrays_objects.js
```

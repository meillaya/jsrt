# Test Spec — JavaScript Runtime Milestone 7

## Coverage
- parser: assignment targets for identifiers, properties, and indexes
- evaluator: object property update/add, bracket-key update/add, array index update, nested assignment
- diagnostics: invalid assignment targets, invalid aggregate targets, invalid array indexes
- integration: `examples/assignment.js`

## Verification
```sh
make clean && make test
./build/jsrt examples/assignment.js
```

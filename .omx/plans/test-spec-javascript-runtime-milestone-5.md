# Test Spec — JavaScript Runtime Milestone 5

## Coverage
- valid parsing remains unchanged for existing language features
- parser recovery aggregates multiple syntax errors
- block/function parsing can recover enough to report later independent errors

## Verification
```sh
make clean && make test
```

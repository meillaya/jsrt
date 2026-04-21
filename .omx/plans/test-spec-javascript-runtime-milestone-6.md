# Test Spec — JavaScript Runtime Milestone 6

## Coverage
- GC collects unreachable function/environment cycles created inside nested scopes
- live closures returned from inner scopes survive collection and still evaluate correctly
- final program cleanup succeeds without the old force-destroy sweep
- regression coverage for existing examples/features stays green

## Verification
```sh
make clean && make test
```

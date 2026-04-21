# Test Spec — Roadmap M3: Mutable Aggregate/Reference Model

## Coverage Goals
- environment reads/writes preserve array/object identity
- alias mutation is visible across variables, nested properties, function arguments, and function returns
- aggregate equality/stringification remain safe on cyclic structures
- existing non-aggregate semantics stay green

## Expected Evidence
- targeted aggregate aliasing/evaluator regressions in `tests/test_eval.c`
- fresh `make clean && make test`
- fresh end-to-end example run showing reference semantics
- fresh ASan verification

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/references.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- keep M3 bounded to aggregate identity and directly related safety fixes
- if preserving aggregate identity requires widening GC scope or redesigning the evaluator root model, stop and report against the M2 abort criteria
- update current-facing docs/baseline language after the tests prove the new behavior

# Test Spec — Roadmap N1: Semantics Baseline and Ownership Guardrails

## Coverage Goals
- baseline documentation matches actual runtime behavior
- ownership/lifetime notes match current implementation boundaries
- existing supported examples continue to pass
- any newly documented but previously under-tested behavior gains narrow regression coverage

## Expected Evidence
- fresh `make clean && make test`
- fresh example runs for the current visible feature set
- fresh ASan run using the repo's current CFLAGS override path
- artifact review showing the baseline matrix and ownership notes are grounded in current code/tests/examples

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/arithmetic.js
./build/jsrt examples/branching.js
./build/jsrt examples/functions.js
./build/jsrt examples/strings.js
./build/jsrt examples/arrays_objects.js
./build/jsrt examples/loops_logic.js
./build/jsrt examples/assignment.js
./build/jsrt examples/for_loops.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- if a documented baseline claim is not test-backed, add the smallest possible regression test
- if a behavior seems accidental or contradictory, record it explicitly instead of silently redefining it
- do not broaden this milestone into feature work

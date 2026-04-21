# Test Spec — Roadmap N8: General Heap GC MVP

## Coverage Goals
- canonical collector path is exercised in normal evaluation flows
- closure/env behavior remains correct
- arrays/objects survive and release correctly under the managed path
- current aggregate copy-isolated semantics remain unchanged
- no regressions to parser/runtime behavior already locked by N1/N2 and later milestones

## High-value regression/stress cases
- nested closure creation and release
- returned closures held across multiple calls
- arrays/objects built inside loops and function calls
- nested arrays/objects passed through env boundaries
- assignment into arrays/objects after migration
- recursive named function-expression calls with aggregate values
- existing examples remain green

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
./build/jsrt examples/do_while.js
./build/jsrt examples/switch.js
./build/jsrt examples/function_expressions.js
./build/jsrt examples/const.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- if a stress case requires a new fixture, keep it small and semantics-focused
- if any example output changes unexpectedly, treat it as a probable semantic regression unless the ADR explicitly allows it
- if bounded temporary-rooting is not sufficient, stop and escalate rather than widening the milestone silently

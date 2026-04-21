# Test Spec — Roadmap M1: GC Observability and Stress Rig

## Coverage Goals
- observability hooks can be enabled without changing behavior
- stress fixtures exercise closure/env/array/object lifetime repeatedly
- normal runtime results remain unchanged with observability disabled
- debug or explicit observability mode emits stable enough evidence for manual inspection

## Verification Commands
```sh
make clean && make test
make clean && make debug
./build/jsrt examples/gc_stress.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

# Test Spec — Roadmap M9: VM Surface Widening I

## Coverage Goals
- widened compiler lowering for aggregates and deferred control flow succeeds
- VM execution matches tree-walk for arrays/objects, assignment, `switch`, `do...while`, `break`, and `continue`
- closure/upvalue-dependent cases still fail explicitly
- CLI VM backend switch works for a widened-surface example

## Expected Evidence
- updated compiler tests in `tests/test_bytecode.c`
- updated parity tests in `tests/test_vm.c`
- fresh `make clean && make test`
- fresh ASan verification

## Verification Commands
```sh
make clean && make test
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- keep closure/upvalue boundaries explicit
- prefer parity checks over isolated VM-only expectations where practical
- do not silently widen function-expression/closure support without tests proving what changed

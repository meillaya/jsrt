# Test Spec — Roadmap M8: VM Interpreter Parity Backend

## Coverage Goals
- VM execution succeeds for the bounded bytecode subset
- tree-walk and VM results match on representative programs
- unsupported bytecode surface still fails explicitly
- CLI backend switch works for a bounded example

## Expected Evidence
- parity tests in `tests/test_vm.c`
- fresh `make clean && make test`
- fresh ASan verification

## Verification Commands
```sh
make clean && make test
JSRT_BACKEND=vm ./build/jsrt examples/vm_subset.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- keep unsupported constructs explicit; do not silently widen M8 beyond the M7 subset
- treat tree-walk results as the parity oracle
- VM backend should remain opt-in until broader parity exists

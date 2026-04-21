# Test Spec — Roadmap M7: Bytecode Compiler MVP

## Coverage Goals
- successful bytecode lowering for representative bounded-subset programs
- deterministic disassembly suitable for golden-style assertions
- explicit compiler diagnostics for deferred constructs
- unchanged runtime tree-walk tests remain green

## Expected Evidence
- compiler/disassembly tests in `tests/test_bytecode.c`
- fresh `make clean && make test`
- fresh ASan verification

## Verification Commands
```sh
make clean && make test
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- keep M7 additive; do not start executing bytecode yet
- prefer explicit compiler errors over silently miscompiling deferred features
- keep disassembly formatting stable once tests lock it in

# Test Spec — Roadmap M10: VM Closure + Function-Expression Parity

## Coverage Goals
- closure-producing function declarations execute on the VM backend
- function expressions execute on the VM backend
- parity coverage includes closure mutation scenarios already present in repo tests/examples
- normal and ASan builds remain green

## Verification Commands
```sh
make clean && make test
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes
- treat the existing tree-walk behavior as the semantic oracle
- keep any remaining closure caveats explicit in docs rather than silently implying full JS-complete closure semantics

# Test Spec — Roadmap M4: Built-in Library Foundation

## Coverage Goals
- global built-ins resolve and call through the evaluator correctly
- `print`, `len`, and `typeOf` behave correctly on supported inputs
- built-in misuse produces source-based runtime diagnostics
- existing user-defined function behavior remains green

## Expected Evidence
- targeted evaluator regressions for built-in success/misuse cases
- fresh `make clean && make test`
- fresh example run proving the built-in output path
- fresh ASan verification

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/builtins.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- built-ins should stay globally available but bounded in number and behavior
- keep error messages explicit enough to distinguish wrong arity from wrong input type
- avoid changing CLI final-value printing rules; `print` should return `null`

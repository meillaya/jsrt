# Test Spec — Roadmap M5: Exceptionless Structured Runtime Error Model

## Coverage Goals
- runtime error diagnostics consistently include category labels
- `EvalResult` carries the expected runtime error category and source location metadata
- existing runtime-failure messages remain semantically stable beneath the new category label
- successful evaluation paths remain green

## Expected Evidence
- targeted runtime diagnostic tests in `tests/test_diagnostics.c`
- fresh `make clean && make test`
- fresh ASan verification

## Verification Commands
```sh
make clean && make test
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- keep M5 exceptionless; the goal is structured reporting, not runtime recovery
- prefer shared helper paths over one-off ad hoc diagnostic assembly
- when updating tests, preserve the substantive error message text unless the category model truly requires a wording change

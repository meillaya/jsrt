# Test Spec — Roadmap N2: Golden Diagnostics Harness

## Coverage Goals
- exact lexer diagnostic for invalid token or unterminated string
- exact parser diagnostic for a single syntax error
- exact aggregated parser-recovery diagnostic for multiple syntax errors in one input
- exact runtime diagnostic for a stable user-facing failure
- existing broader test/example coverage remains green

## Expected Evidence
- fresh `make clean && make test`
- fresh ASan run via CFLAGS override
- diagnostics tests prove full-output matching, not just substring checks, for the selected goldens

## Verification Commands
```sh
make clean && make test
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Notes for Execution
- keep the golden set intentionally small and representative
- use exact string equality for the selected cases
- if current diagnostics are inconsistent, prefer minimal normalization over large rewrites

# Test Spec — Roadmap N3: `do...while` Support

## Coverage Goals
- lexer recognizes `do`
- parser accepts `do...while` syntax and rejects malformed forms
- evaluator proves one-iteration minimum behavior
- `break` and `continue` work inside `do...while`
- example output is stable

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/do_while.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

# Test Spec — Roadmap N6: `const` and Declaration-Rule Refinement

## Coverage Goals
- lexer recognizes `const`
- parser accepts valid const declarations and rejects missing initializer/name issues
- evaluator rejects const reassignment
- declaration layer rejects same-scope redeclaration
- nested shadowing still works
- example output is stable

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/const.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

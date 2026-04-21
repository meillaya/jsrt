# Test Spec — Roadmap N5: Function Expressions and Anonymous Closures

## Coverage Goals
- parser accepts function expressions in expression position
- evaluator supports assignment/call of anonymous closures
- closure capture works for expression-form functions
- optional named self-reference works if implemented
- existing declaration-form functions remain green

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/function_expressions.js
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

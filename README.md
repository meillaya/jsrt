# jsrt

A small JavaScript-like runtime in C with a hand-written lexer, recursive-descent parser, tree-walk evaluator, and an opt-in bytecode VM.

## What it does

`jsrt` currently supports a bounded language surface with:
- numbers, booleans, `null`, and strings
- variables, `const`, assignment, and blocks
- `if`, `while`, `for`, `do...while`, `switch`, `break`, `continue`
- functions, closures, and function expressions
- arrays and objects with shared-reference mutation semantics plus explicit prototype linkage / prototype-chain lookup
- bounded `this` binding and parenthesized `new` constructor semantics
- bounded `class` declarations lowered onto the constructor/prototype runtime
- built-ins: `print`, `len`, `typeOf`
- source-based diagnostics with caret markers
- an opt-in VM backend via `JSRT_BACKEND=vm`

## Build

```sh
make
```

## Run

Tree-walk backend:

```sh
./build/jsrt examples/arithmetic.js
./build/jsrt examples/references.js
./build/jsrt examples/this_new.js
./build/jsrt examples/class_syntax.js
```

VM backend:

```sh
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
JSRT_BACKEND=vm ./build/jsrt examples/class_syntax.js
```

## Test

```sh
make test
```

ASan lane:

```sh
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Docs

- Current status: `docs/project-status-and-architecture.md`
- Semantics baseline: `docs/semantics-baseline.md`
- Ownership/lifetimes: `docs/ownership-guardrails.md`
- Benchmark harness: `docs/benchmark-and-profiling-harness.md`
- Roadmap: `.omx/plans/prd-javascript-runtime-roadmap.md`

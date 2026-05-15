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
- bounded static named `import` / declaration `export` modules on the tree-walk backend
- built-ins: `print`, `len`, `typeOf`, `defer`, `runJobs`, `strContains`, `strSlice`, `regexMatch`
- source-based diagnostics with caret markers
- a small embeddable C host API for source/file evaluation
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
./build/jsrt examples/modules.js
```

VM backend:

```sh
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
JSRT_BACKEND=vm ./build/jsrt examples/class_syntax.js
```

## Test

```sh
make test
make conformance
make test262-adapter
make vm-parity
make verify-production
```

ASan lane:

```sh
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## Docs

- Production subset spec: `docs/production/subset-specification.md`
- Support-tier matrix: `docs/production/support-tier-matrix.md`
- Unsupported diagnostics policy: `docs/production/unsupported-diagnostics-policy.md`
- Verification targets: `docs/production/verification-targets.md`
- VM parity harness: `docs/production/vm-parity-harness.md`
- Module linker ADR: `docs/production/module-linker-adr.md`
- Host API ADR: `docs/production/host-api-adr.md`
- Public C API guide: `docs/production/public-c-api-guide.md`
- Release checklist: `docs/production/release-checklist.md`
- Conformance and quality gates: `docs/production/conformance-and-quality-gates.md`
- RegExp ADR: `docs/production/regexp-adr.md`
- Async ADR: `docs/production/async-adr.md`
- Current status: `docs/project-status-and-architecture.md`
- Semantics baseline: `docs/semantics-baseline.md`
- Module system: `docs/module-system.md`
- Embeddable host API: `docs/embeddable-host-api.md`
- Regex and stdlib: `docs/regex-and-stdlib.md`
- Conformance subset harness: `docs/conformance-subset-harness.md`
- Optimization feasibility: `docs/optimization-feasibility-study.md`
- Ownership/lifetimes: `docs/ownership-guardrails.md`
- Benchmark harness: `docs/benchmark-and-profiling-harness.md`
- Roadmap: `.omx/plans/prd-javascript-runtime-roadmap.md`

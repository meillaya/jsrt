# jsrt Master Documentation

## Overview

`jsrt` is a small JavaScript-like runtime written in C. It has a hand-written lexer, parser, tree-walk evaluator, managed runtime objects, a bounded standard library, an embeddable C API, and an opt-in bytecode virtual machine for a supported subset.

The tree-walk evaluator is the semantic reference. The virtual machine is supported only where tests and parity checks prove the same observable behavior.

## Build, Run, and Verification

Zig 0.16.0 or newer is the primary build system. The Makefile is a compatibility wrapper over `zig build`.

```sh
zig build
./build/jsrt examples/arithmetic.js
```

Useful verification commands:

```sh
zig build test
zig build conformance
zig build test262-adapter
zig build vm-parity
zig build verify-production
zig build bench-smoke
zig build asan
```

Compatibility wrappers remain available:

```sh
make test
make conformance
make test262-adapter
make vm-parity
make verify-production
make bench-smoke
make asan
```

The build preserves `./build/jsrt` and `./build/test_jsrt` because scripts and tests use those paths.

## Language and Runtime Scope

The runtime intentionally implements a bounded language rather than full ECMAScript. Supported behavior includes:

- numbers, booleans, `null`, and strings
- mutable variables, `const`, assignment, and lexical blocks
- `if`, `while`, `for`, `do...while`, `switch`, `break`, and `continue`
- functions, closures, and function expressions
- arrays and objects with shared-reference mutation semantics
- explicit prototype links and prototype-chain property lookup
- bounded `this` binding and parenthesized constructor calls with `new`
- bounded class declarations lowered onto constructor/prototype behavior
- bounded static named imports and declaration exports on the tree-walk backend
- source-based diagnostics with caret markers

Unsupported language features should fail clearly instead of behaving partially or silently.

## Values, Aggregates, and Equality

Values include primitive values, functions, arrays, objects, and managed runtime objects. Arrays and objects use shared-reference mutation semantics: assigning or passing an aggregate preserves identity, so mutations through one reference are visible through the other.

Stringification and equality are bounded and cycle-safe. The implementation avoids pretending to match broad ECMAScript coercion rules.

## Functions, Constructors, Prototypes, and Classes

Functions are callable values and can close over their defining environment. Constructor behavior is intentionally narrow: parenthesized `new Ctor(args)` is supported for the documented constructor/prototype subset.

Prototype behavior supports explicit prototype links and method lookup through the chain. Class declarations are a bounded syntax layer over the constructor/prototype runtime. Inheritance, `super`, private fields, static fields, broad class expressions, and broad method forms are unsupported unless explicitly implemented and tested.

`setPrototypeOf(obj, proto)` updates an object's explicit prototype link. The receiver must be an object, the prototype must be an object or `null`, and attempts to set a self-prototype or create a prototype cycle fail with deterministic runtime errors.

## Modules

The tree-walk backend supports bounded static named imports and declaration exports. Module resolution is file-based and intentionally simple. The runtime supports deterministic diagnostics for unsupported or hidden imports.

The virtual machine does not claim general module execution. VM behavior must either match the tree-walk backend for a supported fixture or reject unsupported module syntax with a clear diagnostic.

## Built-In Library

Current built-ins include:

- `print(value)`
- `len(value)`
- `typeOf(value)`
- `defer(fn)`
- `runJobs()`
- `setPrototypeOf(obj, proto)`
- `strContains(text, needle)`
- `strSlice(text, start, end)`
- `regexMatch(text, pattern)`

The library is intentionally small. Unsupported built-ins should be added only with tests, documentation, and clear diagnostics.

## Async and Job Queue Behavior

The runtime includes a bounded host-controllable job queue through `defer` and `runJobs`. This is not a full browser or Node.js event loop, and it is not a full Promise or async/await implementation.

Job behavior must remain deterministic and explicit. Host integration should avoid hidden background execution.

## Regex Behavior

Regex support is bounded and exposed through `regexMatch(text, pattern)`. The implementation is dependency-free and intentionally narrower than ECMAScript `RegExp`. Unsupported pattern forms should fail clearly rather than backtracking indefinitely or silently accepting broad syntax.

## Diagnostics and Unsupported Behavior

Diagnostics should be deterministic, source-oriented, and behavior-first. Messages should name the unsupported construct or runtime boundary directly instead of referring to internal delivery phases.

Expected diagnostic behavior:

- parse errors identify unsupported syntax or malformed input
- runtime errors identify invalid operations such as division by zero or invalid calls
- compiler and VM diagnostics identify unsupported backend behavior directly
- module diagnostics identify missing, hidden, unsupported, or invalid imports/exports

Regression tests should lock important diagnostic wording when the wording is part of the user-facing contract.

## Memory Management and Ownership

The runtime uses explicit ownership rules plus managed runtime objects for environments, functions, arrays, objects, and related state. Temporary roots protect values across allocation and evaluation boundaries.

Ownership rules:

- token/source/AST lifetimes are explicit
- values that own heap memory must be copied or freed by the owning layer
- environments and closures must preserve captured state safely
- aggregate objects participate in reachability traversal
- GC trace mode is opt-in through `JSRT_GC_TRACE=1`

Example trace run:

```sh
zig build debug
JSRT_GC_TRACE=1 ./build/jsrt examples/gc_stress.js 2> build/gc-trace.log
```

## Bytecode and Virtual Machine

The compiler lowers a supported subset into bytecode. The stack virtual machine executes the supported bytecode surface and is selected with `JSRT_BACKEND=vm`.

```sh
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
```

VM support is bounded by parity tests. A feature is VM-supported only when tests demonstrate matching tree-walk behavior or a deterministic unsupported diagnostic exists.

## Embedding and Public C API

The public C API is declared under `include/jsrt/runtime.h`. It provides runtime lifecycle helpers, source/file evaluation, version checks, and diagnostic access for embedders.

Embedding boundaries:

- runtime creation and destruction are explicit
- source evaluation returns structured status
- diagnostics remain owned by the runtime until copied or the runtime is destroyed
- current embedding is source/file oriented
- advanced host callbacks, broad value handles, cancellation, and host module hooks are outside the current stable API unless added with tests and documentation

## Conformance, Parity, and Quality Gates

The repository uses local verification rather than broad ECMAScript conformance claims.

Verification surfaces:

- unit tests: `zig build test`
- local conformance manifest: `zig build conformance`
- Test262 subset manifest validation: `zig build test262-adapter`
- tree/VM parity fixtures: `zig build vm-parity`
- production gate bundle: `zig build verify-production`
- benchmark smoke: `zig build bench-smoke`
- sanitizer lane: `zig build asan`

The Test262 subset manifest tracks feature areas, support tiers, expected outcomes, skip reasons, and expected-failure reasons. It is a curated adapter surface, not a claim of full Test262 compliance. Its schema is:

```text
id,feature,area,tier,source,local_path,backends,expect,stdout,stderr_contains,skip_reason,xfail_reason
```

`tier` must be `supported`, `experimental`, `future`, or `rejected`; `backends` must be `tree`, `vm`, `both`, or blank; and `expect` must be `success`, `failure`, `skip`, or `xfail`. Skipped cases require `skip_reason`, expected failures require `xfail_reason`, and non-skipped rows with `local_path` must point to an existing file.

## Benchmarking

Benchmarks are manifest-driven and produce CSV output with a stable schema. Benchmark modes separate tree-walk timing, VM timing, comparison runs, and harness smoke checks.

```sh
zig build bench-smoke
zig build bench -Diterations=5 -Dwarmups=1
zig build bench-vm -Diterations=5 -Dwarmups=1
zig build bench-compare -Diterations=5 -Dwarmups=1
```

Benchmark results are local measurements. They are useful for comparing changes on the same machine, not for portable performance guarantees.

## Known Unsupported Behavior

The project intentionally does not claim support for:

- full ECMAScript compatibility
- browser or Node.js host environments
- broad module resolution, package loading, cyclic module semantics, or dynamic import
- full Promise, async function, or event-loop semantics
- full ECMAScript regular expressions
- broad coercion semantics
- `extends`, `super`, private class fields, decorators, or full class syntax
- native JIT compilation
- full VM parity for every tree-walk feature
- advanced embeddable host APIs beyond the documented C surface

Unsupported behavior should remain explicit and testable.

## Source Layout

- `src/` contains runtime implementation.
- `include/jsrt/` contains public and internal headers.
- `tests/` contains C unit tests.
- `examples/` contains executable language examples.
- `conformance/` contains local conformance and parity manifests.
- `benchmarks/` contains benchmark fixtures and baseline CSV data.
- `scripts/` contains shell harnesses used by build steps.
- `docs/MASTER.md` is the canonical product documentation.

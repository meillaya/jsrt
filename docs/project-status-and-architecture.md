# jsrt — Current Theory, Architecture, and Implementation Status

This document is the **current-state overview** for `jsrt` as of **2026-04-21**. It complements the milestone-local notes in `docs/` and the planning artifacts under `.omx/plans/`.

Older milestone docs such as `docs/vm-parity-backend.md` capture the scope of a specific milestone at the time it was written. This file is the best place to understand what the repository implements **today**.

## 1. Project thesis

`jsrt` is a small JavaScript-like runtime written in C. The project is explicitly learning-first: the code favors clarity, explicit ownership, and milestone-by-milestone evolution over full ECMAScript compatibility.

The main design choice is to keep two execution paths in the same repository. The tree-walk evaluator remains the semantic authority, while the bytecode compiler and VM act as a bounded, opt-in backend that is gradually brought toward parity.

## 2. Theory and language model

The language model is intentionally narrow. It implements a useful JavaScript-shaped subset rather than the ECMAScript specification as a whole.

The frontend follows the standard interpreter pipeline: lexical analysis produces tokens, parsing produces an AST, and evaluation executes that AST directly or lowers it into bytecode first.

```text
source file
  -> lexer
  -> token stream
  -> recursive-descent parser
  -> AST
  -> tree-walk evaluator (default)
     or
  -> bytecode compiler
  -> stack VM (opt-in)
```

The runtime model is value-oriented for numbers, booleans, `null`, and strings. Arrays, objects, functions, and environments live on a bounded managed heap.

Aggregate semantics are now **shared-reference** semantics. Arrays and objects preserve identity across environment reads/writes, function arguments/returns, and property/index updates, so aliasing is visible by design.

## 3. Architectural overview

The repository is organized around a clean split between frontend, runtime, compiler, and verification layers.

```text
include/jsrt/
  token.h        token model
  lexer.h        lexer API
  ast.h          AST node model
  parser.h       parser API
  value.h        runtime value model
  env.h          lexical environments
  gc.h           bounded managed heap + tracing GC
  eval.h         tree-walk evaluator API
  bytecode.h     bytecode IR
  compiler.h     AST -> bytecode compiler API
  vm.h           VM execution API

src/
  lexer.c        hand-written scanner
  parser.c       recursive-descent parser + recovery
  ast.c          AST allocation/destruction helpers
  value.c        value operations, equality, stringification, aggregate helpers
  env.c          lexical scope bindings
  gc.c           mark/sweep registry and temp-root handling
  eval.c         semantic oracle: tree-walk execution
  bytecode.c     bytecode containers + disassembly
  compiler.c     lowering from AST to stack bytecode
  vm.c           stack-based VM backend
  main.c         CLI, backend selection, cleanup

tests/
  test_lexer.c
  test_parser.c
  test_eval.c
  test_diagnostics.c
  test_bytecode.c
  test_vm.c
```

The CLI entry point in `src/main.c` owns the top-level orchestration. It reads the file, lexes, parses, optionally compiles when `JSRT_BACKEND=vm`, executes, prints the final non-`null` value, and then performs structured cleanup.

## 4. Frontend implementation details

### 4.1 Lexer

The lexer is hand-written and tokenizes punctuation, operators, identifiers, literals, and reserved words. It is intentionally simple and predictable, which makes it a good fit for a learning runtime.

Relevant files:
- `src/lexer.c`
- `include/jsrt/token.h`
- `include/jsrt/lexer.h`

Supported token families include:

```text
literals: number, string, true, false, null
keywords: let, const, function, return, if, else, do, for, while,
          switch, case, default, break, continue
operators: + - * / ! == != < <= > >= && || = . [ ] { } ( ) : ; ,
```

### 4.2 Parser

The parser is a hand-written recursive-descent parser. It constructs an explicit AST with dedicated node kinds for literals, variables, unary/binary/logical expressions, calls, function expressions, arrays, objects, indexing, property access, declarations, blocks, loops, `switch`, and control-flow statements.

Relevant files:
- `src/parser.c`
- `src/ast.c`
- `include/jsrt/ast.h`
- `include/jsrt/parser.h`

The parser also supports recovery and aggregated syntax diagnostics. That gives the project a useful middle ground between toy parsing and industrial tooling.

## 5. Runtime implementation details

### 5.1 Value system

`Value` is the runtime payload type. It supports seven categories:

```text
number | bool | null | string | function | array | object
```

Relevant files:
- `include/jsrt/value.h`
- `src/value.c`

`src/value.c` centralizes truthiness, equality, stringification, aggregate reachability, and clone/release behavior. It also contains the cycle-safe logic needed now that self-referential arrays/objects can exist under shared-reference semantics.

### 5.2 Environments and closures

Lexical environments are represented by `Env`. Each environment holds a linked list of bindings plus a parent pointer for outer scopes.

Relevant files:
- `include/jsrt/env.h`
- `src/env.c`

Functions retain the environment where they were declared. That gives the tree-walk runtime lexical scoping and closure capture without introducing a heavyweight object system.

### 5.3 Tree-walk evaluator

The default backend is the tree-walk evaluator in `src/eval.c`. This file is the semantic oracle for the project.

It currently implements:
- declarations and assignment
- arithmetic, comparison, equality, and logical operators
- blocks and conditionals
- `while`, `for`, and `do...while`
- `switch`/`case`/`default`
- `break`, `continue`, and `return`
- function declarations and function expressions
- closure capture
- arrays/objects with property/index access and mutation
- built-ins `print`, `len`, and `typeOf`

The evaluator uses an explicit `EvalResult` structure rather than exceptions. That structure carries success values, control-flow signals, and structured runtime error metadata.

### 5.4 Diagnostics model

Diagnostics are source-based and include caret markers. Lexer, parser, compiler, and runtime failures all follow this source-oriented style.

Relevant files:
- `src/diagnostics.c`
- `include/jsrt/diagnostics.h`
- `docs/runtime-error-model.md`
- `docs/diagnostics-golden-harness.md`

Runtime diagnostics are additionally categorized. The current categories are:

```text
type | name | arity | lookup | assignment | control-flow | index | math | internal
```

## 6. Managed heap and memory model

`jsrt` does **not** run a universal GC for every allocation in the process. It uses a bounded managed heap for the runtime objects that matter most to closures and aggregate sharing.

Managed object families today:

```text
Env
JsFunction
JsArray
JsObject
```

Still manually owned today:

```text
source buffers
TokenArray lexemes
AST nodes
parser-owned strings
diagnostic strings
ordinary runtime strings stored in Value
```

Relevant files:
- `include/jsrt/gc.h`
- `src/gc.c`
- `docs/ownership-guardrails.md`
- `docs/gc-observability-and-stress.md`

Collection is explicit and safe-point based. The evaluator and runtime use temporary GC roots when a managed value must survive nested evaluation before it is stored somewhere permanent.

The collector is mark/sweep over a registry of managed objects. It also exposes observability counters and optional tracing through `JSRT_GC_TRACE=1`.

## 7. Compiler and VM architecture

The second backend is a stack-based bytecode VM. The compiler lowers AST nodes into bytecode instructions stored in `BytecodeProgram` and `BytecodeFunction` containers.

Relevant files:
- `include/jsrt/bytecode.h`
- `include/jsrt/compiler.h`
- `include/jsrt/vm.h`
- `src/bytecode.c`
- `src/compiler.c`
- `src/vm.c`

The high-level VM pipeline is:

```text
AST
  -> compiler resolves locals / globals / upvalues
  -> stack bytecode
  -> VM call frames + operand stack
  -> runtime Value result
```

The compiler supports lexical locals, globals, control-flow patching, aggregate construction/access/mutation, function declarations, function expressions, and closure capture through upvalues. The VM executes the compiled subset using stack frames and captured values on compiled functions.

The default backend is still the tree-walk evaluator. The VM is opt-in and intentionally treated as the secondary backend until broader parity work justifies any promotion.

## 8. What has been achieved so far

The project has moved far beyond a minimal arithmetic interpreter. The current repository already demonstrates a coherent end-to-end language/runtime stack.

### Frontend and semantics

Achieved so far:
- hand-written lexer with source diagnostics
- recursive-descent parser with recovery and aggregated parse errors
- explicit AST with clear ownership rules
- variables, assignment, functions, closures, and function expressions
- strings, arrays, objects, explicit prototype-chain lookup, and shared-reference aggregate semantics
- `if`, `while`, `for`, `do...while`, `switch`, `break`, `continue`
- `const` declarations
- built-ins `print`, `len`, and `typeOf`

Representative examples:

```text
examples/arithmetic.js
examples/functions.js
examples/arrays_objects.js
examples/assignment.js
examples/do_while.js
examples/switch.js
examples/function_expressions.js
examples/references.js
```

### Runtime and memory

Achieved so far:
- lexical environments and closure capture
- exceptionless structured runtime error model
- bounded managed heap for env/function/array/object objects
- explicit temporary-root handling
- GC observability counters and trace mode
- stress coverage via `examples/gc_stress.js`

### Compiler and VM backend

Achieved so far:
- bytecode IR and disassembler
- AST-to-bytecode compiler
- stack VM with parity tests
- widened VM support for aggregates and structured control flow
- VM closure + function-expression support for the currently tested parity surface

Representative VM examples:

```sh
JSRT_BACKEND=vm ./build/jsrt examples/vm_subset.js
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
```

### Verification and process

Achieved so far:
- dedicated lexer, parser, evaluator, diagnostics, bytecode, and VM tests
- milestone companion docs in `docs/`
- roadmap, PRD, ADR, and test-spec artifacts in `.omx/plans/`
- ASan-friendly build path through `make test CFLAGS='... -fsanitize=address ...'`

## 9. Current gaps and boundaries

The project is intentionally incomplete. That is a feature, not a flaw, because the repo is optimized for learnable progression.

Still outside scope today:
- modules and multi-file execution
- classes / `super` / class-field semantics
- async/event loop semantics
- broad ECMAScript coercion and compatibility
- JIT or optimization passes
- a general-purpose embedding API

The VM is also not presented as “full parity”. It is parity-tested for the current bounded surface exercised by `tests/test_vm.c` and the VM examples.

## 10. Build, run, and test

### 10.1 Build

Standard build:

```sh
make
```

Debug build:

```sh
make debug
```

Clean build artifacts:

```sh
make clean
```

### 10.2 Run

Run the default tree-walk backend:

```sh
./build/jsrt examples/arithmetic.js
./build/jsrt examples/functions.js
./build/jsrt examples/references.js
```

Run the VM backend explicitly:

```sh
JSRT_BACKEND=vm ./build/jsrt examples/vm_subset.js
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
```

Run GC trace mode:

```sh
make clean && make debug
JSRT_GC_TRACE=1 ./build/jsrt examples/gc_stress.js 2> build/gc-trace.log
cat build/gc-trace.log
```

### 10.3 Test

Run the main test suite:

```sh
make test
```

Run the sanitizer lane:

```sh
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

Useful end-to-end checks:

```sh
./build/jsrt examples/builtins.js
./build/jsrt examples/references.js
JSRT_BACKEND=vm ./build/jsrt examples/vm_surface.js
```

## 11. Next major milestones

The immediate next major milestones now sit at the boundary between the remaining core language semantics and the broader architecture/measurement work that follows them.

### Near-next execution targets

L2 prototype-chain lookup is now part of the bounded runtime surface. The next execution targets build on that base.

1. **L1 — Benchmark and profiling harness**
   - Add a repeatable performance corpus and measurement harness.
   - Establish baseline numbers before optimization or architecture expansion.

2. **L4 — Class syntax and desugaring/direct support**
   - Layer class syntax on top of the bounded constructor/prototype runtime.
   - Keep the runtime model narrow unless syntax support proves more primitives are necessary.

3. **L5 — Module system**
   - Add import/export semantics and multi-file execution.
   - Treat it as a separate milestone from the current single-file runtime contract.

### Larger long-horizon targets

```text
L4  class syntax and desugaring/direct support
L5  module system (import/export)
L6  embeddable host API
L7  async/event-loop experiment
L8  regex + richer standard library
L9  conformance subset harness
L10 optional JIT / advanced optimization feasibility study
```

## 12. Practical reading of the docs tree

If you want the shortest path to understanding the repo, read in this order:

1. `README.md`
2. `docs/project-status-and-architecture.md`
3. `docs/semantics-baseline.md`
4. `docs/ownership-guardrails.md`
5. `docs/runtime-error-model.md`
6. `docs/vm-parity-backend.md` and `docs/bytecode-compiler-mvp.md`
7. `.omx/plans/prd-javascript-runtime-roadmap.md`

The `docs/*.md` files named after milestones are best read as **milestone snapshots**. This file is the current synthesis across those snapshots.

## 13. References

The following resources are the most useful starting set for building a project like this from scratch. These are the references I would treat as the required study list for re-implementing the repo from first principles.

- **Robert Nystrom, _Crafting Interpreters_** — practical, end-to-end guidance for both a tree-walk interpreter and a bytecode VM; especially relevant for the frontend pipeline, environments, closures, and VM structure.  
  https://craftinginterpreters.com/

- **Alfred V. Aho, Monica S. Lam, Ravi Sethi, Jeffrey D. Ullman, _Compilers: Principles, Techniques, and Tools (2nd ed.)_** — the classic compiler reference for lexing, parsing, semantic analysis, IR thinking, and control-flow/code-generation fundamentals.  
  https://www.pearson.com/en-us/subject-catalog/p/compilers-principles-techniques-and-tools/P200000003472/9780133002140

- **Keith D. Cooper and Linda Torczon, _Engineering a Compiler (2nd ed.)_** — a very strong companion to the Dragon Book for practical compiler construction, IR design, and lowering choices.  
  https://www.oreilly.com/library/view/engineering-a-compiler/9780080916613/

- **Richard Jones, Antony Hosking, Eliot Moss, _The Garbage Collection Handbook_** — the best single reference for tracing GC design, root management, and the tradeoffs behind bounded collectors like the one used here.  
  https://gchandbook.org/

- **C. J. Cheney, “A Nonrecursive List Compacting Algorithm” (CACM, 1970)** — foundational GC paper; useful background for understanding tracing collection and why simple collector designs stay so influential.  
  https://cacm.acm.org/research/a-nonrecursive-list-compacting-algorithm/

- **Roberto Ierusalimschy, Luiz Henrique de Figueiredo, Waldemar Celes, “The Implementation of Lua 5.0”** — one of the best compact case studies for a small dynamic language runtime and VM implemented in C.  
  https://www.lua.org/doc/sblp2005.pdf

- **The official Lua reference manuals** — not because `jsrt` is Lua-like in syntax, but because the Lua manuals show how to describe a compact dynamic language and runtime surface precisely.  
  https://www.lua.org/manual/

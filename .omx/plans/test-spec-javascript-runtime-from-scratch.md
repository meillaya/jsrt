# Test Spec — Tiny JavaScript Interpreter in C

## Scope
This test spec covers milestone 1 for `.omx/plans/prd-javascript-runtime-from-scratch.md`: a small C interpreter that supports expressions, variables, conditionals, functions, and end-to-end script execution.

## Test Strategy
- Prefer fast C unit tests for lexer, parser, and evaluator subsystems.
- Add a small integration harness that runs example scripts through the main interpreter entry point.
- Keep fixtures tiny and readable so failures teach something immediately.
- Verify both success paths and clear failure handling for unsupported/invalid input.

## Test Layers

### 1. Build verification
**Goal:** confirm the project builds reproducibly.
- `make`
- `make test`
- optional `make clean && make`

### 2. Lexer unit tests
**Target files:** `src/lexer.c`, `include/jsrt/token.h`
- tokenizes identifiers, keywords, numbers, punctuation, and operators
- preserves source position metadata
- rejects/flags invalid characters cleanly

Example cases:
- `let x = 1;`
- `if (x > 2) { x = x + 1; }`
- `function add(a, b) { return a + b; }`

### 3. Parser unit tests
**Target files:** `src/parser.c`, `src/ast.c`
- operator precedence and associativity
- variable declarations and assignments
- block statements
- `if` / `else`
- function declarations and call expressions
- `return`
- invalid syntax diagnostics

Example cases:
- `1 + 2 * 3`
- `let x = 4; x = x + 1;`
- `if (x) { return 1; } else { return 2; }`
- `function inc(n) { return n + 1; } inc(4);`

### 4. Evaluator unit tests
**Target files:** `src/eval.c`, `src/env.c`, `src/value.c`
- arithmetic evaluation
- variable lookup and reassignment
- lexical scope chaining
- conditional branching
- function calls with parameters
- return propagation out of nested blocks

Expected behaviors:
- expressions produce correct numeric results
- branch selection matches truthy/falsy rules chosen for milestone 1
- function calls create child environments correctly

### 5. Integration tests
**Target files:** `src/main.c`, `examples/*.js`
- run `examples/arithmetic.js`
- run `examples/branching.js`
- run `examples/functions.js`
- verify expected output and zero exit status

### 6. Negative-path tests
- parse error on malformed syntax (missing `)` / `}` / `;` where required)
- runtime error for undefined variable
- runtime error for calling a non-function value
- unsupported syntax should fail clearly rather than silently misbehave

## Acceptance Mapping

| PRD acceptance criterion | Test proof |
| --- | --- |
| Build succeeds | `make`, `make test` |
| Lexer works | lexer unit tests |
| Parser works | parser unit tests |
| Expressions + variables work | evaluator unit tests |
| Conditionals work | evaluator + integration tests |
| Functions work | evaluator + integration tests |
| Example scripts run | integration tests on `examples/*.js` |
| Ownership is understandable | documented destroy rules + reviewer/verifier inspection |

## Suggested Test Harness Shape
- `tests/test_runner.c` composes subsystem test suites.
- Helper assertions for token streams, AST node expectations, and runtime values.
- Integration helper can execute fixture files and compare stdout/stderr/exit status.

## Verification Commands
```sh
make
make test
./build/jsrt examples/arithmetic.js
./build/jsrt examples/branching.js
./build/jsrt examples/functions.js
```

## Exit Criteria
Milestone 1 is test-ready when:
1. Every supported syntax feature has at least one unit test.
2. Every example program runs through the CLI path.
3. Negative-path tests cover malformed syntax and basic runtime misuse.
4. The verifier can trace each PRD acceptance criterion to at least one automated or clearly repeatable manual check.

## Known Deferrals
These are intentionally not required in milestone 1 test coverage:
- garbage collection
- bytecode execution
- async/event loop behavior
- modules/classes/prototypes
- host API integration

# PRD — Tiny JavaScript Interpreter in C

## Metadata
- Source brief: `.omx/specs/deep-interview-javascript-runtime-from-scratch.md`
- Planning mode: ralplan / consensus-style direct planning
- Project state: greenfield (repo currently contains OMX metadata only)
- Recommended execution lane: `$ralph` after plan approval

## Evidence Base
- The clarified goal is a **learning-first** project centered on **low-level systems practice in C**, not broad JS compatibility (`.omx/specs/deep-interview-javascript-runtime-from-scratch.md`).
- Milestone 1 is explicitly a **tiny interpreter**, not a VM/JIT/event-loop runtime.
- Out of scope for milestone 1: GC, VM/JIT, async/event loop, advanced OO/module features, and browser/Node host APIs.
- Completion requires running small scripts with variables, conditionals, and functions end to end.

## RALPLAN-DR Summary

### Principles
1. Prefer the smallest end-to-end slice that teaches core interpreter internals.
2. Keep memory ownership explicit and visible in C.
3. Choose simplicity over engine realism in milestone 1.
4. Make every milestone demonstrable with tests and sample scripts.
5. Defer complexity unless it directly improves the learning objective.

### Decision Drivers
1. Maximize low-level C systems learning value.
2. Reach a runnable milestone quickly enough to sustain momentum.
3. Preserve a clean path for later expansion into richer runtime internals.

### Viable Options

#### Option A — Tree-walk interpreter over an AST (Chosen)
- Pros:
  - Smallest complete path from source text to execution.
  - Teaches lexer, parser, AST, environments, and evaluation clearly.
  - Keeps memory ownership visible without forcing VM machinery too early.
- Cons:
  - Less engine-like than bytecode execution.
  - Performance will be modest.

#### Option B — Bytecode compiler + VM from milestone 1
- Pros:
  - Closer to real engine architecture.
  - Creates a natural path to optimization work later.
- Cons:
  - Adds compiler, instruction set, VM, and debugging complexity too early.
  - Dilutes the user's stated goal of a small learning-first milestone.

#### Option C — Expression evaluator first, parser/functions later
- Pros:
  - Very fast first demo.
  - Lowest implementation risk.
- Cons:
  - Too narrow to satisfy the agreed completion proof.
  - Delays the most educational parts: scopes, functions, and control flow.

### Decision
Adopt **Option A**: build a small tree-walk interpreter in C with a hand-written lexer and recursive-descent parser.

### Invalidation Rationale
- Option B is rejected for milestone 1 because VM/JIT complexity was explicitly deferred.
- Option C is rejected because it fails the agreed success criteria of running small scripts with conditionals and functions.

## Requirements Summary

### User Intent
Build a JavaScript runtime from scratch as a C learning exercise, with the primary value coming from direct exposure to parsing, evaluation, memory ownership, and runtime data structures.

### Milestone 1 Outcome
A command-line interpreter that can load a small script file, parse a narrow JavaScript-like subset, and execute programs that use:
- numeric literals
- variables / assignment
- arithmetic and comparison operators
- block statements
- `if` / `else`
- function declarations
- function calls
- `return`

### Non-goals
- Garbage collection
- Bytecode VM / JIT
- Async/event loop
- Modules, classes, prototypes
- Browser/Node host APIs
- Broad ECMAScript compatibility

## Proposed Repository Structure

```text
Makefile
README.md
include/jsrt/
  token.h
  ast.h
  parser.h
  value.h
  env.h
  eval.h
  memory.h
src/
  main.c
  lexer.c
  parser.c
  ast.c
  value.c
  env.c
  eval.c
  memory.c
examples/
  arithmetic.js
  branching.js
  functions.js
tests/
  test_lexer.c
  test_parser.c
  test_eval.c
  test_runner.c
```

## Architecture Plan

### Language subset
Support only the syntax needed for milestone 1 demos:
- statements: expression statement, variable declaration, block, if/else, function declaration, return
- expressions: literals, identifiers, unary ops, binary ops, assignment, call expressions

### Execution model
- Use a **tree-walk interpreter** over parsed AST nodes.
- Represent runtime values with a tagged union (`number`, `bool`, `null`, `function`).
- Represent lexical scope with chained environments.
- Represent functions as user-defined objects storing parameter lists, body AST, and defining environment.

### Memory model
- Start with manual allocation/free.
- Centralize ownership rules in `memory.[ch]` and AST teardown helpers.
- Prefer explicit destroy functions per major structure over ad hoc frees.
- Avoid introducing arenas unless profiling or complexity later justifies them.

### Build/tooling choice
- Use a simple `Makefile` with `cc`, `-std=c17`, `-Wall`, `-Wextra`, `-Werror`, `-pedantic`, and a debug target.
- No external dependencies.
- Test binaries should compile from the same source modules used by the interpreter.

## Acceptance Criteria
1. `make` builds the interpreter successfully on the target development environment.
2. The lexer tokenizes supported syntax with deterministic test coverage.
3. The parser builds ASTs for supported statements/expressions and rejects unsupported syntax cleanly.
4. The evaluator executes arithmetic, variable binding, conditionals, and user-defined functions.
5. `examples/arithmetic.js`, `examples/branching.js`, and `examples/functions.js` run successfully.
6. `make test` passes unit and integration checks for the milestone-1 feature set.
7. Memory ownership is documented well enough that future contributors can extend the interpreter without guessing who frees what.

## Implementation Steps

### Step 1 — Scaffold the greenfield repo
**Files:** `Makefile`, `README.md`, `include/jsrt/*.h`, `src/main.c`, `tests/test_runner.c`
- Create project layout and baseline build/test targets.
- Add a minimal CLI entry point that reads a source file path.
- Document milestone-1 scope and non-goals in `README.md`.

### Step 2 — Implement the lexer
**Files:** `include/jsrt/token.h`, `src/lexer.c`, `tests/test_lexer.c`
- Define token kinds for the supported subset.
- Implement scanning with line/column tracking for diagnostics.
- Add focused lexer tests for keywords, punctuation, operators, numbers, and identifiers.

### Step 3 — Implement AST structures and parser
**Files:** `include/jsrt/ast.h`, `include/jsrt/parser.h`, `src/ast.c`, `src/parser.c`, `tests/test_parser.c`
- Define AST node structures and destruction helpers.
- Implement a recursive-descent parser for milestone-1 grammar.
- Add parser tests for precedence, blocks, conditionals, function declarations, calls, and invalid syntax.

### Step 4 — Implement values, environments, and memory helpers
**Files:** `include/jsrt/value.h`, `include/jsrt/env.h`, `include/jsrt/memory.h`, `src/value.c`, `src/env.c`, `src/memory.c`
- Add tagged runtime values.
- Add chained lexical environments.
- Centralize allocation/free helpers and ownership notes.

### Step 5 — Implement evaluator core
**Files:** `include/jsrt/eval.h`, `src/eval.c`, `tests/test_eval.c`
- Evaluate expressions, assignments, and block statements.
- Support control flow with `if` / `else`.
- Implement user-defined function declaration/call/return behavior.

### Step 6 — Add end-to-end examples and CLI execution
**Files:** `src/main.c`, `examples/*.js`, `tests/test_runner.c`
- Wire the CLI to read, parse, evaluate, and print failures clearly.
- Add sample programs proving the required milestone behavior.
- Add an integration-style test path that executes example scripts.

### Step 7 — Document extension path for later milestones
**Files:** `README.md`, optional `docs/milestones.md`
- Capture deferred features: GC, bytecode VM, richer object model, host APIs.
- Describe the recommended next milestone ordering so expansion stays intentional.

## Risks and Mitigations

| Risk | Why it matters | Mitigation |
| --- | --- | --- |
| Scope drift toward a “real engine” | Would undermine the learning-first milestone | Treat non-goals as binding; defer VM/GC/async work to later milestones |
| Memory bugs from manual ownership | Likely in a C interpreter project | Centralize destroy paths and require tests to free constructed objects |
| Parser complexity grows too quickly | Could stall momentum early | Keep grammar intentionally narrow and reject unsupported syntax explicitly |
| Weak diagnostics slow learning | Hard to debug early language work | Add line/column info in tokens and surface parser errors clearly |
| Tests lag behind implementation | Makes refactors unsafe | Add unit tests per subsystem before expanding language coverage |

## Verification Steps
1. `make`
2. `make test`
3. Run each example script through the CLI and compare output/exit status to expected behavior.
4. Optionally run the test binary under `valgrind` or platform equivalent in a later hardening pass.

## ADR
- **Decision:** Build milestone 1 as a tree-walk interpreter in C with a hand-written lexer, recursive-descent parser, explicit AST, chained lexical environments, and manual memory management.
- **Drivers:** Learning value in C, small runnable milestone, preserved path for later runtime expansion.
- **Alternatives considered:** Bytecode VM first; expression evaluator first.
- **Why chosen:** Best match for the clarified user intent and non-goal constraints.
- **Consequences:** Milestone 1 favors clarity and learning over performance and JavaScript breadth.
- **Follow-ups:** After milestone 1, consider milestone 2 options such as arrays/objects, improved diagnostics, then GC or bytecode only if the learning goal still supports them.

## Available-Agent-Types Roster
- `executor` — primary implementation lane for the interpreter scaffold and core modules
- `architect` — architecture check if parser/value model tradeoffs become unclear
- `test-engineer` — test-shape review and regression-hardening lane
- `verifier` — completion evidence and acceptance-criteria audit
- `writer` — README and milestone docs refinement

## Follow-up Staffing Guidance

### Recommended `$ralph` lane
- Owner: `executor`
- Review/support lanes: `test-engineer`, then `verifier`
- Suggested reasoning: high for executor, medium for test-engineer, high for verifier
- Why: work is sequential and strongly benefits from one persistent owner across scaffold -> lexer -> parser -> eval -> verification.

### Optional `$team` lane if execution expands
- Lane 1: `executor` for scaffold + lexer
- Lane 2: `executor` for parser + AST
- Lane 3: `test-engineer` for harness + examples + verification scripts
- Close with `verifier` to audit milestone acceptance criteria

## Team Launch Hint
- Ralph path: `$ralph .omx/plans/prd-javascript-runtime-from-scratch.md`
- Team path: `$team .omx/plans/prd-javascript-runtime-from-scratch.md`

## Team Verification Path
If `$team` is used later, the team should prove:
1. Build succeeds from a clean checkout.
2. Unit tests cover lexer, parser, and evaluator behavior.
3. Example scripts execute end to end.
4. Memory ownership conventions are documented.
5. A final verifier pass maps the implementation back to the milestone-1 acceptance criteria.

# PRD — Long-Horizon Roadmap for `c_javascript_runtime`

## Scope and intent
This roadmap assumes the current baseline already exists: a small tree-walk JavaScript-like interpreter in C with lexer, parser, AST, evaluator, strings, arrays/objects, property/index assignment, `while`/`for`, `break`/`continue`, parser recovery, closure-cycle collection, examples, and tests.

It is designed to be execution-ready for later use by **Ralph** or **Team**, and it intentionally covers both:
- **currently in-scope evolution** of the current interpreter, and
- **currently out-of-scope / stretch goals** already deferred in the repo (`full heap GC`, `VM/JIT`, `async`, `modules/classes/prototypes`, host APIs, broad ECMAScript compatibility).

---

## RALPLAN-DR summary

### Principles
1. Preserve a runnable, teachable interpreter at every major milestone.
2. Prefer regression safety and semantic clarity before breadth.
3. Isolate high-risk runtime rewrites behind design-first milestones.
4. Keep near-term work aligned with the current learning-first architecture.
5. Treat stretch goals as explicit branches, not accidental scope creep.

### Roadmap-level semantic gate
- After `N1` and `N2`, only semantically low-risk surface-area work may proceed until the runtime-foundation gate is complete.
- The runtime-foundation gate is: `N7 -> N8 -> M1 -> M2`.
- Milestones that would materially depend on unsettled aggregate/value/runtime semantics must wait until that gate is complete or explicitly declare why they are insulated from it.

### Top 3 decision drivers
1. **Learning value:** maximize insight into language/runtime internals in C.
2. **Execution safety:** sequence risky internals work so the current green baseline stays recoverable.
3. **Future optionality:** keep a credible path from interpreter -> richer runtime -> experimental engine work.

### Viable options

#### Option A — Language-surface-first roadmap
- **Pros:** visible progress quickly; easier demos; lower short-term design overhead
- **Cons:** compounds interpreter debt; makes future GC/VM shifts harder

#### Option B — Runtime-internals-first roadmap
- **Pros:** stronger long-term foundation; better path to performance/backend work
- **Cons:** slower visible feature growth; higher near-term complexity

#### Option C — Balanced staged roadmap with explicit stretch lanes **(chosen)**
- **Pros:** matches repo state; enables safe near-term wins while preparing for GC/VM work
- **Cons:** requires discipline to keep stretch work from polluting near-term milestones

---

## ADR

- **Decision:** Adopt a balanced three-horizon roadmap: near-term hardening plus small language/runtime improvements, mid-term semantics/runtime architecture expansion, and long-term platform/engine stretch goals.
- **Drivers:** learning value, safety of execution, extensibility of architecture.
- **Alternatives considered:** language-first; internals-first; balanced staged roadmap.
- **Why chosen:** it best fits the current repo, existing milestone trajectory, and the repo’s explicitly deferred goals.
- **Consequences:** some attractive goals remain intentionally deferred; several milestones require design/ADR work before implementation.
- **Follow-ups:** convert selected milestones into per-milestone PRD + test-spec artifacts before execution; use Team for cross-cutting milestones and Ralph for bounded single-owner slices.

---

# Roadmap

## Near-term horizon

### N1. Semantics baseline and ownership guardrails
- **Horizon:** Near-term
- **Goal:** Freeze current behavior and memory-ownership assumptions before adding new surface area.
- **Key deliverables:** current-feature behavior matrix; ownership/lifetime notes for values/env/gc; regression checklist
- **Dependencies:** current baseline only
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `verifier`, `writer`
- **Verification shape:** `make clean && make test`; run all existing examples; compare behavior matrix to actual outputs
- **Risk level:** Low

### N2. Golden diagnostics harness
- **Horizon:** Near-term
- **Goal:** Make parser/runtime diagnostics regression-proof.
- **Key deliverables:** golden error fixtures; caret/span expectations; multi-error parse fixtures
- **Dependencies:** N1
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `test-engineer`, `verifier`
- **Verification shape:** targeted lexer/parser/eval diagnostics tests; snapshot diffs stay stable
- **Risk level:** Low

### N3. `do...while` support
- **Horizon:** Near-term
- **Goal:** Close a low-risk control-flow gap.
- **Key deliverables:** lexer/parser/eval support; tests; example; docs update
- **Dependencies:** N2
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `test-engineer`, `verifier`
- **Verification shape:** `make test`; end-to-end example; invalid-syntax diagnostics
- **Risk level:** Medium

### N4. `switch` / `case` / `default` MVP
- **Horizon:** Near-term
- **Goal:** Add structured branching without jumping to broad JS compatibility.
- **Key deliverables:** AST/parser/eval changes; fallthrough policy; `break` semantics; tests/examples
- **Dependencies:** N2
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `test-engineer`, `verifier`
- **Verification shape:** parser/eval suites; nested control-flow tests; example run
- **Risk level:** Medium

### N5. Function expressions and anonymous closures
- **Horizon:** Near-term
- **Goal:** Improve composability before deeper object/model changes.
- **Key deliverables:** parser/AST/eval support; closure tests; example coverage
- **Dependencies:** N2
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `test-engineer`, `verifier`
- **Verification shape:** parser/eval tests for capture behavior; regression against existing function declarations
- **Risk level:** Medium

### N6. `const` and declaration-rule refinement
- **Horizon:** Near-term
- **Goal:** Tighten variable semantics and error reporting.
- **Key deliverables:** `const`; redeclaration/reassignment rules; diagnostics; tests
- **Dependencies:** N5
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `test-engineer`, `verifier`
- **Verification shape:** scope/assignment diagnostics tests; example coverage
- **Risk level:** Medium

### N7. General heap GC architecture spike
- **Horizon:** Near-term
- **Goal:** Decide how to evolve from closure-cycle collection toward broader heap management.
- **Key deliverables:** ADR for roots, object coverage, collector style, migration sequence, rollback boundaries
- **Go/no-go gate:** must define adoption criteria, abort criteria, rollback path, and which downstream milestones it unlocks or cancels
- **Dependencies:** N1, N2
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `critic`, `verifier`
- **Verification shape:** design review artifact; root inventory; explicit non-goals and migration cut-lines
- **Risk level:** High

### N8. General heap GC MVP
- **Horizon:** Near-term
- **Goal:** Extend GC beyond closure/environment cycles in a bounded way.
- **Key deliverables:** managed allocation path for selected heap objects; root traversal; teardown simplification; tests
- **Dependencies:** N7
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `executor`, `debugger`, `test-engineer`, `verifier`
- **Verification shape:** `make test`; ASan build/tests; allocation stress programs; leak/crash regression passes
- **Risk level:** High

## Mid-term horizon

### M1. GC observability and stress rig
- **Horizon:** Mid-term
- **Goal:** Make collector behavior inspectable before larger runtime changes.
- **Key deliverables:** debug counters/flags; stress fixtures; GC trace mode; troubleshooting notes
- **Dependencies:** N8
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `executor`, `debugger`, `test-engineer`
- **Verification shape:** repeated stress runs; ASan/regression sweeps; trace sanity checks
- **Risk level:** Medium

### M2. Aggregate semantics ADR (`copy` vs `reference`)
- **Horizon:** Mid-term
- **Goal:** Decide whether arrays/objects stay copy-isolated or move toward shared-reference semantics.
- **Key deliverables:** ADR; compatibility matrix; migration/no-migration plan
- **Go/no-go gate:** must define adoption criteria, abort criteria, rollback path, and whether `M3` is required, deferred, or cancelled
- **Dependencies:** N1, M1
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `critic`, `verifier`
- **Verification shape:** option review against current tests/examples; explicit breakage map
- **Risk level:** High

### M3. Mutable aggregate/reference model implementation
- **Horizon:** Mid-term
- **Goal:** Implement the chosen aggregate semantics if the ADR moves beyond current copy-style behavior.
- **Key deliverables:** runtime/value/env changes; aliasing rules; assignment semantics update; docs/tests
- **Dependencies:** M2
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `executor`, `debugger`, `test-engineer`, `verifier`
- **Verification shape:** aliasing and nested mutation suites; memory-safety passes; old/new semantic fixture comparison
- **Risk level:** High

### M4. Built-in library foundation
- **Horizon:** Mid-term
- **Goal:** Introduce a minimal built-in function mechanism without full host embedding.
- **Key deliverables:** native-call abstraction; small standard built-ins (`print`, length/introspection helpers, etc.); docs/tests
- **Dependencies:** N5, N6
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `writer`, `verifier`
- **Verification shape:** built-in invocation tests; example programs; diagnostics for misuse
- **Risk level:** Medium

### M5. Exceptionless structured runtime error model
- **Horizon:** Mid-term
- **Goal:** Normalize propagation/reporting of runtime errors as the runtime grows.
- **Key deliverables:** clearer internal error/result paths; categorized runtime errors; improved reporting consistency
- **Dependencies:** N2, M4
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `verifier`
- **Verification shape:** targeted runtime-failure tests; diagnostic consistency snapshots
- **Risk level:** Medium

### M6. Bytecode / IR architecture spike
- **Horizon:** Mid-term
- **Goal:** Define a credible path from tree-walk evaluation to a compiled backend.
- **Key deliverables:** instruction set draft; compiler boundary; runtime stack model; parity target
- **Go/no-go gate:** must define adoption criteria, abort criteria, rollback path, and whether `M7`/`M8` proceed
- **Dependencies:** N7, N8, M2
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `critic`, `verifier`
- **Verification shape:** design review; instruction coverage matrix; AST-to-IR mapping samples
- **Risk level:** High

### M7. Bytecode compiler MVP
- **Horizon:** Mid-term
- **Goal:** Compile a bounded subset of the current AST into bytecode.
- **Key deliverables:** compiler pass; bytecode format; disassembler; fixtures
- **Dependencies:** M6
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `executor`, `build-fixer`, `test-engineer`, `verifier`
- **Verification shape:** compiler golden tests; disassembly snapshots; backend smoke programs
- **Risk level:** High

### M8. VM interpreter parity backend
- **Horizon:** Mid-term
- **Goal:** Run the supported subset on a stack VM and compare against tree-walk semantics.
- **Key deliverables:** VM loop; backend switch; parity harness; example compatibility matrix
- **Backend semantic-authority rule:** tree-walk remains canonical semantics until VM parity is explicitly declared complete; post-`M8` features must either ship in both backends or declare a temporary tree-walk-only gate with an exit plan
- **Dependencies:** M7
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `executor`, `debugger`, `performance-reviewer`, `verifier`
- **Verification shape:** tree-walk vs VM parity suite; full example sweep; ASan/debug builds
- **Risk level:** High

## Long-term horizon

### L1. Benchmark and profiling harness
- **Horizon:** Long-term
- **Goal:** Measure current performance before advanced optimization or platform work.
- **Key deliverables:** benchmark corpus; timing harness; profiling notes; baseline thresholds
- **Dependencies:** M8
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `performance-reviewer`, `verifier`
- **Verification shape:** repeatable benchmark runs; documented baseline tables
- **Risk level:** Medium

### L2. Prototype chain and method dispatch
- **Horizon:** Long-term
- **Goal:** Introduce object inheritance semantics.
- **Key deliverables:** object model update; prototype lookup chain; method dispatch rules; tests
- **Dependencies:** M2, M8
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `test-engineer`, `verifier`
- **Verification shape:** lookup/mutation regressions; prototype edge-case fixtures
- **Risk level:** High

### L3. `this` binding and constructor semantics
- **Horizon:** Long-term
- **Goal:** Support constructor-style object patterns.
- **Key deliverables:** `this` resolution; constructor calls; `new` semantics; diagnostics/tests
- **Dependencies:** L2
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `debugger`, `verifier`
- **Verification shape:** constructor behavior tests; invalid-call diagnostics; interaction with methods
- **Risk level:** High

### L4. Class syntax and desugaring/direct support
- **Horizon:** Long-term
- **Goal:** Add higher-level OO syntax over the chosen runtime model.
- **Key deliverables:** parser support; desugaring or direct runtime implementation; docs/examples
- **Dependencies:** L3
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `writer`, `verifier`
- **Verification shape:** parser snapshots; class behavior tests; end-to-end examples
- **Risk level:** High

### L5. Module system (`import` / `export`)
- **Horizon:** Long-term
- **Goal:** Move from single-file execution toward multi-file programs.
- **Key deliverables:** loader/resolution rules; module environment semantics; integration fixtures
- **Dependencies:** M8
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `test-engineer`, `verifier`
- **Verification shape:** multi-file integration tests; module-cycle/error diagnostics; fixture suite
- **Risk level:** High

### L6. Embeddable host API
- **Horizon:** Long-term
- **Goal:** Allow C host programs to expose native functions/values into the runtime.
- **Key deliverables:** embedding API; native ABI; sample host integration; docs
- **Dependencies:** M4, L5
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `writer`, `verifier`
- **Verification shape:** sample host build; API contract tests; docs walkthrough
- **Risk level:** High

### L7. Async/event-loop experiment
- **Horizon:** Long-term
- **Goal:** Explore task queue semantics without promising Node/browser parity.
- **Key deliverables:** event queue model; timers/task scheduling MVP; explicit design limits; tests
- **Dependencies:** M8
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `executor`, `debugger`, `verifier`
- **Verification shape:** deterministic async fixtures; shutdown/queue safety tests; traceable event ordering
- **Risk level:** High

### L8. Regex and richer standard-library experiment
- **Horizon:** Long-term
- **Goal:** Explore selective feature breadth after runtime foundations stabilize.
- **Key deliverables:** regex strategy decision (minimal/custom/wrapped); string/array utility growth; tests
- **Dependencies:** M4, L1
- **Recommended execution mode:** `ralph`
- **Suggested primary lanes/roles:** `executor`, `researcher`, `verifier`
- **Verification shape:** targeted feature tests; compatibility notes; performance sanity checks
- **Risk level:** Medium

### L9. Conformance subset harness
- **Horizon:** Long-term
- **Goal:** Define how close the runtime aims to get to broader JavaScript behavior.
- **Key deliverables:** curated conformance suite; unsupported-feature matrix; roadmap gap report
- **Dependencies:** M8, L8
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `researcher`, `test-engineer`, `verifier`
- **Verification shape:** reproducible conformance runs; categorized pass/fail reports
- **Risk level:** Medium

### L10. Optional JIT / advanced optimization feasibility study
- **Horizon:** Long-term
- **Goal:** Decide whether the project should remain a teaching runtime or grow into engine experimentation.
- **Key deliverables:** feasibility ADR; hotspot analysis; prototype scope boundaries; no-go/go recommendation
- **Go/no-go gate:** must define adoption criteria, abort criteria, rollback path, and whether any optimization prototype is allowed to fork from the teaching-runtime baseline
- **Dependencies:** M8, L1, L9
- **Recommended execution mode:** `team`
- **Suggested primary lanes/roles:** `architect`, `performance-reviewer`, `critic`, `verifier`
- **Verification shape:** benchmark evidence; hotspot profiles; explicit feasibility decision record
- **Risk level:** High

---

# Available-agent-types roster

| Agent type | Primary use in this roadmap | Suggested reasoning |
|---|---|---|
| `explore` | repo-local discovery, impact mapping, current-pattern lookup | low |
| `planner` | milestone decomposition, PRD/test-spec drafting | medium |
| `architect` | semantics, GC, VM, modules, async, API design | high |
| `executor` | core implementation lane | high |
| `debugger` | runtime regressions, GC/VM failure diagnosis | high |
| `test-engineer` | regression strategy, fixtures, parity/stress harnesses | medium |
| `verifier` | completion gate, evidence review, terminal validation | high |
| `critic` | challenge risky designs and migration plans | high |
| `researcher` | external semantics/reference research where needed | high |
| `writer` | README/ADR/roadmap/docs polish | high |
| `performance-reviewer` | benchmarking, profiling, hotspot analysis | medium |
| `build-fixer` | build/test/toolchain failures during deeper runtime work | high |
| `security-reviewer` | relevant once embedding/host API surface appears | medium |

---

# Staffing guidance

## Ralph: when to prefer it
Use **Ralph** for bounded, single-owner milestones with limited parallel coordination needs.

**Best-fit milestones:**
- N1, N2, N3, N5, N6
- M4
- L1, L8

**Typical Ralph lane shape**
- Primary lane: `executor` — **high**
- Validation lane/checkpoint: `verifier` — **high**
- Optional docs lane: `writer` — **high**

### Ralph launch hints
```text
$ralph Execute roadmap milestone N1: semantics baseline and ownership guardrails for jsrt. Produce a behavior matrix, ownership notes, regression checklist, and fresh verification evidence.
```

```text
$ralph Execute roadmap milestone N3: add do...while support to jsrt with parser/evaluator/tests/examples/docs and fresh verification evidence.
```

```text
$ralph Execute roadmap milestone M4: add a minimal built-in library foundation to jsrt with docs/tests/examples and fresh verification evidence.
```

### Ralph checkpoint / verification loop
1. convert the roadmap milestone into a milestone-specific PRD + test-spec pair before coding
2. keep one primary implementation lane active at a time
3. require a verifier checkpoint before declaring the milestone complete
4. for memory-sensitive milestones, collect sanitizer evidence in addition to normal tests
5. retain an evidence bundle: fresh test output, example output, and any milestone-specific stress/parity/benchmark proof

### Ralph completion proof
- `make clean && make test`
- milestone example/fixture run
- sanitizer run for memory-sensitive milestones:
  ```bash
  make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
  ```
- if the milestone changes backend semantics or performance posture, add parity/benchmark evidence before exit

## Team: when to prefer it
Use **Team** for cross-cutting, design-heavy, or high-risk milestones.

**Best-fit milestones:**
- N4, N7, N8
- M1, M2, M3, M5, M6, M7, M8
- L2, L3, L4, L5, L6, L7, L9, L10

**Recommended Team phase shapes**

### Design-heavy phase
- Worker role: `architect`
- Headcount: 2–3
- Reasoning: **high**
- Use for: N7, M2, M6, L2/L5/L7 design phases

### Implementation-heavy runtime phase
- Worker role: `executor`
- Headcount: 3–4
- Reasoning: **high**
- Use for: N8, M3, M7, M8, L3, L4, L6

### Verification-heavy phase
- Worker role: `test-engineer` or `verifier`
- Headcount: 2–3
- Reasoning: `test-engineer` **medium**, `verifier` **high**
- Use for: parity validation, conformance runs, stress/ASan proof

**Practical Team note**
Because `omx team` uses one worker role per run, mixed-role work should usually be staged:
1. `architect` phase
2. `executor` phase
3. `test-engineer` / `verifier` phase

---

# Explicit `$team` / `omx team` launch hints

## Design spike launch
```bash
omx team 2:architect "Roadmap milestone N7: design general heap GC architecture for jsrt; deliver root inventory, collector strategy, migration plan, and top risks"
```

## Runtime implementation launch
```bash
omx team 4:executor "Roadmap milestone N8: implement general heap GC MVP for jsrt; split work into allocation migration, root traversal, regression fixtures, and evidence bundle"
```

## Semantics ADR launch
```bash
omx team 3:architect "Roadmap milestone M2: decide jsrt aggregate semantics (copy vs reference); produce ADR, compatibility matrix, and migration recommendation"
```

## Backend/compiler launch
```bash
omx team 4:executor "Roadmap milestone M7: build bytecode compiler MVP for jsrt; split compiler pass, bytecode format, disassembler fixtures, and smoke verification"
```

## Verification launch
```bash
omx team 3:test-engineer "Roadmap milestone M8 verification: prove tree-walk vs VM parity with fixture coverage, example sweep, and sanitizer evidence"
```

## Skill-style equivalent
```text
$team 4:executor "Roadmap milestone N8: implement general heap GC MVP for jsrt..."
```

---

# Concrete Team verification path

## 1. Pre-launch
- prepare or reuse a milestone-specific context snapshot
- define milestone task lanes before launch
- ensure verification ownership is explicit in the prompt

## 2. Launch by phase
- use one worker role per phase
- choose headcount based on milestone scope
- encode delivery lanes directly in the task text

## 3. Minimum lane model
- **Lane A:** feature/runtime delivery
- **Lane B:** regression/tests/fixtures
- **Lane C:** sanitizer/stress or profiling evidence
- **Lane D:** final verification bundle

## 4. Monitor with runtime evidence
```bash
omx team status <team-name> --json
omx team await <team-name> --timeout-ms 30000 --json
```

## 5. Required milestone proof
At minimum:
- new behavior has direct tests
- prior examples still pass unless intentionally changed
- a new example/fixture exists for new surface area
- sanitizer evidence exists for memory-sensitive milestones using the explicit CFLAGS override command below unless/until a dedicated Make target exists
- parity evidence exists for backend milestones
- benchmark evidence exists for performance milestones

## 6. Completion gate
Do not treat a milestone as complete until:
- `pending=0`
- `in_progress=0`
- `failed=0` or explicitly triaged with an approved follow-up path

## 7. Evidence bundle to retain
- test command outputs
- example run outputs
- ASan/stress output for memory work using:
  ```bash
  make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
  ```
- parity/benchmark reports for VM/perf work
- milestone ADR notes when the milestone is design-affecting

---

# Recommended initial execution order

## Phase 1 — harden current baseline
N1 -> N2

## Phase 2 — runtime-foundation gate
N7 -> N8 -> M1 -> M2

## Phase 3 — low-risk language and semantics wins after the gate
N3 -> N4 -> N5 -> N6

## Phase 4 — resolve post-gate semantic direction
M3 -> M4 -> M5

## Phase 5 — backend evolution
M6 -> M7 -> M8 -> L1

## Phase 6 — stretch branches
- OO lane: L2 -> L3 -> L4
- Modules/embedding lane: L5 -> L6
- Async experiment lane: L7
- Conformance/optimization lane: L8 -> L9 -> L10

---

# Highest-value first milestones to convert into executable follow-up plans
1. **N1** — Semantics baseline and ownership guardrails
2. **N4** — `switch` / `case` / `default` MVP
3. **N7** — General heap GC architecture spike
4. **N8** — General heap GC MVP
5. **M6** — Bytecode / IR architecture spike

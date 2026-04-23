# jsrt Milestone Verification Checklist

Use this checklist when executing future roadmap milestones via Ralph or Team.

## Pre-implementation

- [ ] milestone PRD exists
- [ ] milestone test-spec exists
- [ ] baseline impact is understood (`docs/semantics-baseline.md`)
- [ ] ownership/runtime impact is understood (`docs/ownership-guardrails.md`)

## During implementation

- [ ] changed semantics are covered by narrow regression tests
- [ ] at least one example or fixture demonstrates new visible behavior
- [ ] docs are updated if the milestone changes the visible/runtime baseline

## Required fresh evidence

- [ ] `make clean && make test`
- [ ] relevant example runs succeed
- [ ] diagnostics or parser recovery changes have targeted failing-case tests

## Memory-sensitive/runtime-sensitive milestones

- [ ] ASan verification run:

```sh
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## L2 prototype-chain / method-dispatch exit gates

Use this addendum when verifying the bounded L2 milestone. L2 is done only when the new lookup model lands **without** leaking L3 semantics.

- [ ] prototype linkage is explicit and bounded (for example `setPrototypeOf(obj, protoOrNull)`), with negative coverage for invalid receiver/prototype values, self-linking, and cycles
- [ ] property reads are verified as own-first, then prototype-chain lookup, including multi-hop lookup and full-chain miss failure/message coverage
- [ ] direct writes remain receiver-only; shadowing creates/updates own properties instead of mutating prototype-owned entries
- [ ] inherited intermediate-container assignment is covered explicitly: inherited container reads may resolve through the prototype chain, but the final write mutates only the resolved container and does not invent an own receiver-local container
- [ ] method dispatch is still plain-function invocation after prototype-aware lookup; no implicit `this`, constructor behavior, `new`, or class semantics are introduced
- [ ] `len(obj)`, object equality, and object stringification are explicitly verified to remain own-only in L2
- [ ] evaluator and VM suites cover the same supported positive and negative cases before milestone closure
- [ ] `docs/semantics-baseline.md` is updated to describe the bounded L2 surface, while keeping `this` / `new` / constructors / classes deferred to L3
- [ ] the ASan lane is re-run after the final object-graph changes land, and the exact command/result are captured in the completion evidence bundle

## L3 `this` / constructor / `new` exit gates

Use this addendum when verifying the bounded L3 milestone.

- [ ] bare/top-level `this` and plain/extracted calls are explicitly verified as `null`
- [ ] `obj.method()` and `obj["method"]()` bind `this` to the concrete receiver, including prototype-inherited methods
- [ ] `new Ctor(args)` is verified only in the approved parenthesized L3 form; bare `new Ctor` and broader postfix/call chaining stay rejected
- [ ] user functions expose only the narrow `prototype` meta-property; non-`prototype` function property reads/writes remain rejected with type diagnostics
- [ ] native-function constructor policy is locked: `builtin.prototype` reads as `null`, writes fail, and `new builtin()` fails
- [ ] bytecode/disassembly evidence covers `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW`
- [ ] docs/examples show constructor + prototype-method interaction while keeping the broader class surface deferred to L4

## L4 bounded class-declaration exit gates

Use this addendum when verifying the bounded L4 milestone.

- [x] `class Name { ... }` declarations are verified across parser, evaluator, compiler, and VM
- [x] supported class behavior is locked: optional constructor, identifier-named instance methods, default constructor behavior, prototype-installed methods, and extracted-method `this = null`
- [x] `Name()` without `new` is rejected on both backends with stable runtime type diagnostics
- [x] `Name.prototype` / `Name["prototype"]` remain readable and writable under the existing object-or-null rules
- [x] unsupported syntax (`extends`, `super`, static members, fields, getters/setters, computed names, class expressions) stays parser-owned with stable diagnostics
- [x] bytecode/disassembly proof still routes lowered classes through `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW` with no new class-specific invocation opcode
- [x] docs/examples describe the bounded L4 class surface while keeping inheritance and the broader ES class feature set deferred

## L4 class syntax / desugaring exit gates

Use this addendum when verifying the bounded L4 milestone.

- [x] bounded `class Name { ... }` declarations are covered on both evaluator and VM backends
- [x] class bindings are construct-only, while plain calls fail with stable type diagnostics
- [x] omitted constructors, prototype-installed methods, extracted-method `this = null`, and `Name.prototype` read/write parity are all explicitly verified
- [x] unsupported `extends`, `super`, static members, fields, accessors, computed names, and class expressions remain parser-owned rejections with source diagnostics
- [x] docs/examples describe the bounded class surface and the deliberate no-TDZ sequential declaration model
- [x] bytecode/disassembly evidence shows class lowering still routes through `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW`

## Design-heavy milestones

- [ ] architect review completed
- [ ] critic review completed when the milestone is ADR/spike-heavy or high risk
- [ ] adoption/abort/rollback conditions are recorded for speculative architecture work

## Completion bundle

- [ ] changed files listed
- [ ] simplifications made listed
- [ ] remaining risks listed
- [ ] milestone-specific follow-ups recorded explicitly

## L4 closure record — 2026-04-23

This section formally closes the bounded L4 milestone against the checklist above.

Generic prerequisite/evidence/completion gates were satisfied for L4 via the approved PRD + test spec, the implementation/planning commits, the retained evidence below, and the high-risk design review artifacts (`architect` + `critic`) referenced from the L4 planning docs.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1035 assertions`
- `./build/jsrt examples/class_syntax.js` → `7:true`
- `JSRT_BACKEND=vm ./build/jsrt examples/class_syntax.js` → `7:true`
- Bytecode/disassembly evidence is locked in `tests/test_bytecode.c`, including proof that lowered class execution still uses `CALL`, `CALL_METHOD`, and `CALL_NEW`, with no class-specific invocation opcode.
- Parser/diagnostic rejection coverage is locked in `tests/test_parser.c`, `tests/test_diagnostics.c`, and `tests/test_vm.c`.

### Changed files in the L4 milestone implementation

- `.omx/plans/prd-javascript-runtime-l4-class-syntax-and-desugaring.md`
- `.omx/plans/ralplan-javascript-runtime-l4-class-syntax-and-desugaring-draft.md`
- `.omx/plans/test-spec-javascript-runtime-l4-class-syntax-and-desugaring.md`
- `README.md`
- `docs/milestone-verification-checklist.md`
- `docs/project-status-and-architecture.md`
- `docs/semantics-baseline.md`
- `examples/class_syntax.js`
- `include/jsrt/ast.h`
- `include/jsrt/bytecode.h`
- `include/jsrt/token.h`
- `include/jsrt/value.h`
- `src/ast.c`
- `src/compiler.c`
- `src/eval.c`
- `src/lexer.c`
- `src/parser.c`
- `src/vm.c`
- `tests/test_bytecode.c`
- `tests/test_diagnostics.c`
- `tests/test_eval.c`
- `tests/test_lexer.c`
- `tests/test_parser.c`
- `tests/test_vm.c`

### Simplifications made

- Kept first-slice L4 bounded to `class Name { ... }` declarations with an optional constructor and identifier-named instance methods.
- Reused the landed L3 constructor/prototype runtime instead of introducing a parallel class object model.
- Preserved the existing `BC_CALL` / `BC_CALL_METHOD` / `BC_CALL_NEW` family instead of inventing class-specific dispatch opcodes.
- Kept unsupported class surface parser-owned so out-of-scope features fail early and explicitly.

### Remaining risks

- `extends`, `super`, static members, fields, accessors, computed names, and class expressions remain intentionally deferred and must not be inferred as partially supported.
- Future inheritance work will likely require explicit superclass/home-object metadata rather than a trivial extension of this first-slice lowering.
- The bounded no-TDZ sequential declaration model is deliberate for this slice and should not be widened silently.

### Milestone-specific follow-ups

- Treat `extends` / `super` as later work requiring explicit superclass/home-object metadata and possibly wider function heritage affordances.
- Reassess after L4 whether a later L4.x follow-up should own inheritance/static surface separately.
- Do not widen the class surface without revisiting the approved L4 PRD and test spec.

## M8 closure record — 2026-04-23

This section formally closes the bounded **M8 — VM interpreter parity backend** milestone against its approved PRD/test-spec and the retained repo evidence below.

This closure record applies to the **initial bounded M8 subset**. Later milestones widened the backend surface further, but that later work does not replace the need to record that the original opt-in VM/parity milestone itself landed and was verified.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `JSRT_BACKEND=vm ./build/jsrt examples/vm_subset.js` → `number:5:12`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1035 assertions`
- Tree-walk vs VM parity coverage is retained in `tests/test_vm.c`.
- Current-facing milestone companion and contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-m8-vm-interpreter-parity-backend.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-m8-vm-interpreter-parity-backend.md`
  - `docs/vm-parity-backend.md`

### Changed files in the M8 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-m8-vm-interpreter-parity-backend.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m8-vm-interpreter-parity-backend.md`
- `Makefile`
- `README.md`
- `docs/semantics-baseline.md`
- `docs/vm-parity-backend.md`
- `examples/vm_subset.js`
- `include/jsrt/bytecode.h`
- `include/jsrt/value.h`
- `include/jsrt/vm.h`
- `src/compiler.c`
- `src/eval.c`
- `src/main.c`
- `src/value.c`
- `src/vm.c`
- `tests/test.h`
- `tests/test_runner.c`
- `tests/test_vm.c`

### Simplifications made

- Kept tree-walk as the semantic authority instead of promoting the VM to default execution.
- Kept M8 bounded to an opt-in backend with explicit parity evidence rather than pretending full-language VM parity.
- Used an explicit CLI backend switch (`JSRT_BACKEND=vm`) instead of introducing a more complex runtime-selection surface.

### Remaining risks

- The original M8 subset was intentionally narrower than the current shipped VM surface; this closure should not be misread as proving full VM parity for every later-added feature.
- Unsupported backend surface at the time of M8 still required explicit compiler/backend failures rather than silent fallback behavior.
- Broader closure/upvalue hardening and later surface widening were still follow-up work at the original M8 boundary.

### Milestone-specific follow-ups

- Keep tree-walk as the semantic oracle unless broader parity evidence justifies any future promotion.
- Treat later VM widening (`M9`, `M10`) as distinct follow-on closure work rather than retroactively inflating M8.
- Keep unsupported backend surface explicit until it is truly parity-tested.

## N8 closure record — 2026-04-23

This section formally closes the bounded **N8 — General Heap GC MVP** milestone against its approved PRD/test-spec and the retained repo evidence below.

This closure record applies to the bounded managed-heap MVP, not to a hypothetical universal GC for every allocation in the process. The closure should be read together with the documented guardrail that N8 stayed intentionally narrower than full heap-management ambitions.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `./build/jsrt examples/gc_stress.js` → `85`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1035 assertions`
- Current-facing milestone companion and contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
  - `docs/n8-general-heap-gc-mvp-notes.md`
- Current runtime evidence for the bounded managed path is retained in:
  - `include/jsrt/gc.h`
  - `src/gc.c`
  - `src/eval.c`
  - `src/value.c`
  - `examples/gc_stress.js`

### Changed files in the N8 milestone implementation

The original N8 implementation diff is not cleanly recoverable as one single historical commit from the current rewritten repository history. The current authoritative implementation surface for the landed N8 milestone is:

- `.omx/plans/prd-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-n8-general-heap-gc-mvp.md`
- `docs/n8-general-heap-gc-mvp-notes.md`
- `docs/project-status-and-architecture.md`
- `include/jsrt/gc.h`
- `src/gc.c`
- `src/eval.c`
- `src/value.c`
- `examples/gc_stress.js`
- `tests/test_eval.c`

### Simplifications made

- Kept N8 bounded to managed runtime objects (`Env`, `JsFunction`, `JsArray`, `JsObject`) instead of widening into full-process ownership.
- Reused explicit evaluator safe points and temporary roots instead of switching to conservative stack scanning or allocation-triggered collection.
- Preserved then-current language semantics while moving ownership onto the bounded managed path.

### Remaining risks

- N8 is not universal/full heap GC and should not be read as such.
- Frontend/manual objects such as AST nodes, source buffers, tokens, and diagnostic buffers remain outside the managed path.
- Later runtime work still needs to respect the bounded-temp-root model and the explicit object-family boundary unless a new design milestone says otherwise.

### Milestone-specific follow-ups

- Keep N8 documentation explicit that it is a bounded managed-heap MVP, not broad/full heap collection.
- Treat later GC observability, aggregate-semantics, and backend-planning work as follow-on milestones rather than retroactively widening N8.
- Do not widen ownership scope without revisiting the N7/N8 ADR + PRD constraints.

## M3 closure record — 2026-04-23

This section formally closes the bounded **M3 — Mutable aggregate/reference model implementation** milestone against its approved PRD/test-spec and the retained repo evidence below.

This closure record applies to the repo’s bounded shared-reference aggregate model: arrays and objects now preserve identity across environment flow and alias-visible mutation, while structural equality/stringification remain intentionally bounded and cycle-safe. It does **not** claim broad ECMAScript-complete object/reference semantics.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `./build/jsrt examples/references.js` → `5:9:7`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1035 assertions`
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-m3-mutable-aggregate-reference-model.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-m3-mutable-aggregate-reference-model.md`
- Current runtime/baseline evidence for the landed M3 surface is retained in:
  - `docs/semantics-baseline.md`
  - `docs/project-status-and-architecture.md`
  - `examples/references.js`
  - `src/value.c`
  - `tests/test_eval.c`

### Changed files in the M3 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-m3-mutable-aggregate-reference-model.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m3-mutable-aggregate-reference-model.md`
- `README.md`
- `docs/ownership-guardrails.md`
- `docs/semantics-baseline.md`
- `examples/references.js`
- `src/value.c`
- `tests/test_eval.c`

### Simplifications made

- Removed deep cloning from aggregate value flow instead of inventing a separate aliasing model.
- Preserved structural equality and stringification for acyclic values while hardening both against self-referential structures.
- Kept the milestone bounded to aggregate identity/mutation semantics instead of widening it into a broader language redesign.

### Remaining risks

- The shared-reference aggregate model is still intentionally narrower than full JavaScript object/reference semantics.
- Future aggregate work must preserve cycle-safe stringification/equality unless a later milestone intentionally revisits that contract.
- Broader coercion/equality/host-interaction questions remain outside this milestone’s closure boundary.

### Milestone-specific follow-ups

- Keep later aggregate work explicit about whether it changes equality, stringification, or host-facing mutation semantics.
- Treat future aggregate/runtime changes as follow-on milestones rather than retroactively broadening M3.
- Preserve cycle-safety guarantees unless a later milestone deliberately replaces them with a new tested contract.

## L1 closure record — 2026-04-23

This section formally closes the bounded **L1 — Benchmark and profiling harness** milestone against its approved PRD/test-spec and the retained repo evidence below.

This closure record applies to the repo’s coarse end-to-end CLI timing workflow only. It does **not** claim internal phase instrumentation, portable profiler integration, or cross-machine-comparable performance numbers.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `make bench-smoke` → `bench-smoke: ok`
- `make bench ITERATIONS=1 WARMUPS=0` → live CSV output confirming the frozen schema and tree-walk harness execution
- `benchmarks/baseline.csv` retains a checked-in baseline using the same frozen CSV schema
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-l1-benchmark-and-profiling-harness.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-l1-benchmark-and-profiling-harness.md`
  - `docs/benchmark-and-profiling-harness.md`

### Changed files in the L1 milestone implementation

- `Makefile`
- `benchmarks/aggregates.js`
- `benchmarks/arithmetic_control.js`
- `benchmarks/baseline.csv`
- `benchmarks/calls_closures.js`
- `benchmarks/gc_churn.js`
- `benchmarks/manifest.csv`
- `benchmarks/vm_subset.js`
- `docs/benchmark-and-profiling-harness.md`
- `scripts/benchmark.sh`

### Simplifications made

- Kept L1 as an external harness around the existing CLI instead of instrumenting runtime internals.
- Froze one manifest contract and one CSV schema early so later benchmark work can compare like with like.
- Kept performance workflow outside `make test` so timing claims stay separate from correctness.

### Remaining risks

- L1 numbers are same-machine relative baselines, not portable or cross-machine guarantees.
- The benchmark workflow is intentionally coarse and should not be confused with parser/compiler/runtime phase attribution.
- Future benchmark contract changes must preserve or explicitly version the manifest/schema surface to avoid drift.

### Milestone-specific follow-ups

- Preserve the manifest contract and frozen CSV header unless intentionally versioning the benchmark workflow.
- Treat deeper profiling/instrumentation as later work rather than retroactively widening L1.
- Keep timing assertions out of `make test` unless a future milestone explicitly changes that process contract.

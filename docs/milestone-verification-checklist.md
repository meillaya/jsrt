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

## M1 closure record — 2026-04-23

This section formally closes the bounded **M1 — GC observability and stress rig** milestone against its approved PRD/test-spec and the retained repo evidence below.

This closure record applies to the repo’s bounded GC observability surface: counters, trace hooks, temp-root tracking, and a repeatable stress fixture. It does **not** claim a collector-policy rewrite, allocation-triggered collection, or broader GC semantics changes.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `make clean && make debug` → debug-capable build completed successfully
- `JSRT_GC_TRACE=1 ./build/jsrt examples/gc_stress.js` → trace output emitted plus final program value `85`
- `tail -n 8 build/gc-trace.log` → retained trace lines including `reason=process-cleanup` and live/swept/temp-root counters
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-m1-gc-observability.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-m1-gc-observability.md`
  - `docs/gc-observability-and-stress.md`

### Changed files in the M1 milestone implementation

The original M1 implementation diff is not isolated as one clean historical commit in the current repository history. The current authoritative implementation surface for the landed M1 milestone is:

- `.omx/plans/prd-javascript-runtime-roadmap-m1-gc-observability.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m1-gc-observability.md`
- `docs/gc-observability-and-stress.md`
- `docs/project-status-and-architecture.md`
- `include/jsrt/gc.h`
- `src/gc.c`
- `examples/gc_stress.js`
- `tests/test_eval.c`

### Simplifications made

- Added observability around the existing bounded collector rather than changing collection policy.
- Kept the stress rig grounded in currently supported semantics instead of inventing a synthetic benchmark-only GC workload.
- Used an opt-in trace flag (`JSRT_GC_TRACE`) rather than making GC telemetry part of normal execution output.

### Remaining risks

- M1 observability is intentionally bounded and should not be misread as a full GC debugging/profiling framework.
- Trace output is useful for manual inspection but is not a stable external API contract beyond the documented debugging workflow.
- Later GC/runtime work must preserve the distinction between observability and collector-policy changes unless a later milestone intentionally revises it.

### Milestone-specific follow-ups

- Keep GC trace mode opt-in unless a future milestone intentionally changes that UX contract.
- Treat deeper collector-policy or allocation-trigger work as later milestones rather than retroactively widening M1.
- Preserve the stress fixture as a semantics-grounded regression tool, not just a debugging demo.

## M7 closure record — 2026-04-23

This section formally closes the bounded **M7 — Bytecode Compiler MVP** milestone against its approved PRD/test-spec and the retained repo evidence below.

This closure record applies to the original compiler-only M7 boundary: bytecode representation, AST-to-bytecode lowering for the bounded subset, stable disassembly, and explicit compiler diagnostics for deferred surface. It does **not** retroactively absorb later VM execution work from M8 or later widening from M9/M10.

### Evidence retained

- `make clean && make test` → `Tests passed: 1035 assertions`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1035 assertions`
- Compiler/disassembly evidence is retained in `tests/test_bytecode.c`, including stable disassembly checks and explicit unsupported-surface diagnostics.
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-m7-bytecode-compiler-mvp.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-m7-bytecode-compiler-mvp.md`
  - `docs/bytecode-compiler-mvp.md`
- Historical implementation closure is also anchored by commit `fbf7d57` (`Turn the backend plan into a concrete compiler/disassembler artifact`).

### Changed files in the M7 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-m7-bytecode-compiler-mvp.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-m7-bytecode-compiler-mvp.md`
- `Makefile`
- `README.md`
- `docs/bytecode-compiler-mvp.md`
- `examples/bytecode_fixture.js`
- `include/jsrt/bytecode.h`
- `include/jsrt/compiler.h`
- `src/bytecode.c`
- `src/compiler.c`
- `tests/test.h`
- `tests/test_bytecode.c`
- `tests/test_runner.c`

### Simplifications made

- Kept M7 compiler-only instead of collapsing compiler and VM execution risk into one milestone.
- Used stable disassembly and compiler-focused tests rather than inventing a separate CLI/disassembly product surface.
- Preferred explicit compiler diagnostics for deferred constructs over ambiguous partial lowering.

### Remaining risks

- The original M7 compiler-only boundary is narrower than the current shipped compiler surface after later milestones.
- Later backend milestones widened compiler support; this closure should not be read as a complete proof for every currently supported backend feature.
- Disassembly formatting remains a contract that later work should change only deliberately and with test updates.

### Milestone-specific follow-ups

- Preserve explicit unsupported-surface diagnostics unless a later milestone truly implements the deferred surface.
- Treat VM execution and later widening as distinct milestones rather than retroactively broadening M7.
- Keep disassembly stability intentional and test-backed if formatting evolves.

## L5 closure record — 2026-05-15

This section formally closes the bounded **L5 — Module system (`import` / `export`)** milestone against the ultragoal story objective and retained repo evidence below.

This closure applies to the intentionally narrow tree-walk module surface: static named imports from string-literal file specifiers, declaration exports, relative path resolution from the importing file, exported-name enforcement, and explicit VM-backend deferral. It does **not** claim full ECMAScript module compatibility.

### Evidence retained

- `make clean && make test` → `Tests passed: 1062 assertions`
- `./build/jsrt examples/modules.js` → `82`
- `JSRT_BACKEND=vm ./build/jsrt examples/modules.js` rejects module syntax with the L5 tree-walk-only compiler diagnostic.
- Lexer/parser/evaluator coverage is retained in:
  - `tests/test_lexer.c`
  - `tests/test_parser.c`
  - `tests/test_eval.c`
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-l5-module-system.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-l5-module-system.md`
  - `docs/module-system.md`

### Changed files in the L5 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-l5-module-system.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-l5-module-system.md`
- `README.md`
- `docs/module-system.md`
- `docs/project-status-and-architecture.md`
- `docs/repo-milestone-status.md`
- `docs/semantics-baseline.md`
- `examples/module_math.js`
- `examples/modules.js`
- `include/jsrt/ast.h`
- `include/jsrt/eval.h`
- `include/jsrt/token.h`
- `src/ast.c`
- `src/compiler.c`
- `src/eval.c`
- `src/lexer.c`
- `src/main.c`
- `src/parser.c`
- `tests/test_eval.c`
- `tests/test_lexer.c`
- `tests/test_parser.c`

### Simplifications made

- Chose static named imports and declaration exports only instead of attempting default exports, namespace imports, dynamic imports, or package resolution.
- Kept module execution on the tree-walk backend and made VM deferral explicit through a compiler diagnostic.
- Reused the existing environment, diagnostics, and function/closure machinery rather than adding a separate module object model.
- Rejected circular imports instead of introducing partially initialized bindings.

### Remaining risks

- Module records are process-local evaluation artifacts rather than a stable public module cache API.
- Relative path handling is intentionally simple and not a package resolver.
- VM parity for modules is deferred and must be designed deliberately if a later milestone needs it.

### Milestone-specific follow-ups

- Do not widen module syntax without updating `docs/module-system.md`, `docs/semantics-baseline.md`, and parser/evaluator tests.
- Preserve the explicit VM diagnostic until module lowering is implemented and parity-tested.
- Treat cyclic modules and package resolution as separate future decisions, not implied L5 behavior.

## L6 closure record — 2026-05-15

This section formally closes the bounded **L6 — Embeddable host API** milestone against the ultragoal story objective and retained repo evidence below.

This closure applies to a small source/file evaluation C API with explicit runtime lifecycle, caller-owned result strings, and structured runtime error kind propagation. It does **not** claim a stable ABI, native host callback interface, direct mutable `Value` handles, custom allocator support, or VM backend embedding.

### Evidence retained

- `make clean && make test` → `Tests passed: 1075 assertions`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1075 assertions`
- `cc -std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g examples/embed_host.c $(find build/obj/src -name '*.o' ! -name 'main.o' | sort) -o build/embed_host_example && ./build/embed_host_example` → `42`
- Host API coverage is retained in `tests/test_runtime.c`, including source evaluation, runtime diagnostics, result cleanup, and file/module evaluation.
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-l6-embeddable-host-api.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-l6-embeddable-host-api.md`
  - `docs/embeddable-host-api.md`

### Changed files in the L6 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-l6-embeddable-host-api.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-l6-embeddable-host-api.md`
- `Makefile`
- `README.md`
- `docs/embeddable-host-api.md`
- `docs/project-status-and-architecture.md`
- `docs/repo-milestone-status.md`
- `docs/semantics-baseline.md`
- `examples/embed_host.c`
- `include/jsrt/runtime.h`
- `src/runtime.c`
- `tests/test.h`
- `tests/test_runner.c`
- `tests/test_runtime.c`

### Simplifications made

- Returned final values as owned text instead of exposing mutable runtime `Value` handles across the API boundary.
- Reused the existing lexer/parser/tree-walk evaluator and diagnostics path rather than introducing a separate host execution engine.
- Kept VM backend selection and native callback registration outside L6 to avoid unstable API commitments.

### Remaining risks

- The API is intentionally small and source/file-oriented; embedders needing native callbacks or direct value manipulation need a future milestone.
- `JsrtRuntime` is a lifecycle handle, not a full isolated realm or configurable runtime policy object.
- API/ABI stability is not promised beyond this repository's bounded learning runtime contract.

### Milestone-specific follow-ups

- Do not expose direct `Value` handles without a documented ownership and lifetime model.
- Keep CLI behavior separate from host API behavior unless a future milestone deliberately unifies them.
- Add host callback and custom allocator support only with dedicated tests and docs.

## L7 closure record — 2026-05-15

This section formally closes the bounded **L7 — Async / event-loop experiment** milestone against the ultragoal story objective and retained repo evidence below.

This closure applies to the explicit deferred-callback experiment exposed through `defer(callback)` and `runJobs()`. It does **not** implement promises, `async` / `await`, timers, I/O events, microtasks, concurrency, or host event-loop integration.

### Evidence retained

- `make test` → `Tests passed: 1080 assertions`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1080 assertions`
- Deferred-callback coverage is retained in `tests/test_eval.c`, including closure mutation, FIFO drain behavior, and non-function callback diagnostics.
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-l7-async-event-loop-experiment.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-l7-async-event-loop-experiment.md`
  - `docs/async-event-loop-experiment.md`

### Changed files in the L7 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-l7-async-event-loop-experiment.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-l7-async-event-loop-experiment.md`
- `README.md`
- `docs/async-event-loop-experiment.md`
- `docs/project-status-and-architecture.md`
- `docs/repo-milestone-status.md`
- `docs/semantics-baseline.md`
- `src/eval.c`
- `tests/test_eval.c`

### Simplifications made

- Used explicit built-ins instead of syntax-level `async` / `await`.
- Reused the existing function invocation path for callbacks.
- Kept callbacks synchronous at `runJobs()` time to avoid unowned host scheduler semantics.

### Remaining risks

- This is an experiment and should not be presented as JavaScript promise compatibility.
- There is no scheduler fairness, cancellation, timer, or I/O model.
- VM support for `defer` / `runJobs` depends on the existing native built-in path but broader async lowering is out of scope.

### Milestone-specific follow-ups

- Any future promise/microtask work must get a new semantics doc and test-spec.
- Preserve queued callback GC rooting if the evaluator safe-point model changes.
- Do not add host event-loop integration without extending the L6 host API contract deliberately.

## L8 closure record — 2026-05-15

This section formally closes the bounded **L8 — Regex + richer standard library** milestone against the ultragoal story objective and retained repo evidence below.

This closure applies to dependency-free string helpers and a tiny regex matcher exposed through `strContains`, `strSlice`, and `regexMatch`. It does **not** implement ECMAScript `RegExp`, regex literals, captures, flags, alternation, character classes, Unicode classes, or replacement APIs.

### Evidence retained

- `make test` → `Tests passed: 1087 assertions`
- `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` → `Tests passed: 1087 assertions`
- String/regex built-in coverage is retained in `tests/test_eval.c`, including success cases and type/range diagnostics.
- Current-facing milestone contract artifacts are retained in:
  - `.omx/plans/prd-javascript-runtime-roadmap-l8-regex-richer-stdlib.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-l8-regex-richer-stdlib.md`
  - `docs/regex-and-stdlib.md`

### Changed files in the L8 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-l8-regex-richer-stdlib.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-l8-regex-richer-stdlib.md`
- `README.md`
- `docs/project-status-and-architecture.md`
- `docs/regex-and-stdlib.md`
- `docs/repo-milestone-status.md`
- `docs/semantics-baseline.md`
- `src/eval.c`
- `tests/test_eval.c`

### Simplifications made

- Added a tiny dependency-free regex subset instead of bringing in an external regex dependency.
- Used built-ins instead of adding regex literal syntax.
- Kept string helpers byte-indexed and explicitly range-checked.

### Remaining risks

- `regexMatch` is intentionally small and should not be marketed as ECMAScript `RegExp`.
- Byte-indexed slicing is simple but not Unicode-aware.
- Additional string/regex APIs need dedicated tests and docs before being considered supported.

### Milestone-specific follow-ups

- Do not add regex syntax without parser diagnostics and semantics docs.
- Keep dependency-free behavior unless a later milestone explicitly accepts a dependency tradeoff.
- If richer regex semantics are added, rename or version docs so L8's tiny subset remains understandable.

## L9 closure record — 2026-05-15

This section formally closes the bounded **L9 — Conformance subset harness** milestone against the ultragoal story objective and retained repo evidence below.

This closure applies to a repo-local CSV-manifest harness for positive/negative fixtures with tree-walk and VM backend applicability. It does **not** import Test262 or claim broad ECMAScript compatibility.

### Evidence retained

- `make test` → `Tests passed: 1087 assertions`
- `make conformance` → `Conformance passed: 7 case(s)`
- Harness artifacts are retained in:
  - `conformance/manifest.csv`
  - `conformance/positive/`
  - `conformance/negative/`
  - `scripts/conformance.sh`
  - `docs/conformance-subset-harness.md`

### Changed files in the L9 milestone implementation

- `.omx/plans/prd-javascript-runtime-roadmap-l9-conformance-subset-harness.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-l9-conformance-subset-harness.md`
- `Makefile`
- `README.md`
- `conformance/manifest.csv`
- `conformance/positive/arithmetic.js`
- `conformance/positive/module_lib.js`
- `conformance/positive/module_main.js`
- `conformance/negative/const_assign.js`
- `conformance/negative/division_by_zero.js`
- `conformance/negative/import_lib.js`
- `conformance/negative/import_hidden.js`
- `docs/conformance-subset-harness.md`
- `docs/project-status-and-architecture.md`
- `docs/repo-milestone-status.md`
- `docs/semantics-baseline.md`
- `scripts/conformance.sh`

### Simplifications made

- Used a simple CSV manifest instead of dynamic discovery or external test-suite import.
- Checked stable stdout and diagnostic substrings instead of whole diagnostic snapshots.
- Added backend applicability per case so tree-only features do not masquerade as VM parity.

### Remaining risks

- The manifest parser intentionally does not support complex CSV escaping.
- The harness is a project subset, not an ECMAScript conformance certification.
- More fixtures are needed over time to make the subset representative.

### Milestone-specific follow-ups

- Add a conformance fixture whenever a future milestone changes externally visible behavior.
- Keep backend applicability explicit for every fixture.
- Do not import large external conformance suites without a dedicated dependency/process decision.

## L10 closure record — 2026-05-15

This section formally closes the bounded **L10 — Optional JIT / advanced optimization feasibility study** milestone against the ultragoal story objective and retained repo evidence below.

This closure is documentation/decision-only. It recommends deferring native JIT implementation and pursuing bytecode peephole optimization plus literal-only constant folding first. It does **not** implement a runtime optimizer or native code generator.

### Evidence retained

- `make test` → `Tests passed: 1087 assertions`
- `make conformance` → `Conformance passed: 7 case(s)`
- Feasibility artifacts are retained in:
  - `docs/optimization-feasibility-study.md`
  - `.omx/plans/adr-javascript-runtime-l10-optimization-feasibility.md`
  - `.omx/plans/prd-javascript-runtime-roadmap-l10-optimization-feasibility.md`
  - `.omx/plans/test-spec-javascript-runtime-roadmap-l10-optimization-feasibility.md`

### Changed files in the L10 milestone implementation

- `.omx/plans/adr-javascript-runtime-l10-optimization-feasibility.md`
- `.omx/plans/prd-javascript-runtime-roadmap-l10-optimization-feasibility.md`
- `.omx/plans/test-spec-javascript-runtime-roadmap-l10-optimization-feasibility.md`
- `README.md`
- `docs/optimization-feasibility-study.md`
- `docs/project-status-and-architecture.md`
- `docs/repo-milestone-status.md`
- `docs/semantics-baseline.md`

### Simplifications made

- Closed L10 as an ADR-backed feasibility decision instead of adding speculative optimizer code.
- Preferred bytecode/local optimization follow-ups over native JIT complexity.
- Grounded future optimization work in existing tests, conformance fixtures, and benchmark harnesses.

### Remaining risks

- No performance improvement is delivered by L10 itself.
- Future optimizer work still needs implementation-specific tests and benchmark evidence.
- Native JIT feasibility may change if project goals shift toward performance over portability/clarity.

### Milestone-specific follow-ups

- Start with a dedicated O1 bytecode peephole optimizer milestone if optimization work continues.
- Require an executable-memory/security ADR before any native JIT experiment.
- Keep the tree-walk evaluator as semantic oracle for optimization validation.

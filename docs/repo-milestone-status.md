# jsrt Milestone Status Ledger

This document is the **canonical milestone-status synthesis** for `jsrt` as of **2026-04-23**.

Use this file to answer **which roadmap milestones are done, which remain open, and which older source statements are now stale or superseded**.

Document-role boundaries:
- `docs/project-status-and-architecture.md` is the canonical **current-state architecture and implementation overview**.
- `.omx/plans/prd-javascript-runtime-roadmap.md` and milestone-local artifacts under `.omx/plans/` remain **planning/history artifacts**.
- Older milestone companion notes under `docs/` may accurately capture milestone scope at the time they were written, even when some of their framing is now stale relative to the current repo.

## Interpretation rule

- **Basis = Evidence** means the status is directly supported by current repo sources.
- **Basis = Evidence + Inference** means the repo contains strong current evidence, but the exact milestone closure is synthesized across multiple sources rather than stated in one single closure record.
- **Explicit closure artifact** means the repo contains a milestone-specific completion/handoff artifact that directly records closure (for example an accepted ADR handoff or a checklist closure record).
- **Current-state synthesis** means the repo clearly ships the milestone’s bounded surface today, but the ledger is inferring milestone completion from shipped docs/code/tests rather than pointing at one dedicated closure artifact.

## Milestone ledger

| Milestone | Status | Basis | Evidence | Notes |
|---|---|---|---|---|
| N1 — Semantics baseline and ownership guardrails | DONE | Evidence | `docs/semantics-baseline.md`; `docs/ownership-guardrails.md` | The bounded semantics baseline and ownership notes are checked-in as first-class docs. |
| N2 — Golden diagnostics harness | DONE | Evidence | `docs/diagnostics-golden-harness.md`; `tests/test_diagnostics.c` | Golden lexer/parser/runtime diagnostic coverage is explicitly documented and present in tests. |
| N3 — `do...while` support | DONE | Evidence | `README.md`; `examples/do_while.js`; `tests/test_eval.c`; `src/parser.c` | Current repo advertises and tests `do...while`. |
| N4 — `switch` / `case` / `default` MVP | DONE | Evidence | `README.md`; `examples/switch.js`; `tests/test_eval.c`; `src/parser.c` | Current repo advertises and tests `switch`. |
| N5 — Function expressions and anonymous closures | DONE | Evidence | `README.md`; `examples/function_expressions.js`; `docs/project-status-and-architecture.md`; `tests/test_eval.c` | Function expressions and closures are part of the current supported surface. |
| N6 — `const` and declaration-rule refinement | DONE | Evidence | `README.md`; `examples/const.js`; `tests/test_eval.c`; `tests/test_parser.c` | `const` is part of the current supported surface with rule coverage. |
| N7 — General heap GC architecture spike | DONE | Evidence | `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md`; `.omx/plans/test-spec-javascript-runtime-roadmap-n7-general-heap-gc-spike.md` | Design/ADR milestone: accepted direction artifact exists and explicitly hands off to N8. |
| N8 — General heap GC MVP | DONE | Evidence | `docs/project-status-and-architecture.md`; `docs/milestone-verification-checklist.md`; `docs/n8-general-heap-gc-mvp-notes.md`; `src/gc.c`; `include/jsrt/gc.h`; `examples/gc_stress.js` | Current-state docs and runtime code explicitly describe and implement the bounded managed-heap path for `Env`, `JsFunction`, `JsArray`, and `JsObject`, and the checklist now contains an explicit N8 closure record. |
| M1 — GC observability and stress rig | DONE | Evidence | `docs/gc-observability-and-stress.md`; `docs/milestone-verification-checklist.md`; `include/jsrt/gc.h`; `examples/gc_stress.js` | GC counters, trace mode, and stress coverage are documented and implemented, and the checklist now contains an explicit M1 closure record. |
| M2 — Aggregate semantics ADR (`copy` vs `reference`) | DONE | Evidence | `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md`; `docs/aggregate-semantics-adr.md` | Design/ADR milestone: the decision artifact exists. Some companion wording is now stale; see appendix below. |
| M3 — Mutable aggregate/reference model implementation | DONE | Evidence | `docs/semantics-baseline.md`; `docs/milestone-verification-checklist.md`; `docs/project-status-and-architecture.md`; `examples/references.js`; `tests/test_eval.c` | Current baseline explicitly says arrays/objects preserve shared identity and alias-visible mutation, and the checklist now contains an explicit M3 closure record. |
| M4 — Built-in library foundation | DONE | Evidence | `docs/built-in-library-foundation.md`; `README.md`; `src/eval.c`; `tests/test_eval.c` | `print`, `len`, `typeOf`, and `setPrototypeOf` exist as the bounded built-in surface. |
| M5 — Exceptionless structured runtime error model | DONE | Evidence | `docs/runtime-error-model.md`; `tests/test_diagnostics.c`; `src/eval.c` | Structured runtime error categories and source-based reporting are current behavior. |
| M6 — Bytecode / IR architecture spike | DONE | Evidence | `docs/bytecode-ir-architecture-spike.md`; `.omx/plans/adr-javascript-runtime-m6-bytecode-ir-direction.md` | Design/ADR milestone: backend direction artifact exists and handed off to M7/M8. |
| M7 — Bytecode compiler MVP | DONE | Evidence | `docs/bytecode-compiler-mvp.md`; `docs/milestone-verification-checklist.md`; `src/compiler.c`; `tests/test_bytecode.c` | Compiler and disassembler are present and documented, and the checklist now contains an explicit M7 closure record. |
| M8 — VM interpreter parity backend | DONE | Evidence | `docs/vm-parity-backend.md`; `docs/milestone-verification-checklist.md`; `README.md`; `src/vm.c`; `tests/test_vm.c` | Opt-in VM backend exists and now has an explicit closure record for the original bounded parity milestone. |
| M9 — VM surface widening I | DONE | Evidence + Inference | `docs/vm-parity-backend.md`; `docs/project-status-and-architecture.md`; `tests/test_vm.c` | The current bounded VM subset already includes aggregate operations plus the structured control-flow gaps (`do...while`, `switch`, `break`, `continue`), so the planned M9 widening scope is now subsumed by the shipped backend surface. |
| M10 — VM closure + function-expression parity | DONE | Evidence + Inference | `docs/vm-parity-backend.md`; `docs/project-status-and-architecture.md`; `src/compiler.c`; `tests/test_vm.c`; `tests/test_bytecode.c` | The current bounded VM/compiler surface already includes function expressions, closure capture, and upvalue lowering, so the planned M10 parity scope is now subsumed by the shipped backend surface. |
| L1 — Benchmark and profiling harness | DONE | Evidence | `docs/benchmark-and-profiling-harness.md`; `docs/milestone-verification-checklist.md`; `benchmarks/manifest.csv`; `benchmarks/baseline.csv`; `scripts/benchmark.sh`; `Makefile` | Benchmark fixtures, manifest, schema, baseline CSV, script, and Make targets are all checked in, and the checklist now contains an explicit L1 closure record. |
| L2 — Prototype chain and method dispatch | DONE | Evidence | `docs/semantics-baseline.md`; `README.md`; `src/eval.c`; `tests/test_eval.c` | Explicit prototype linkage and prototype-chain method dispatch are part of the current baseline. |
| L3 — `this` binding and constructor semantics | DONE | Evidence | `docs/semantics-baseline.md`; `README.md`; `tests/test_eval.c`; `tests/test_vm.c` | Bounded `this`, parenthesized `new`, and constructor behavior are current support. |
| L4 — Class syntax and desugaring/direct support | DONE | Evidence | `docs/semantics-baseline.md`; `docs/milestone-verification-checklist.md`; `examples/class_syntax.js`; `tests/test_vm.c` | Bounded class declarations are supported, and the checklist now includes a formal closure record. |
| L5 — Module system (`import` / `export`) | DONE | Evidence | `docs/module-system.md`; `docs/semantics-baseline.md`; `README.md`; `examples/modules.js`; `tests/test_eval.c`; `tests/test_parser.c`; `tests/test_lexer.c` | Bounded static named imports and declaration exports are supported on the tree-walk backend; VM module lowering remains explicitly deferred. |
| L6 — Embeddable host API | DONE | Evidence | `docs/embeddable-host-api.md`; `include/jsrt/runtime.h`; `src/runtime.c`; `tests/test_runtime.c`; `examples/embed_host.c` | A bounded source/file evaluation C API exists with documented lifecycle, result ownership, and diagnostics; advanced host callbacks/value handles remain deferred. |
| L7 — Async/event-loop experiment | DONE | Evidence | `docs/async-event-loop-experiment.md`; `docs/semantics-baseline.md`; `src/eval.c`; `tests/test_eval.c` | A bounded deferred-callback job queue experiment exists through `defer` / `runJobs`; promises, async/await, timers, and I/O remain deferred. |
| L8 — Regex + richer standard library | DONE | Evidence | `docs/regex-and-stdlib.md`; `docs/semantics-baseline.md`; `src/eval.c`; `tests/test_eval.c`; `README.md` | Bounded dependency-free `regexMatch` plus `strContains`/`strSlice` are supported; ECMAScript `RegExp` remains deferred. |
| L9 — Conformance subset harness | DONE | Evidence | `docs/conformance-subset-harness.md`; `conformance/manifest.csv`; `scripts/conformance.sh`; `Makefile` | A repo-local positive/negative conformance subset harness exists with tree/VM applicability; broad Test262 compatibility remains out of scope. |
| L10 — Optional JIT / advanced optimization feasibility study | DONE | Evidence | `docs/optimization-feasibility-study.md`; `.omx/plans/adr-javascript-runtime-l10-optimization-feasibility.md`; `docs/semantics-baseline.md` | Feasibility study closes L10 with a recommendation to defer native JIT and pursue bytecode peephole/constant-folding experiments first. |

## DONE milestone closure provenance

### DONE rows backed by explicit closure artifacts

These milestones have a repo-local artifact that directly records completion or formal handoff:

| Milestone | Explicit closure artifact | Notes |
|---|---|---|
| N7 — General heap GC architecture spike | `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md` | ADR says it is **accepted for roadmap handoff to N8**, which is the completion artifact for this design milestone. |
| N8 — General heap GC MVP | `docs/milestone-verification-checklist.md` | Contains the **N8 closure record — 2026-04-23** with retained managed-heap/stress/ASan evidence for the bounded N8 surface. |
| M2 — Aggregate semantics ADR (`copy` vs `reference`) | `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md` | ADR says it is **accepted for roadmap handoff to M3**, which is the completion artifact for this design milestone. |
| M3 — Mutable aggregate/reference model implementation | `docs/milestone-verification-checklist.md` | Contains the **M3 closure record — 2026-04-23** with retained reference-semantics/example/ASan evidence for the bounded M3 surface. |
| M6 — Bytecode / IR architecture spike | `.omx/plans/adr-javascript-runtime-m6-bytecode-ir-direction.md` | ADR says it is **accepted for roadmap handoff to M7/M8**, which is the completion artifact for this design milestone. |
| M7 — Bytecode compiler MVP | `docs/milestone-verification-checklist.md` | Contains the **M7 closure record — 2026-04-23** with retained compiler/disassembly/ASan evidence for the bounded M7 surface. |
| M8 — VM interpreter parity backend | `docs/milestone-verification-checklist.md` | Contains the **M8 closure record — 2026-04-23** with retained parity/backend-switch/ASan evidence for the original bounded M8 surface. |
| M1 — GC observability and stress rig | `docs/milestone-verification-checklist.md` | Contains the **M1 closure record — 2026-04-23** with retained debug-build/trace/stress evidence for the bounded M1 surface. |
| L1 — Benchmark and profiling harness | `docs/milestone-verification-checklist.md` | Contains the **L1 closure record — 2026-04-23** with retained harness/smoke/live-schema evidence for the bounded L1 workflow. |
| L4 — Class syntax and desugaring/direct support | `docs/milestone-verification-checklist.md` | Contains the **L4 closure record — 2026-04-23** with retained evidence and checked exit gates. |
| L5 — Module system (`import` / `export`) | `docs/milestone-verification-checklist.md` | Contains the **L5 closure record — 2026-05-15** with retained lexer/parser/evaluator/module-example evidence for the bounded tree-walk module surface. |
| L6 — Embeddable host API | `docs/milestone-verification-checklist.md` | Contains the **L6 closure record — 2026-05-15** with retained runtime API/source/file/module/diagnostic evidence. |
| L7 — Async/event-loop experiment | `docs/milestone-verification-checklist.md` | Contains the **L7 closure record — 2026-05-15** with retained deferred-callback/job-queue evidence. |
| L8 — Regex + richer standard library | `docs/milestone-verification-checklist.md` | Contains the **L8 closure record — 2026-05-15** with retained regex/string built-in evidence. |
| L9 — Conformance subset harness | `docs/milestone-verification-checklist.md` | Contains the **L9 closure record — 2026-05-15** with retained manifest/script/Make target evidence. |
| L10 — Optional JIT / advanced optimization feasibility study | `docs/milestone-verification-checklist.md` | Contains the **L10 closure record — 2026-05-15** with retained feasibility/ADR evidence. |

### DONE rows backed by current-state synthesis instead of a single closure artifact

These milestones are marked `DONE` because the current repo clearly ships their bounded surface, but there is no single dedicated closure record currently serving as the milestone’s canonical completion artifact:

`N1`, `N2`, `N3`, `N4`, `N5`, `N6`, `M4`, `M5`, `M9`, `M10`, `L2`, `L3`

For these rows, the ledger relies on the cited current-state docs, examples, tests, and implementation files rather than a standalone milestone-closure record.

## Stale / superseded source statements appendix

These entries track **source statements or framing** that are misleading relative to the current repo. They are not milestone-state rows.

| Source | Source statement / framing | Subject | Disposition | Current interpretation / correction | Basis |
|---|---|---|---|---|---|
| `docs/project-status-and-architecture.md` §11 | Lists **L1**, **L2**, and **L3** as “near-next execution targets,” while only L4 is labeled landed. | Milestone framing in the current-state overview | STALE | L1–L4 are all now evidenced in bounded repo form. Section 11 should point to this ledger and reserve “remaining work” wording for still-open milestones. | Evidence |
| `docs/aggregate-semantics-adr.md` | Says current runtime behavior is still copy-isolated and unchanged until M3 implements the migration. | M2/M3 aggregate semantics framing | SUPERSEDED | Current baseline now states arrays/objects preserve shared identity across environment flow and alias-visible mutation. | Evidence |
| `docs/bytecode-ir-architecture-spike.md` | Marks closures/upvalues, `switch`, `do...while`, and fuller parity as deferred beyond the first backend target. | M6-era backend subset framing | SUPERSEDED | Current compiler/VM docs and tests show those surfaces are already implemented in the bounded backend. | Evidence |
| `docs/bytecode-compiler-mvp.md` | Says “M7 is compiler-only” and “no VM execution yet.” | M7 guardrail framing | SUPERSEDED | The repo now includes an opt-in VM backend and parity tests, so this note is historical milestone framing, not the current state. | Evidence |
| `.omx/plans/prd-javascript-runtime-roadmap.md` M9/M10 entries | Treats VM surface widening and closure/function-expression parity as future backend milestones. | Historical roadmap sequencing | STALE | Current VM companion docs, architecture docs, and parity tests already include those bounded backend surfaces, so the roadmap wording is now historical rather than current-state. | Evidence + Inference |
| `.omx/plans/prd-javascript-runtime-roadmap.md` Phase 6 / long-horizon ordering | Treats **L2**, **L3**, and **L4** as a future OO lane after backend evolution. | Historical roadmap sequencing | STALE | The roadmap remains useful planning history, but current repo state already includes bounded L2/L3/L4 support. | Evidence + Inference |

## Maintenance note

When a future milestone closes:
1. update the milestone row in the ledger,
2. update the evidence paths if the authoritative source moves,
3. add or remove stale/superseded appendix rows only when a specific older source statement becomes misleading.

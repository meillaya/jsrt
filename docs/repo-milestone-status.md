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
| N8 — General heap GC MVP | DONE | Evidence | `docs/project-status-and-architecture.md`; `docs/n8-general-heap-gc-mvp-notes.md`; `src/gc.c`; `include/jsrt/gc.h`; `examples/gc_stress.js` | Current-state docs and runtime code explicitly describe and implement the bounded managed-heap path for `Env`, `JsFunction`, `JsArray`, and `JsObject`. |
| M1 — GC observability and stress rig | DONE | Evidence | `docs/gc-observability-and-stress.md`; `include/jsrt/gc.h`; `examples/gc_stress.js` | GC counters, trace mode, and stress coverage are documented and implemented. |
| M2 — Aggregate semantics ADR (`copy` vs `reference`) | DONE | Evidence | `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md`; `docs/aggregate-semantics-adr.md` | Design/ADR milestone: the decision artifact exists. Some companion wording is now stale; see appendix below. |
| M3 — Mutable aggregate/reference model implementation | DONE | Evidence | `docs/semantics-baseline.md`; `docs/project-status-and-architecture.md`; `examples/references.js`; `tests/test_eval.c` | Current baseline explicitly says arrays/objects preserve shared identity and alias-visible mutation. |
| M4 — Built-in library foundation | DONE | Evidence | `docs/built-in-library-foundation.md`; `README.md`; `src/eval.c`; `tests/test_eval.c` | `print`, `len`, `typeOf`, and `setPrototypeOf` exist as the bounded built-in surface. |
| M5 — Exceptionless structured runtime error model | DONE | Evidence | `docs/runtime-error-model.md`; `tests/test_diagnostics.c`; `src/eval.c` | Structured runtime error categories and source-based reporting are current behavior. |
| M6 — Bytecode / IR architecture spike | DONE | Evidence | `docs/bytecode-ir-architecture-spike.md`; `.omx/plans/adr-javascript-runtime-m6-bytecode-ir-direction.md` | Design/ADR milestone: backend direction artifact exists and handed off to M7/M8. |
| M7 — Bytecode compiler MVP | DONE | Evidence | `docs/bytecode-compiler-mvp.md`; `src/compiler.c`; `tests/test_bytecode.c` | Compiler and disassembler are present and documented. |
| M8 — VM interpreter parity backend | DONE | Evidence | `docs/vm-parity-backend.md`; `README.md`; `src/vm.c`; `tests/test_vm.c` | Opt-in VM backend exists and is documented as a bounded parity-tested surface. |
| M9 — VM surface widening I | DONE | Evidence + Inference | `docs/vm-parity-backend.md`; `docs/project-status-and-architecture.md`; `tests/test_vm.c` | The current bounded VM subset already includes aggregate operations plus the structured control-flow gaps (`do...while`, `switch`, `break`, `continue`), so the planned M9 widening scope is now subsumed by the shipped backend surface. |
| M10 — VM closure + function-expression parity | DONE | Evidence + Inference | `docs/vm-parity-backend.md`; `docs/project-status-and-architecture.md`; `src/compiler.c`; `tests/test_vm.c`; `tests/test_bytecode.c` | The current bounded VM/compiler surface already includes function expressions, closure capture, and upvalue lowering, so the planned M10 parity scope is now subsumed by the shipped backend surface. |
| L1 — Benchmark and profiling harness | DONE | Evidence | `docs/benchmark-and-profiling-harness.md`; `benchmarks/manifest.csv`; `benchmarks/baseline.csv`; `scripts/benchmark.sh`; `Makefile` | Benchmark fixtures, manifest, schema, baseline CSV, script, and Make targets are all checked in. |
| L2 — Prototype chain and method dispatch | DONE | Evidence | `docs/semantics-baseline.md`; `README.md`; `src/eval.c`; `tests/test_eval.c` | Explicit prototype linkage and prototype-chain method dispatch are part of the current baseline. |
| L3 — `this` binding and constructor semantics | DONE | Evidence | `docs/semantics-baseline.md`; `README.md`; `tests/test_eval.c`; `tests/test_vm.c` | Bounded `this`, parenthesized `new`, and constructor behavior are current support. |
| L4 — Class syntax and desugaring/direct support | DONE | Evidence | `docs/semantics-baseline.md`; `docs/milestone-verification-checklist.md`; `examples/class_syntax.js`; `tests/test_vm.c` | Bounded class declarations are supported, and the checklist now includes a formal closure record. |
| L5 — Module system (`import` / `export`) | REMAINING | Evidence | `.omx/plans/prd-javascript-runtime-roadmap.md`; `docs/semantics-baseline.md` | Modules/imports/exports remain explicitly unsupported. |
| L6 — Embeddable host API | REMAINING | Evidence | `.omx/plans/prd-javascript-runtime-roadmap.md`; `docs/project-status-and-architecture.md`; `docs/built-in-library-foundation.md` | Current docs still say there is no general-purpose embedding API. |
| L7 — Async/event-loop experiment | REMAINING | Evidence | `.omx/plans/prd-javascript-runtime-roadmap.md`; `docs/semantics-baseline.md`; `docs/project-status-and-architecture.md` | Async/event-loop behavior remains explicitly outside current scope. |
| L8 — Regex + richer standard library | REMAINING | Evidence | `.omx/plans/prd-javascript-runtime-roadmap.md`; `README.md`; `docs/built-in-library-foundation.md` | Current stdlib remains intentionally narrow and no regex support is claimed in current-state docs. |
| L9 — Conformance subset harness | REMAINING | Evidence | `.omx/plans/prd-javascript-runtime-roadmap.md`; `docs/project-status-and-architecture.md` | The repo still treats broad compatibility work as future-facing rather than providing a conformance harness today. |
| L10 — Optional JIT / advanced optimization feasibility study | REMAINING | Evidence | `.omx/plans/prd-javascript-runtime-roadmap.md`; `docs/project-status-and-architecture.md` | JIT/advanced optimization work remains explicitly outside current scope. |

## DONE milestone closure provenance

### DONE rows backed by explicit closure artifacts

These milestones have a repo-local artifact that directly records completion or formal handoff:

| Milestone | Explicit closure artifact | Notes |
|---|---|---|
| N7 — General heap GC architecture spike | `.omx/plans/adr-javascript-runtime-n7-general-heap-gc.md` | ADR says it is **accepted for roadmap handoff to N8**, which is the completion artifact for this design milestone. |
| M2 — Aggregate semantics ADR (`copy` vs `reference`) | `.omx/plans/adr-javascript-runtime-m2-aggregate-semantics.md` | ADR says it is **accepted for roadmap handoff to M3**, which is the completion artifact for this design milestone. |
| M6 — Bytecode / IR architecture spike | `.omx/plans/adr-javascript-runtime-m6-bytecode-ir-direction.md` | ADR says it is **accepted for roadmap handoff to M7/M8**, which is the completion artifact for this design milestone. |
| L4 — Class syntax and desugaring/direct support | `docs/milestone-verification-checklist.md` | Contains the **L4 closure record — 2026-04-23** with retained evidence and checked exit gates. |

### DONE rows backed by current-state synthesis instead of a single closure artifact

These milestones are marked `DONE` because the current repo clearly ships their bounded surface, but there is no single dedicated closure record currently serving as the milestone’s canonical completion artifact:

`N1`, `N2`, `N3`, `N4`, `N5`, `N6`, `N8`, `M1`, `M3`, `M4`, `M5`, `M7`, `M8`, `M9`, `M10`, `L1`, `L2`, `L3`

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

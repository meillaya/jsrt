# ADR — N7 General Heap GC Direction for jsrt

## Status
Accepted for roadmap handoff to N8.

## Decision
Adopt a **centralized managed-heap architecture as the target direction**, but implement N8 as a **bounded, incremental migration** that reuses the current tracing model's strongest properties.

In practice, that means:
- move toward one managed-heap model for **`Env`**, **`JsFunction`**, **`JsArray`**, and **`JsObject`**
- keep **AST**, **source buffers**, **tokens**, **diagnostic strings**, and runtime scaffolding outside GC scope for now
- preserve the current **copy-isolated aggregate semantics** during N8
- keep collection at explicit **safe points** only
- explicitly **defer allocation-triggered GC**, conservative stack scanning, and any broader shadow-stack/root-handle redesign beyond what N8 minimally needs

## Why this synthesis
The three completed N7 lanes converged on the same core need:
- the current runtime has a real tracing substrate for `Env` / `JsFunction`
- arrays/objects are already heap-allocated and traversable
- the current lifetime model is fragmented and the GC entry logic is duplicated

The difference between lanes was mostly about **target architecture vs migration style**:
- worker-1 and worker-2 favored a centralized managed heap as the target architecture
- worker-3 favored incremental broadening of the current tracer as the safest implementation style

The reconciled decision keeps both insights:
- **target:** centralized managed heap
- **implementation style:** incremental migration

## Evidence-backed current state
### Explicit/manual ownership still dominates
- runtime cleanup in `src/main.c` still manually frees final values, env, AST, tokens, source, and error buffers
- `docs/ownership-guardrails.md` already records that jsrt is still primarily an explicit-ownership runtime

### Current tracing scope is narrow
- `gc_collect` manages reachability for the env/function graph only
- `Env` and `JsFunction` each have dedicated registry/sweep behavior today

### Arrays/objects are good next candidates
- they are already heap-allocated
- they are already traversed during mark reachability
- they are still explicitly freed and deeply cloned across env boundaries today

### Root model is already explicit
- active lexical env scopes are tracked explicitly in evaluator state
- live transient results can already be passed explicitly as roots at safe points
- this strongly favors explicit root registration over conservative C-stack scanning

### Current technical smell to remove first
- `src/eval.c` has a local `collect_garbage` path that duplicates GC orchestration and diverges from `gc_collect`
- specifically, `gc_collect` calls `function_prepare_sweep_unreachable`, while the evaluator-local path does not

That makes canonicalizing GC entry/sweep orchestration the first concrete N8 task.

## Alternatives considered
### 1. Incremental broadening of the current tracer only
**Pros**
- smallest initial diff
- reuses current mark/sweep shape
- lowest short-term conceptual disruption

**Cons**
- risks cementing fragmented ownership rules
- encourages more bespoke registries/sweep paths
- weaker long-term foundation for N8/M1/M2/M6

### 2. Centralized managed heap immediately and broadly
**Pros**
- cleanest long-term architecture
- one collector path
- strongest conceptual foundation for later runtime work

**Cons**
- too much rewrite pressure for a single milestone
- risks dragging AST/source/tokens into scope unnecessarily
- raises rollback risk sharply

### 3. Defer broader GC entirely
**Pros**
- lowest immediate delivery risk
- preserves the current green baseline unchanged

**Cons**
- blocks or weakens the roadmap runtime-foundation gate
- leaves current lifetime fragmentation in place
- weakens the basis for M1 and later runtime ADRs

## Chosen interpretation of N8
N8 should be a **bounded managed-heap MVP** with these constraints:
1. manage only `Env`, `JsFunction`, `JsArray`, and `JsObject`
2. keep AST/source/tokens/diagnostics manual
3. preserve current aggregate copy semantics
4. use safe-point collection only
5. add explicit temporary-root support only as far as required for correctness

## Migration phases
### Phase 1 — Canonical GC entry path
- remove divergence between `gc_collect` and evaluator-local collection logic
- establish one canonical mark/prepare/sweep flow

### Phase 2 — Managed heap infrastructure
- introduce common heap metadata/registry for selected runtime objects
- do not widen to frontend-owned structures

### Phase 3 — Port env/function onto canonical heap path
- preserve current closure/env behavior
- keep verification anchored on existing passing semantics

### Phase 4 — Port arrays/objects
- move arrays/objects to the managed path
- keep deep-clone env-boundary behavior unchanged during N8

### Phase 5 — Observability handoff
- leave strings/manual buffers alone
- hand off to M1 for counters/stress/trace instrumentation

## Adoption criteria
- N8 remains bounded to selected runtime heap objects
- root traversal can be expressed from active envs plus explicit transient roots
- no aggregate semantic drift is introduced
- normal and ASan verification stay green

## Abort criteria
- if correctness requires allocation-triggered GC to succeed
- if temporary evaluator values cannot be rooted without invasive evaluator redesign
- if arrays/objects cannot be migrated without forcing the M2 semantic decision early
- if rollback boundaries become unclear or too wide

## Rollback path
If N8 becomes too risky:
- keep the current env/function cycle collector intact
- leave arrays/objects/strings explicitly owned
- revert any partially introduced managed paths for arrays/objects
- keep AST/source/tokens/diagnostics untouched

## Downstream impact
### Unlocks
- N8 bounded implementation
- M1 GC observability and stress rig
- M2 aggregate semantics ADR on a cleaner runtime footing
- M6 backend planning with clearer heap assumptions

### Does not force
- M3 reference-style aggregates
- string interning/shared identity
- allocation-triggered or fully general heap collection

## Directive for N8
Treat this ADR as the source of truth:
- **architecture target:** centralized managed heap
- **delivery style:** incremental migration
- **scope guard:** safe-point, bounded, no frontend ownership migration, no semantic drift

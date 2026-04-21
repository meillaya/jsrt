# PRD — Roadmap N7: General Heap GC Architecture Spike

## Goal
Design the next-stage memory-management architecture that can evolve the runtime from closure/environment cycle collection toward broader heap management, without committing prematurely to a risky implementation path.

## Why this milestone exists
The runtime currently has a narrow tracing collector for unreachable closure/environment cycles plus explicit ownership for many other structures. Future milestones—especially aggregate semantics changes, richer object models, and any VM/backend path—need a clear memory-management strategy before deeper runtime complexity lands.

## In Scope
- inventory current heap-managed and explicitly-owned runtime structures
- define candidate root sets and root-discovery strategy
- evaluate collector strategy options appropriate for this project stage
- define migration boundaries from current ownership model to a broader GC model
- define rollback/abort boundaries if the spike concludes a broader GC is not yet justified
- produce an ADR-quality recommendation that later implementation milestones can execute

## Out of Scope
- implementing the new collector
- broad refactors to value/env/object storage
- changing externally visible language semantics
- VM/compiler implementation
- performance optimization beyond design-level reasoning

## Core Questions to Resolve
1. What should be GC-managed in the next stage, and what should remain explicitly owned?
2. What are the roots of reachability in the current interpreter architecture?
3. What collector style best fits the repo's learning-first goals at this stage?
4. How can migration happen incrementally without destabilizing the green baseline?
5. What is the explicit no-go condition for deferring broader GC work?

## Required Option Set
At minimum, compare:
1. **Incremental broadening of current tracing approach**
2. **Centralized managed-heap model for selected runtime values/objects**
3. **Defer broader GC and keep explicit ownership longer**

Each option must include bounded pros/cons, migration impact, and expected verification burden.

## Deliverables
1. Heap/root inventory.
2. ADR covering collector options, decision drivers, chosen direction, and rejected alternatives.
3. Migration plan with phases, prerequisites, and rollback boundaries.
4. Explicit adoption criteria, abort criteria, and downstream milestones unlocked/cancelled.
5. Verification strategy for the later implementation milestone (N8).

## Acceptance Criteria
1. The spike produces a written ADR-level recommendation, not just notes.
2. Root sets and ownership boundaries are concrete enough to guide N8 implementation.
3. Adoption criteria, abort criteria, and rollback boundaries are explicit.
4. The plan states which future milestones depend on the chosen GC direction.
5. No implementation is performed as part of this milestone.

## Suggested Execution Mode
- Preferred: `team`
- Rationale: this is design-heavy, cross-cutting, and benefits from Architect + Critic + Verifier staging

## Suggested Lanes
- `architect` — root model, collector options, migration shape
- `critic` — challenge hidden complexity, rollback weakness, and overreach
- `verifier` — confirm the recommendation is grounded in the current repo design and later roadmap needs
- optional `researcher` — if comparing collector strategies beyond current repo-local patterns

## Verification Shape
- artifact review against current runtime structure (`value`, `env`, `gc`, evaluator ownership assumptions)
- explicit option comparison with rationale
- explicit downstream dependency/unlock map
- no code changes required unless a tiny proof artifact is needed for clarity

## Go / No-Go Gate
The output must explicitly state:
- **Adoption criteria:** what must be true to proceed into N8
- **Abort criteria:** what findings would mean “do not implement broader GC yet”
- **Rollback path:** how the project falls back to the current model if N8 proves too risky
- **Unlocks / cancels:** which later milestones are enabled, deferred, or invalidated

## Risks
- over-designing beyond the repo's current needs
- choosing a GC direction that conflicts with future aggregate semantics or VM work
- understating the migration complexity from explicit ownership to managed allocation

## Risk Mitigations
- keep the spike grounded in current code, not engine fantasies
- require a real option comparison, including “defer GC” as a valid outcome
- prefer phased migration over all-at-once redesign

## Dependencies
- N1 semantics/ownership baseline strongly recommended
- N2 diagnostics harness helpful but not required

## Exit Condition
There is a concrete, reviewable GC architecture decision package that makes N8 implementable—or clearly explains why N8 should be deferred.

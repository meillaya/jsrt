# PRD — Roadmap N1: Semantics Baseline and Ownership Guardrails

## Goal
Freeze the current runtime's externally visible semantics and internal ownership assumptions so later milestones can evolve safely without accidental regressions.

## Why this milestone exists
The project has already accumulated meaningful language/runtime behavior: closures, arrays/objects, property/index assignment, parser recovery, closure-cycle GC, and `for`/`break`/`continue`. Before deeper GC, value-model, or VM work, the repo needs a crisp baseline that says what behavior is intentional today.

## In Scope
- document current supported language behavior as a baseline matrix
- document current ownership/lifetime rules for tokens, AST, values, envs, closures, and GC-managed cleanup paths
- add/strengthen targeted regression coverage where current behavior is relied on but insufficiently locked
- define a milestone verification checklist reusable by later roadmap milestones
- update README or nearby docs only as needed to reflect the new baseline artifact set

## Out of Scope
- changing language semantics
- introducing new runtime features
- refactoring core evaluator/parser architecture beyond test/doc-driven cleanup
- broad GC redesign
- VM/backend work

## Deliverables
1. Baseline behavior matrix covering currently supported syntax/semantics.
2. Ownership/lifetime guardrail document for runtime structures.
3. Any narrow regression tests needed to lock baseline behavior.
4. Reusable verification checklist for future milestones.

## Acceptance Criteria
1. There is a committed baseline artifact describing the currently supported feature/behavior surface.
2. There is a committed ownership/lifetime artifact describing how key runtime structures are created, retained, and destroyed.
3. Existing behavior remains green with fresh verification.
4. Any behavior relied on by the baseline but not adequately protected is covered by tests.
5. `make clean && make test` passes.
6. ASan verification passes via the existing CFLAGS override path.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded, single-owner, documentation/test-heavy hardening pass

## Suggested Lanes
- `executor` — gather baseline, add/update artifacts/tests
- `verifier` — confirm artifact/test coverage and evidence
- `writer` — optional doc polish if needed

## Primary Risks
- documenting behavior that is accidental rather than intentional
- missing an important ownership edge case in env/value/closure cleanup
- over-expanding scope into redesign instead of baseline capture

## Risk Mitigations
- prefer evidence from current tests/examples/source over memory
- add only narrow regression tests where baseline claims need proof
- avoid semantic changes; if a contradiction is found, document it as an explicit follow-up

## Dependencies
- none beyond the current green repo baseline

## Exit Condition
The repo has a stable, evidence-backed semantics/ownership baseline that later roadmap milestones can cite instead of rediscovering from code.

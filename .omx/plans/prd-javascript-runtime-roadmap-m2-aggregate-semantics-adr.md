# PRD — Roadmap M2: Aggregate Semantics ADR (`copy` vs `reference`)

## Goal
Resolve whether arrays and objects should remain copy-isolated values or become shared-reference aggregates before the runtime grows further.

## Why this milestone exists
The repo now has:
- a frozen semantics/ownership baseline from N1
- a bounded managed heap from N8
- GC observability/stress evidence from M1

That means the project can finally decide whether today's copy-on-env-boundary aggregate behavior is a long-term teaching choice or a temporary simplification. Deferring that choice further would make M3, M4, and M6 less coherent.

## In Scope
- current-state evidence gathering from docs/tests/source
- option review of copy-isolated vs shared-reference aggregate semantics
- explicit compatibility matrix showing which user-visible behaviors would change
- adoption / abort / rollback conditions for the chosen path
- a clear statement of whether M3 is required, deferred, or cancelled
- README / roadmap-facing updates that point future execution at the decision artifact

## Out of Scope
- changing runtime code
- changing current semantics baseline directly
- introducing built-ins, VM/backend code, or broader GC changes
- redefining equality/coercion beyond what the ADR must explicitly defer

## Deliverables
1. Accepted ADR recording the aggregate-semantics decision.
2. Compatibility matrix mapping current behavior to the chosen direction.
3. Migration/no-migration plan with explicit adoption, abort, and rollback criteria.
4. Repo-facing documentation that points future work at the M2 decision.

## Acceptance Criteria
1. The repo contains an ADR that chooses either copy-isolated or shared-reference aggregate semantics.
2. The ADR explicitly cites current repo evidence instead of relying on recall.
3. The ADR records adoption criteria, abort criteria, and rollback boundaries.
4. The ADR states whether M3 is required, deferred, or cancelled.
5. README or companion docs point future work at the M2 artifact set.
6. Fresh verification confirms the baseline repo still passes after documentation changes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: design-heavy but still bounded enough for a single-owner gather/synthesize/verify pass

## Suggested Lanes
- `architect` — compare semantic directions and define the recommendation
- `critic` — challenge hidden coupling and rollback assumptions
- `verifier` — confirm repo evidence and milestone guardrails

## Primary Risks
- choosing semantics on intuition instead of repo-local evidence
- underestimating how much alias/reference behavior would ripple through tests/examples/docs
- letting the ADR silently absorb equality or broader JS-compatibility decisions that belong to later milestones

## Risk Mitigations
- anchor the ADR in current source/test/document references
- keep the chosen direction bounded to arrays/objects and env/value behavior
- defer unrelated semantics questions explicitly instead of smuggling them into M2

## Dependencies
- N1 semantics baseline and ownership guardrails
- M1 GC observability/stress evidence

## Exit Condition
Future implementation work can start M3 from a single, explicit decision artifact instead of rediscovering the aggregate model from source and tests.

# PRD — Roadmap M1: GC Observability and Stress Rig

## Goal
Make the post-N8 collector behavior inspectable and repeatably stress-testable so later runtime work can reason about GC activity with evidence instead of guesswork.

## Why this milestone exists
N8 introduced the bounded managed-heap MVP. Before moving into aggregate-semantics ADR work, the project needs visibility into what the collector is doing and fixtures that can repeatedly exercise closure/env/array/object GC behavior.

## In Scope
- lightweight GC counters and/or trace hooks for debug builds
- one or more repeatable GC stress fixtures focused on current supported semantics
- documentation for how to run and interpret the observability hooks
- regression coverage that proves observability features do not change runtime semantics

## Out of Scope
- changing collector policy
- allocation-triggered collection
- aggregate semantic changes
- VM/backend work
- performance optimization beyond observability and stress evidence

## Deliverables
1. GC observability surface (counters and/or trace output) suitable for debug/investigation use.
2. Stress fixtures covering closures, nested aggregates, loops, and repeated safe-point collection.
3. Verification docs / notes for how to run the observability and stress paths.
4. Fresh normal + ASan evidence.

## Acceptance Criteria
1. There is a documented way to observe GC activity in the repo.
2. There is at least one repeatable stress-style test/fixture that exercises the N8 collector meaningfully.
3. Existing visible semantics remain unchanged.
4. `make clean && make test` passes.
5. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded runtime-hardening milestone with a tight edit/verify loop

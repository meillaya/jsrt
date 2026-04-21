# Test Spec — Long-Horizon Roadmap Governance for `c_javascript_runtime`

## Goal
Define how future roadmap milestones should be verified before execution and completion, so Ralph or Team can adopt the roadmap without re-inventing milestone quality gates.

## Coverage
- every milestone must declare scope, dependencies, execution mode, lane ownership, and verification shape before implementation starts
- every milestone converted from the roadmap into execution must get its own milestone PRD + test-spec artifact pair
- memory-sensitive/runtime-sensitive milestones must include sanitizer evidence using:
  ```sh
  make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
  ```
- backend/performance milestones must include parity and/or benchmark evidence
- Team-executed milestones must include explicit staged verification ownership
- ADR/spike milestones must declare adoption criteria, abort criteria, rollback/exit conditions, and which downstream milestones they unlock or cancel
- post-VM milestones must declare whether they ship in both backends or temporarily remain tree-walk-only with an exit plan
- Ralph-executed milestones must include an explicit verifier checkpoint and milestone-completion evidence bundle

## Required per-milestone proof
- fresh test/build evidence
- at least one example or fixture for new visible behavior
- explicit regression coverage for changed semantics
- architect review for design-heavy milestones
- critic review for high-risk ADR-driven milestones

## Verification of this roadmap artifact
- roadmap contains at least 20 milestones
- milestones are grouped into near-term, mid-term, and long-term horizons
- roadmap includes execution guidance for both Ralph and Team
- roadmap includes ADR + RALPLAN-DR summary + available-agent-types roster + staffing guidance + launch hints + team verification path

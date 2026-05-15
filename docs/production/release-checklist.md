# Final Release Checklist and Deslop Record (D1-D4)

Status: active release checklist for the ultragoal run on 2026-05-15.

## D1 — Docs taxonomy cleanup

Current production-facing docs live under `docs/production/`. Historical learning/milestone docs remain in place but must not override current support claims. README links production docs first.

## D2 — README production-subset rewrite and examples audit

README now points to production subset, support tiers, verification targets, VM parity, module/host ADRs, and public C API guide. Examples remain covered through unit tests, conformance, and VM parity manifest where applicable.

## D3 — Public C API guide

`docs/production/public-c-api-guide.md` documents lifecycle, ownership, errors, version checks, and limits.

## D4 — Final gate checklist

Before marking the aggregate Codex goal complete, run:

- `make clean && make verify-production`
- sanitizer lane for touched C/runtime code
- ai-slop-cleaner on changed files
- post-cleaner verification
- code-review with APPROVE/CLEAR result

Known final-gate status: not final until the active ultragoal ledger has all stories checkpointed and the mandatory cleanup/review gate is clean.

## Final gate evidence — 2026-05-15

AI slop cleaner scoped pass:

- Behavior lock: `make clean && make verify-production`, ASan `make test`, `make bench-smoke`, and `bash -n` for shell scripts.
- Cleanup plan: inspect changed production docs/scripts/runtime API changes; remove masking fallback/slop if found; keep changes scoped; rerun verification.
- Fallback findings: documentation-only mentions of forbidden silent fallback; classified as intentional policy text, not masking fallback code. No workaround/bypass branches found in changed scripts/runtime API scope.
- Passes completed: no code deletion needed; shell scripts passed syntax and production verification; no new dependencies introduced.

Code review synthesis:

- Recommendation: APPROVE
- Architectural status: CLEAR
- Evidence: production support claims are centralized under `docs/production/`; VM/module/async/regex unsupported surfaces are explicit gates instead of silent compatibility claims; new Make targets are deterministic wrappers over checked-in manifests; C API version addition is source-level and tested.
- Residual risks: many advanced ECMAScript surfaces remain explicitly Future rather than implemented; this is intentional for the bounded production subset and tracked in the support-tier matrix/ADRs.

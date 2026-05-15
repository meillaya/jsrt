# Conformance and Quality Gates (C1-C2, Q1-Q3)

Status: accepted on 2026-05-15.

## C1 — Per-feature conformance gate protocol

Every new Supported feature needs:

1. support-tier matrix update,
2. unit tests for parser/evaluator/compiler/VM as applicable,
3. conformance positive and/or negative fixture where observable from CLI,
4. VM parity case if the feature is VM-supported,
5. deterministic unsupported diagnostic case if the feature is not VM-supported,
6. closure evidence with commands and counts.

## C2 — Test262 subset expansion by feature

The curated Test262 subset expands by feature, not by wholesale import. `conformance/test262-subset-manifest.csv` tracks tier, milestone, backend, expectation, skip, and xfail rationale. Future imports must preserve source attribution in the `source` column and avoid broad pass-rate claims.

## Q1 — Sanitizer gates

Ownership-sensitive milestones must run the sanitizer lane documented in `docs/production/verification-targets.md`. Current evidence is recorded in milestone closures; future CI can promote it to an automated target.

## Q2 — Fuzzing gates

Fuzzing is currently deterministic/fuzz-like, not coverage-guided infrastructure. Future lexer/parser/regex fuzz fixtures should be source-controlled, bounded, and reproducible without new dependencies unless an ADR accepts them.

## Q3 — Performance / GC / memory / security regressions

Performance and memory-sensitive milestones should use the benchmark harness, GC churn fixtures, sanitizer runs, and negative security-style diagnostics. `make bench-smoke` is the minimum smoke target when performance behavior changes.

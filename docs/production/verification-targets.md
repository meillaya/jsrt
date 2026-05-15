# Verification Target Consolidation (P4)

Status: accepted on 2026-05-15.

This document defines the verification evidence vocabulary for production-subset milestones.

## Required common targets

| Target | Command | Required when |
|---|---|---|
| Unit suite | `make test` | every milestone checkpoint |
| Conformance subset | `make conformance` | support claim, parser/evaluator/VM behavior, diagnostics, modules, async, regex, host-visible behavior |
| Sanitizer lane | `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'` | memory ownership, GC, host API, handles, native callbacks, regex engine, async queues, VM lowering |
| Benchmark smoke | `make bench-smoke` | performance/GC/memory regression gates |
| Per-feature gate | documented command or manifest filter | each feature moving to Supported |

## Evidence convention

Milestone closure records must include:

- command(s) run
- pass/fail result and assertion/case count when available
- files or docs that encode the support contract
- known unsupported/future surfaces
- sanitizer status when ownership-sensitive code changed

## Failure policy

A failed required target blocks the milestone unless a checked-in deferral/feasibility artifact explicitly narrows the milestone to non-implementation work.

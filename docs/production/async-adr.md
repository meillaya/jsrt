# Jobs, Promises, and Async/Await ADR (A1-A8)

Status: accepted on 2026-05-15.

## A1 — Jobs / microtask model

The production model keeps host-controllable jobs explicit. The existing `defer(callback)` / `runJobs()` queue is the semantic seed, but it is Experimental until a host pump and Promise reactions are specified. Jobs run only at explicit drain points; there is no hidden browser/Node event loop.

## A2 — Host-controllable queue

Current behavior remains explicit queueing through `defer` and explicit draining through `runJobs`. A future host API may expose a pump, but must preserve deterministic tests and GC rooting for queued callbacks.

## A3-A5 — Promise model, API subset, and rejection diagnostics

Promises are Future. The accepted bounded design is:

- states: pending, fulfilled, rejected
- reactions queued as jobs, not executed synchronously from `then`
- minimal API: `Promise.resolve`, `Promise.reject`, `then`
- rejection diagnostics deterministic and documented before support claims

No Promise syntax/API is currently Supported.

## A6 — Async function / await lowering

Async/await is Future and requires a separate lowering ADR before parser/evaluator or VM implementation. The preferred direction is syntax lowering onto the Promise/job model once A3-A5 are green.

## A7-A8 — Tree and VM implementation gates

Tree-walk async/await and VM async lowering are explicitly deferred. A future implementation must add parser tests, evaluator tests, VM parity or deterministic VM rejection, Test262 subset cases, and sanitizer evidence for queue/GC ownership.

## Current retained evidence

- `docs/async-event-loop-experiment.md`
- `tests/test_eval.c` coverage for `defer` / `runJobs`
- support tier matrix marks callback queue Experimental and Promises/async Future

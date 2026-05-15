# L7 Async / Event Loop Experiment

This document records the bounded **L7 — Async / event-loop experiment**.

## Supported surface

L7 intentionally does not implement JavaScript promises or `async` / `await`. Instead it adds a tiny explicit job queue through built-ins:

```js
defer(function () {
  // zero-argument callback body
});

runJobs();
```

- `defer(callback)` enqueues a function callback and returns `null`.
- `runJobs()` drains the queue FIFO and returns `null`.
- Callbacks run synchronously when `runJobs()` is called.
- Callbacks are normal jsrt functions/closures and may mutate captured state.
- Jobs enqueued while `runJobs()` is draining are also run before `runJobs()` returns.

## Explicitly unsupported

- `async` / `await` syntax
- promises and microtasks
- timers
- I/O events
- host event-loop integration
- concurrency or preemption

## Rationale

This experiment proves the runtime can hold deferred callbacks safely across evaluator safe points and execute them later through ordinary function invocation, without committing the project to full JavaScript async semantics.

## Verification evidence

- `tests/test_eval.c` covers FIFO deferred closure execution and `defer()` type errors.
- `docs/semantics-baseline.md` records the bounded built-in surface and unsupported async features.

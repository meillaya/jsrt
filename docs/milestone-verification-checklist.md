# jsrt Milestone Verification Checklist

Use this checklist when executing future roadmap milestones via Ralph or Team.

## Pre-implementation

- [ ] milestone PRD exists
- [ ] milestone test-spec exists
- [ ] baseline impact is understood (`docs/semantics-baseline.md`)
- [ ] ownership/runtime impact is understood (`docs/ownership-guardrails.md`)

## During implementation

- [ ] changed semantics are covered by narrow regression tests
- [ ] at least one example or fixture demonstrates new visible behavior
- [ ] docs are updated if the milestone changes the visible/runtime baseline

## Required fresh evidence

- [ ] `make clean && make test`
- [ ] relevant example runs succeed
- [ ] diagnostics or parser recovery changes have targeted failing-case tests

## Memory-sensitive/runtime-sensitive milestones

- [ ] ASan verification run:

```sh
make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'
```

## L2 prototype-chain / method-dispatch exit gates

Use this addendum when verifying the bounded L2 milestone. L2 is done only when the new lookup model lands **without** leaking L3 semantics.

- [ ] prototype linkage is explicit and bounded (for example `setPrototypeOf(obj, protoOrNull)`), with negative coverage for invalid receiver/prototype values, self-linking, and cycles
- [ ] property reads are verified as own-first, then prototype-chain lookup, including multi-hop lookup and full-chain miss failure/message coverage
- [ ] direct writes remain receiver-only; shadowing creates/updates own properties instead of mutating prototype-owned entries
- [ ] inherited intermediate-container assignment is covered explicitly: inherited container reads may resolve through the prototype chain, but the final write mutates only the resolved container and does not invent an own receiver-local container
- [ ] method dispatch is still plain-function invocation after prototype-aware lookup; no implicit `this`, constructor behavior, `new`, or class semantics are introduced
- [ ] `len(obj)`, object equality, and object stringification are explicitly verified to remain own-only in L2
- [ ] evaluator and VM suites cover the same supported positive and negative cases before milestone closure
- [ ] `docs/semantics-baseline.md` is updated to describe the bounded L2 surface, while keeping `this` / `new` / constructors / classes deferred to L3
- [ ] the ASan lane is re-run after the final object-graph changes land, and the exact command/result are captured in the completion evidence bundle

## L3 `this` binding / constructor exit gates

Use this addendum when verifying the bounded L3 milestone. L3 is done only when receiver binding and constructor semantics land **without** bleeding into L4 class syntax or general function-object semantics.

- [ ] `this` and bounded parenthesized `new` syntax are parser-covered, including recovery/diagnostics for malformed or out-of-scope `new` forms
- [ ] bare/top-level/plain/extracted calls keep `this = null`, while `obj.method()` and `obj["method"]()` bind `this` to the concrete receiver, including prototype-inherited methods
- [ ] `new Ctor(args)` allocates a fresh instance, uses `Ctor.prototype` / `Ctor["prototype"]` when that slot is an object, and preserves the default instance when the constructor returns a primitive or `null`
- [ ] non-constructable targets, native functions, invalid constructor-`prototype` writes, and non-`prototype` function property access/assignment fail with targeted runtime type diagnostics
- [ ] evaluator and VM suites cover the same supported positive/negative cases, including evaluation-order guards and bytecode evidence for plain, method, and constructor call lowering
- [ ] at least one example demonstrates constructor + prototype-method interaction, and the docs diff keeps L4 classes / broader function-object semantics explicitly deferred
- [ ] the ASan lane is re-run after the final function/object-graph changes land, and the exact command/result are captured in the completion evidence bundle

## Design-heavy milestones

- [ ] architect review completed
- [ ] critic review completed when the milestone is ADR/spike-heavy or high risk
- [ ] adoption/abort/rollback conditions are recorded for speculative architecture work

## Completion bundle

- [ ] changed files listed
- [ ] simplifications made listed
- [ ] remaining risks listed
- [ ] milestone-specific follow-ups recorded explicitly

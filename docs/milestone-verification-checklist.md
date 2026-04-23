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

## L3 `this` / constructor / `new` exit gates

Use this addendum when verifying the bounded L3 milestone.

- [ ] bare/top-level `this` and plain/extracted calls are explicitly verified as `null`
- [ ] `obj.method()` and `obj["method"]()` bind `this` to the concrete receiver, including prototype-inherited methods
- [ ] `new Ctor(args)` is verified only in the approved parenthesized L3 form; bare `new Ctor` and broader postfix/call chaining stay rejected
- [ ] user functions expose only the narrow `prototype` meta-property; non-`prototype` function property reads/writes remain rejected with type diagnostics
- [ ] native-function constructor policy is locked: `builtin.prototype` reads as `null`, writes fail, and `new builtin()` fails
- [ ] bytecode/disassembly evidence covers `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW`
- [ ] docs/examples show constructor + prototype-method interaction while keeping the broader class surface deferred to L4

## L4 bounded class-declaration exit gates

Use this addendum when verifying the bounded L4 milestone.

- [ ] `class Name { ... }` declarations are verified across parser, evaluator, compiler, and VM
- [ ] supported class behavior is locked: optional constructor, identifier-named instance methods, default constructor behavior, prototype-installed methods, and extracted-method `this = null`
- [ ] `Name()` without `new` is rejected on both backends with stable runtime type diagnostics
- [ ] `Name.prototype` / `Name["prototype"]` remain readable and writable under the existing object-or-null rules
- [ ] unsupported syntax (`extends`, `super`, static members, fields, getters/setters, computed names, class expressions) stays parser-owned with stable diagnostics
- [ ] bytecode/disassembly proof still routes lowered classes through `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW` with no new class-specific invocation opcode
- [ ] docs/examples describe the bounded L4 class surface while keeping inheritance and the broader ES class feature set deferred

## L4 class syntax / desugaring exit gates

Use this addendum when verifying the bounded L4 milestone.

- [ ] bounded `class Name { ... }` declarations are covered on both evaluator and VM backends
- [ ] class bindings are construct-only, while plain calls fail with stable type diagnostics
- [ ] omitted constructors, prototype-installed methods, extracted-method `this = null`, and `Name.prototype` read/write parity are all explicitly verified
- [ ] unsupported `extends`, `super`, static members, fields, accessors, computed names, and class expressions remain parser-owned rejections with source diagnostics
- [ ] docs/examples describe the bounded class surface and the deliberate no-TDZ sequential declaration model
- [ ] bytecode/disassembly evidence shows class lowering still routes through `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW`

## Design-heavy milestones

- [ ] architect review completed
- [ ] critic review completed when the milestone is ADR/spike-heavy or high risk
- [ ] adoption/abort/rollback conditions are recorded for speculative architecture work

## Completion bundle

- [ ] changed files listed
- [ ] simplifications made listed
- [ ] remaining risks listed
- [ ] milestone-specific follow-ups recorded explicitly

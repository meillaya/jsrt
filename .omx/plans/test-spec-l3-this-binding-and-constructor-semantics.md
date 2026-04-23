# Test Spec — L3 `this` binding and constructor semantics

## Goal
Define the minimum test matrix required to close L3 across lexer, parser, evaluator, VM, docs, and example evidence.

## Positive behavior matrix
1. `this` resolves to the receiver for direct method calls.
2. Prototype-inherited method calls still bind `this` to the concrete receiver, not the prototype owner.
3. Index-based method calls (`obj["m"]()`) bind `this` to the receiver.
4. Extracted method call binds `this` to `null`.
5. Plain function call binds `this` to `null`.
6. `new Ctor()` allocates a fresh object and constructor assignments land on that object.
7. `Ctor.prototype = proto` causes `new Ctor()` instances to inherit from `proto`.
8. Constructor explicit return of object/array/function replaces the default instance.
9. Constructor explicit return of number/bool/string/null preserves the default instance.

## Negative behavior matrix
1. `new 1` or equivalent non-function target raises a runtime type diagnostic.
2. `Ctor.prototype = 1` raises a runtime type diagnostic.
3. Unsupported arbitrary function property access remains rejected unless the implementation deliberately broadens scope and updates the PRD/ADR first.
4. Parser recovery for malformed `new` expressions produces source diagnostics with caret markers.

## Backend parity requirements
- Every positive/negative evaluator test has a matching VM test unless the case is parser-only or lexer-only.
- Constructor return truth-table coverage must use the same source programs in both `tests/test_eval.c` and `tests/test_vm.c` where practical.

## Required fresh evidence
1. `make clean && make test`
2. ASan lane from `docs/milestone-verification-checklist.md`
3. at least one example demonstrating constructor + prototype method interaction
4. docs diff confirming L3 support and L4 deferral remain explicit

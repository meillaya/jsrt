# Test Spec — L3 `this` binding and constructor semantics

## Goal
Define the minimum test matrix required to close L3 across lexer, parser, evaluator, VM, docs, and example evidence.

## Positive behavior matrix
1. Bare/top-level `this` resolves to `null`.
2. `this` resolves to the receiver for direct method calls.
3. Prototype-inherited method calls still bind `this` to the concrete receiver, not the prototype owner.
4. Index-based method calls (`obj["m"]()`) bind `this` to the receiver and match dot-call behavior.
5. Extracted method call binds `this` to `null`.
6. Plain function call binds `this` to `null`.
7. `new Ctor()` allocates a fresh object and constructor assignments land on that object.
8. `Ctor.prototype = proto` and `Ctor["prototype"] = proto` cause `new Ctor()` instances to inherit from `proto`.
9. Reading `Ctor.prototype` / `Ctor["prototype"]` reflects the same slot used by `new`.
10. Constructor explicit return of object/array/function replaces the default instance.
11. Constructor explicit return of number/bool/string/null preserves the default instance.
12. Receiver/base, property/index expression, and arguments are each evaluated exactly once and in left-to-right order for method and constructor calls.

## Negative behavior matrix
1. `new 1` or equivalent non-function target raises `RUNTIME_ERROR_TYPE`.
2. `Ctor.prototype = 1` and `Ctor["prototype"] = 1` raise `RUNTIME_ERROR_TYPE`.
3. `new` on a native function raises `RUNTIME_ERROR_TYPE` because native functions are not constructors in L3.
4. `builtin.prototype` / `builtin["prototype"]` read as `null`, and writes to that slot fail with `RUNTIME_ERROR_TYPE`.
5. Non-`prototype` function property access/assignment (`fn.x`, `fn["x"]`, `fn.x = 1`, `fn["x"] = 1`) fails with `RUNTIME_ERROR_TYPE` on both backends; non-string bracket keys on functions also fail under that family.
6. Bare `new Ctor` without parentheses is rejected in L3.
7. Any broader out-of-scope `new` callee shapes such as `new foo()(1)` are rejected rather than silently widening the surface.
8. Parser recovery for malformed `new` expressions produces source diagnostics with caret markers.

## Backend parity requirements
- Every positive/negative evaluator test has a matching VM test unless the case is parser-only or lexer-only. Parser-owned unsupported-`new` rejections are covered in `tests/test_parser.c` + `tests/test_diagnostics.c`; runtime type failures are covered in `tests/test_eval.c` + `tests/test_vm.c`.
- Constructor return truth-table coverage must use the same source programs in both `tests/test_eval.c` and `tests/test_vm.c` where practical.
- Evaluation-order guards should use the same source programs in both backends where practical.

## Required fresh evidence
1. `make clean && make test`
2. ASan lane from `docs/milestone-verification-checklist.md`
3. bytecode/disassembly evidence for `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW` (or equivalently named chosen opcodes) in `tests/test_bytecode.c` and `src/bytecode.c`
4. diagnostics coverage in `tests/test_diagnostics.c` for malformed `new` parser/runtime diagnostics as applicable
5. at least one example demonstrating constructor + prototype method interaction
6. docs diff confirming L3 support and L4 deferral remain explicit

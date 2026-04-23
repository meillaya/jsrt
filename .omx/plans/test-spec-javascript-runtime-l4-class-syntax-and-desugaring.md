# Test Spec — JavaScript Runtime L4 Class Syntax and Desugaring

## Goal
Define the minimum evidence needed to close the bounded first L4 class milestone on top of the landed L3 constructor/prototype runtime.

## Positive behavior matrix
1. `class Name {}` parses and defines a constructable class binding.
2. `class Box { constructor(v) { this.value = v; } get() { return this.value; } }` constructs and dispatches methods correctly.
3. Omitted-constructor classes still instantiate with a default empty constructor behavior.
4. Instance methods live on `Name.prototype`, not on each instance.
5. Instance method calls bind `this` exactly like current L3 prototype methods.
6. Extracted class methods still fall back to L3 `this = null`.
7. `Name.prototype` / `Name["prototype"]` remain readable and writable under the existing L3 object-or-null rules.
8. Class declarations follow the chosen no-TDZ/sequential declaration model consistently across evaluator and VM.

## Negative behavior matrix
1. `Name()` without `new` raises `RUNTIME_ERROR_TYPE` on both backends.
2. Duplicate `constructor` definitions are rejected in parser/diagnostic coverage.
3. `extends` is rejected in parser/diagnostic coverage.
4. `super` usage is rejected in parser/diagnostic coverage.
5. `static foo() {}` is rejected in parser/diagnostic coverage.
6. Field syntax is rejected in parser/diagnostic coverage.
7. `get foo() {}` and `set foo(v) {}` accessor syntax is rejected, while ordinary method names like `get() {}` remain valid.
8. Computed method names are rejected.
9. Class expressions are rejected.
10. Malformed class members produce stable source diagnostics with caret markers.

## Backend parity requirements
- Every supported evaluator class behavior has a matching VM test unless the case is strictly parser-owned.
- The chosen declaration model (`use-before-declaration`, block-local visibility, redeclaration behavior) is parity-tested across evaluator and VM.
- Construct-only class-call rejection is parity-tested across evaluator and VM.
- Bytecode evidence must prove class lowering still routes through `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW`, with no new class-specific invocation opcode.

## Required fresh evidence
1. `make clean && make test`
2. `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'`
3. lexer/parser/eval/vm/bytecode/diagnostics coverage for the bounded L4 matrix
4. bytecode/disassembly proof for lowered class forms using the existing L3 call family
5. at least one class example on the default backend
6. the same example on the VM backend if inside the supported VM subset
7. docs diff confirming bounded class support while keeping `extends` / `super` deferred

## Milestone-specific checklist additions
- verify class constructor `prototype` behavior stays aligned with current L3 rules
- verify unsupported class syntax is parser-owned and source-diagnostic-stable
- verify no-TDZ declaration behavior is explicit in both tests and docs

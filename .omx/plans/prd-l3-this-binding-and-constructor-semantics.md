# PRD — L3 `this` binding and constructor semantics

## Scope
Roadmap milestone L3: add `this` resolution, constructor calls, `new` semantics, and matching diagnostics/tests on top of the landed L2 prototype model without pulling in L4 class syntax.

## Evidence baseline
- `.omx/plans/prd-javascript-runtime-roadmap.md:270-278` defines L3 as `this` resolution, constructor calls, `new` semantics, diagnostics/tests.
- `docs/project-status-and-architecture.md:389-391` says L3 is meaningful only after L2 prototype/method dispatch exists.
- `docs/semantics-baseline.md:71-77` records L2’s bounded behavior: explicit prototype linkage, prototype-chain property lookup, and `obj.method()` still acting like a plain call with no implicit `this`.
- `src/eval.c:845-930`, `src/compiler.c:497-507`, and `src/vm.c:724-740` show call paths that currently pass only callee + args, with no receiver / `this` slot.
- `include/jsrt/token.h`, `src/parser.c`, and `src/lexer.c` contain no `this` or `new` token/syntax support.
- `src/eval.c:1181-1197` and `src/vm.c:624-673` show property/index lookup applies only to arrays/objects today; function values are not yet property-bearing constructor objects.

## 1) Requirements Summary
1. Add surface syntax for `this` and `new` while keeping L3 distinct from L4 classes/desugaring.
2. Define `this` binding for method calls using the existing L2 receiver/prototype lookup model.
3. Define constructor invocation semantics for `new` using the existing object/prototype system.
4. Preserve ordinary function-call behavior as a separate path from method/constructor calls.
5. Add diagnostics for invalid `new` targets and any bounded unsupported `prototype` interactions introduced by the chosen model.
6. Keep evaluator and VM semantics aligned; both backends must pass equivalent positive/negative cases.
7. Avoid new dependencies and reuse current AST / compiler / VM infrastructure patterns.

## Explicit semantic decisions
### `this` binding
- `obj.method()` and `obj["method"]()` bind `this` to the evaluated receiver object (`obj`), even when the function is found through the prototype chain.
- Binding uses the original receiver, not the prototype owner where the function was found.
- Extracted/bare calls (`let f = obj.method; f()`) bind `this` to `null`.
- Calls that are not property/index calls remain ordinary function calls with `this = null`.

### `new`
- `new` is **syntax-only at the surface in L3**: introduce a keyword/operator, not a reflective builtin alternative.
- Supported L3 surface: `new <postfix-callee>(args...)`, where the callee may be an identifier or member/index expression that resolves to a function.
- `new` evaluates the callee and arguments once, allocates a fresh object instance, sets its prototype from the constructor’s prototype slot, then invokes the constructor with `this` bound to that fresh instance.
- Instance prototype rule: use `Ctor.prototype` when it is an object; otherwise fall back to `null`.

### Constructor return behavior
- If the constructor returns an object, array, or function value explicitly, that value becomes the result of `new`.
- If the constructor returns `null`, number, bool, or string, the explicit return is ignored and the fresh instance is returned.
- This keeps L3 predictable while staying compatible with future L4 class lowering, where constructors conceptually initialize an instance rather than replace it with primitives.

### Function `prototype` surface
- L3 should add a **narrow function prototype slot**, not full “functions are generic objects” semantics.
- Supported L3 function-property behavior is limited to the `prototype` property needed for constructor behavior.
- Reading `Ctor.prototype` returns the constructor’s prototype object.
- Assigning `Ctor.prototype = value` accepts an object or `null`; other values yield a runtime type diagnostic.
- Arbitrary function property bags remain out of scope for L3 and stay deferred unless execution discovers that the narrower slot model is unworkable.

## 2) Acceptance Criteria
1. Lexer/parser recognize `this` and `new` and build explicit AST forms for them.
2. `this` inside a method invoked through property/index call resolves to the receiver object, including prototype-inherited methods.
3. Plain function calls and extracted methods continue to execute, but with `this = null`.
4. `new Ctor(args)` allocates a fresh object, binds `this` to it, and applies constructor-side writes to that instance.
5. `Ctor.prototype` controls the prototype assigned to instances created via `new` when the slot is an object; `null` yields no prototype.
6. Returning an aggregate/function from a constructor replaces the default instance; returning a primitive or `null` does not.
7. `new` against a non-function value fails with a targeted runtime diagnostic.
8. Invalid constructor prototype assignment fails with a targeted runtime diagnostic.
9. Evaluator and VM backends cover the same supported positive/negative cases.
10. Docs update the semantics baseline to move `this` / `new` from deferred to supported, while keeping classes deferred to L4.

## 3) Implementation Steps
1. **Syntax/AST planning slice**
   - Add `TOKEN_THIS` and `TOKEN_NEW`.
   - Extend AST with explicit `EXPR_THIS` and `EXPR_NEW` (or equivalent bounded form) rather than encoding `new` as a normal call.
   - Define parser precedence so postfix member/index/call chaining still works and `new` stays a dedicated L3 form.
2. **Call-shape refactor slice**
   - Introduce an internal call descriptor shared by evaluator and compiler/VM planning: callee value, optional receiver/`this`, argument list, and constructor mode.
   - Distinguish plain call vs property/index call vs constructor call at evaluation/compile time.
3. **Runtime object-model slice**
   - Extend `JsFunction` with a dedicated constructor prototype slot plus any GC marking/freeing support required.
   - Initialize user functions with a default empty prototype object; native functions may either expose `null` prototype or opt out of construction explicitly.
   - Add bounded function `prototype` read/write support needed by `new`.
4. **Evaluator semantics slice**
   - Thread `this` into ordinary/method/constructor call execution.
   - Resolve property/index calls to `(callee, receiver)` without double-evaluating the receiver.
   - Implement constructor result selection and diagnostics.
5. **Compiler/VM parity slice**
   - Add bytecode support for receiver-aware call/new behavior (either new opcodes or a call opcode family with explicit receiver/constructor metadata).
   - Keep stack discipline explicit so GC/temp-root safety remains understandable.
6. **Docs/tests/verification slice**
   - Add lexer/parser/eval/vm regression coverage.
   - Update `docs/semantics-baseline.md` and `docs/milestone-verification-checklist.md` with L3 gates.
   - Add or extend an example showing constructor + prototype-method interaction.

## 4) Risks and Mitigations
- **Risk: function values are not object-like today.**
  - Mitigation: keep L3 narrow by adding only a dedicated `prototype` slot instead of full function objects.
- **Risk: receiver evaluation could run twice for `obj.expr().method()`-style shapes if call lowering is naive.**
  - Mitigation: require a call descriptor / temporary stack form that evaluates receiver once, then reuses it for both lookup and binding.
- **Risk: evaluator and VM diverge on constructor return rules.**
  - Mitigation: codify shared truth-table tests across `tests/test_eval.c` and `tests/test_vm.c` before closing the milestone.
- **Risk: `new` parsing can accidentally creep toward L4 class/general operator complexity.**
  - Mitigation: bound L3 to constructor invocation only; no class syntax, `super`, or desugaring work.
- **Risk: GC omissions for new function prototype references introduce leaks or premature frees.**
  - Mitigation: include ASan and GC-sensitive regression runs in milestone verification.

## 5) Verification Steps
1. Add lexer tests for `this` / `new` keyword tokenization.
2. Add parser tests proving explicit `this` and `new` AST forms and relevant recovery diagnostics.
3. Add evaluator tests for:
   - receiver-bound prototype methods
   - extracted method loses receiver (`this = null`)
   - constructor field initialization
   - constructor prototype wiring
   - explicit aggregate-vs-primitive return behavior
   - invalid `new` target
   - invalid `Ctor.prototype = <non-object-non-null>` assignment
4. Mirror the same cases in VM tests.
5. Run `make clean && make test`.
6. Run the ASan lane from `docs/milestone-verification-checklist.md` because function/object graph reachability changes.
7. Run at least one example program demonstrating constructor + prototype method behavior.

## 6) RALPLAN-DR summary
### Principles
1. Reuse the landed L2 prototype model instead of inventing a parallel OO mechanism.
2. Keep L3 semantically bounded: support receiver binding and constructors, not classes.
3. Preserve evaluator/VM parity with the same observable behavior and diagnostics.
4. Prefer narrow runtime model changes over broad object-model generalization.
5. Make new semantics test-first where backend drift is likely.

### Top 3 drivers
1. **L2 dependency reuse:** L3 only makes sense now that prototype lookup exists.
2. **Scope control:** maintain clear separation from L4 classes/desugaring.
3. **Backend correctness:** existing call paths in eval/compiler/vm have no receiver slot, so the plan must make `this` semantics explicit in both runtimes.

### Viable options
#### Option A — Narrow constructor slot on functions (**recommended**)
- Add dedicated `this`/`new` syntax and runtime call metadata.
- Add only a special `prototype` slot on functions for constructor semantics.
- Pros: smallest semantic expansion; preserves L4 separation; lower risk than making functions general objects.
- Cons: introduces a special case for function property handling.

#### Option B — Generalize functions to behave like objects now
- Make function values participate in generic property access/assignment, with `prototype` just another property.
- Pros: more JS-like long-term surface; fewer function-specific property exceptions later.
- Cons: substantially larger runtime/object-model change; blurs L3 vs future milestones; expands GC and property semantics risk.

#### Option C — Keep constructors hidden behind a builtin instead of `new`
- Reuse L2 objects and add a helper such as `construct(fn, args...)`.
- Pros: less parser/compiler work.
- Cons: conflicts with roadmap deliverable naming, delays real `new` syntax, and creates migration debt for L4.

## 7) ADR
### Decision
Implement L3 with explicit `this` and `new` syntax, receiver-aware call semantics, and a narrow function `prototype` slot used by constructors, while keeping classes and general function-object semantics out of scope.

### Drivers
- Roadmap requires real `this`/constructor/`new` behavior now.
- L2 already provides the prototype lookup model L3 should build on.
- Current evaluator/compiler/VM call paths require an explicit receiver strategy to avoid ad hoc drift.
- Scope must stay materially smaller than L4.

### Alternatives considered
- **General function-object semantics now:** rejected as too broad for L3 and likely to absorb later class/runtime concerns.
- **Builtin-only construction helper:** rejected because it avoids the roadmap’s `new` surface and creates a second constructor API to undo later.

### Why chosen
This option unlocks constructor-style object patterns with the least architecture churn: it uses the L2 prototype chain directly, makes `this` behavior explicit, keeps `new` user-visible as required, and avoids prematurely pulling generic function property bags/classes into scope.

### Consequences
- `JsFunction`, GC marking/sweeping, property access, assignment, and call execution paths all need coordinated changes.
- Tests must lock down nuanced constructor return rules and prototype inheritance to prevent backend drift.
- L4 remains free to layer class syntax/desugaring over the L3 constructor/runtime model.

### Follow-ups
- Update the semantics baseline and milestone checklist with L3-specific bounded guarantees.
- If future work requires arbitrary function properties, treat that as a separate design step rather than silently broadening L3.
- After L3 lands, reassess whether L4 can desugar classes directly to the constructor/prototype model without additional runtime primitives.

## 8) Recommended file plan
### Core runtime / parser touchpoints
- `include/jsrt/token.h` — add `this` / `new` token kinds.
- `src/lexer.c` — recognize keywords.
- `include/jsrt/ast.h` / `src/ast.c` — add AST forms and constructors/destructors.
- `src/parser.c` — parse `this`, `new`, and bounded precedence/recovery cases.
- `include/jsrt/value.h` — add constructor prototype slot / metadata to `JsFunction`.
- `src/eval.c` — add receiver-aware call evaluation, `this` lookup, constructor behavior, and function `prototype` handling.
- `include/jsrt/bytecode.h` — add receiver/new bytecode shape if needed.
- `src/compiler.c` — compile method/new calls distinctly from plain calls.
- `src/vm.c` — execute receiver-aware calls/new and mirror diagnostics.
- `src/gc.c` — mark/sweep any new function-owned references.

### Test / docs touchpoints
- `tests/test_lexer.c`
- `tests/test_parser.c`
- `tests/test_eval.c`
- `tests/test_vm.c`
- `docs/semantics-baseline.md`
- `docs/milestone-verification-checklist.md`
- optionally `examples/` for one constructor/prototype demo fixture

## 9) Available-agent-types roster + staffing guidance
### Recommended Ralph path (sequential owner)
- `architect` (high) — confirm final opcode/call-shape design before edits.
- `executor` (high) — main implementation across parser/runtime/compiler/vm.
- `debugger` (high) — only if parity/GC regressions appear.
- `verifier` (high) — milestone closure, evidence bundle, checklist validation.

### Recommended Team path (parallel lanes)
1. `architect` (high): own call-model and function-prototype-slot design note.
2. `executor` (high): own syntax/AST/evaluator/compiler/VM implementation plan integration.
3. `test-engineer` (medium): own lexer/parser/eval/vm matrix and example coverage.
4. `verifier` (high): own final evidence, checklist, ASan lane, and docs parity.
5. Optional `debugger` (high): reserve for backend drift or GC/rooting failures.

### Concrete staffing / role allocation
- **Lane A — syntax/front-end:** `executor` on `include/jsrt/token.h`, `src/lexer.c`, `include/jsrt/ast.h`, `src/ast.c`, `src/parser.c`.
- **Lane B — runtime semantics:** `executor` on `include/jsrt/value.h`, `src/eval.c`, `src/gc.c`.
- **Lane C — compiler/VM parity:** `executor` or second `executor` on `include/jsrt/bytecode.h`, `src/compiler.c`, `src/vm.c`.
- **Lane D — tests/docs:** `test-engineer` on `tests/*`, `docs/semantics-baseline.md`, `docs/milestone-verification-checklist.md`, example fixture.
- **Lane E — closure:** `verifier` runs full test/ASan/example evidence and checks evaluator/VM parity.

### Launch hints for later execution
- **Ralph:** use when one owner should drive the sequence `design confirm -> implement -> parity fix -> verify`.
- **Team:** use when front-end/runtime/compiler/test lanes can move in parallel after the architect locks the call model.
- Suggested team follow-up: start with `architect` brief, then parallelize A/B/C/D, then run `verifier`, then loop a `debugger` lane only if evidence fails.

# PRD — JavaScript Runtime L4 Class Syntax and Desugaring

## Scope
Roadmap milestone L4: add a bounded `class` declaration surface on top of the landed L3 constructor/prototype runtime without introducing a parallel OO model or widening into full ECMAScript class semantics.

## Evidence baseline
- `.omx/plans/prd-javascript-runtime-roadmap.md:280-288` defines L4 as `class syntax and desugaring/direct support`, high risk, recommended mode `team`.
- `docs/semantics-baseline.md:71-79` records the current L3 substrate: prototype chains, receiver-bound methods, parenthesized `new`, and constructor prototype wiring.
- `docs/semantics-baseline.md:100-107` still defers classes / `super`.
- `docs/project-status-and-architecture.md:11-12` keeps the tree-walk evaluator as semantic authority, while `docs/project-status-and-architecture.md:225-227` documents the existing VM call-family lowering.
- `.omx/plans/prd-l3-this-binding-and-constructor-semantics.md` explicitly positioned L3 as the constructor/prototype substrate for future class lowering.
- The old generic milestone-4 loop/logical docs are stale early-project artifacts and not canonical for roadmap L4.

## 1) Requirements summary
### In scope
1. Add bounded class declarations:
   - `class Name { ... }`
   - optional `constructor(...) { ... }`
   - zero or more instance methods with identifier names
2. Lower classes onto the existing L3 constructor/prototype runtime:
   - class name becomes a constructable function value with class metadata
   - instance methods are installed on `Name.prototype`
   - `new Name(args)` reuses existing L3 constructor and prototype semantics
3. Define the missing class runtime contract explicitly:
   - `Name()` without `new` fails with `RUNTIME_ERROR_TYPE`
   - class declarations follow the repo’s current sequential declaration model when they execute / compile; no TDZ emulation in first-slice L4
4. Preserve existing L3 behavior under the class surface:
   - receiver-bound `this`
   - prototype-chain method lookup
   - constructor return rules already established in L3
5. Add explicit parser/diagnostic coverage for unsupported class syntax.
6. Keep evaluator/VM parity milestone-blocking for this L4 slice.
7. Update docs, checklist, and examples.

### Out of scope
- `extends`
- `super`
- static methods, static fields, instance fields
- getters/setters
- computed method names
- class expressions / anonymous classes
- private members
- broader function-object property bags beyond the current L3 `prototype` slot
- broader `new` syntax or constructor-result chaining changes

### Explicit keyword / rejection policy
- Reserve `class`, `extends`, and `super` in the lexer.
- Keep `static`, `get`, and `set` as ordinary identifiers outside class bodies.
- Inside class bodies, reject modifier/accessor forms such as `static foo() {}`, `get foo() {}`, and `set foo(v) {}`.
- Ordinary method names like `get() {}` and `set() {}` remain valid.
- Unsupported class syntax must fail in parser/diagnostic coverage, not drift into later runtime/name failures.

## 2) Acceptance criteria
1. Lexer/parser recognize `class` and produce bounded class declaration syntax with source diagnostics.
2. `class Box { constructor(v) { this.value = v; } get() { return this.value; } } let b = new Box(3); b.get();` succeeds on tree-walk and VM backends.
3. When no constructor is declared, `new` still produces an instance using the class prototype and a default empty constructor behavior.
4. Instance methods are installed on the class prototype, not copied per instance.
5. Methods called through instances bind `this` exactly like current L3 prototype methods.
6. Extracted class methods behave like ordinary extracted functions under L3 (`this = null`).
7. `Box()` without `new` fails with stable runtime type diagnostics on both backends.
8. Class declarations intentionally follow the current repo declaration model: no TDZ emulation or alternate hoisting contract is introduced, and that choice is parity-tested for use-before-declaration, block-local visibility, and redeclaration behavior.
9. Bounded L4 class constructors keep the current L3 `prototype` contract: `Name.prototype` / `Name["prototype"]` remain readable and writable under the existing object-or-null rules, and class lowering installs methods through that same slot.
10. Duplicate `constructor` definitions are rejected.
11. Unsupported members and syntax (`static foo() {}`, fields, `get foo() {}`, `set foo(v) {}`, computed names, `extends`, `super`, class expressions) fail with stable parser/diagnostic coverage.
12. Bytecode/disassembly evidence shows lowered class forms continue to use the existing L3 call-family/runtime model. Metadata-only additions are allowed, but no new class-specific call/dispatch opcodes are introduced in first-slice L4.
13. `docs/semantics-baseline.md` and `docs/project-status-and-architecture.md` are updated to mark bounded class declarations as supported while keeping `extends`/`super` and broader class surface deferred.

## 3) Implementation steps
1. **Front-end syntax slice**
   - Add `TOKEN_CLASS`, `TOKEN_EXTENDS`, and `TOKEN_SUPER`.
   - Add parser-owned contextual rejection for accessor/modifier forms inside class bodies.
   - Likely files: `include/jsrt/token.h`, `src/lexer.c`, `tests/test_lexer.c`.
2. **AST and parser slice**
   - Add explicit class declaration/member AST nodes.
   - Parse bounded member grammar: optional single constructor + identifier-named instance methods.
   - Reject out-of-scope members with source diagnostics.
   - Likely files: `include/jsrt/ast.h`, `src/ast.c`, `src/parser.c`, `tests/test_parser.c`, `tests/test_diagnostics.c`.
3. **Lowering contract slice**
   - Define one canonical lowering contract from class declaration to L3 constructs:
     - constructor function binding
     - prototype method installation
     - default constructor when omitted
     - construct-only class-call rejection contract
   - Keep the lowering shared conceptually across evaluator and compiler.
   - Likely files: `src/eval.c`, `src/compiler.c`, `include/jsrt/bytecode.h`, `src/bytecode.c`, `tests/test_bytecode.c`.
4. **Tree-walk implementation slice**
   - Evaluate class declarations by constructing function values + prototype methods using current L3 primitives.
   - Reuse existing `this`, `new`, and function `prototype` machinery; add only the minimum class metadata needed to reject plain calls.
   - Likely files: `src/eval.c`, `include/jsrt/value.h`, `src/value.c`, `src/gc.c`, `tests/test_eval.c`.
5. **Compiler / VM parity slice**
   - Lower class declarations into bytecode preserving the same L3 constructor/prototype semantics.
   - Reuse `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW` where possible; metadata-only additions are acceptable if needed for construct-only class behavior, but new class-specific call/dispatch opcodes are out of scope.
   - Parity is milestone-blocking for this L4 slice.
   - Likely files: `src/compiler.c`, `src/vm.c`, `include/jsrt/bytecode.h`, `src/bytecode.c`, `tests/test_vm.c`, `tests/test_bytecode.c`.
6. **Docs / examples / verification slice**
   - Add a dedicated class example using constructor + instance methods.
   - Update baseline/status/checklist docs to describe the bounded L4 surface precisely.
   - Add an ADR note that later `extends` / `super` likely need superclass/home-object metadata and possibly wider function heritage affordances.
   - Likely files: `examples/`, `docs/semantics-baseline.md`, `docs/project-status-and-architecture.md`, `docs/milestone-verification-checklist.md`.

## 4) Risks and mitigations
- **Risk: parser-only desugaring obscures diagnostics and future growth.**
  - Mitigation: explicit class AST nodes plus one shared lowering contract.
- **Risk: class support widens into half-implemented ES classes.**
  - Mitigation: freeze the first landing to declarations + constructor + instance methods only; reject `extends`, `super`, static members, fields, computed names.
- **Risk: evaluator and VM diverge on omitted-constructor/default-constructor behavior or construct-only call rejection.**
  - Mitigation: mirrored positive/negative tests and bytecode evidence.
- **Risk: class method installation or later user mutation of `Name.prototype` drifts from the current L3 prototype-slot contract.**
  - Mitigation: explicitly lock read/write behavior for class constructor `prototype` and test it on both backends.
- **Risk: hidden allocation/rooting changes cause GC regressions.**
  - Mitigation: ASan regression and GC-sensitive example/test proof.
- **Risk: future `extends` / `super` work is underestimated.**
  - Mitigation: document now that subclassing/super likely require explicit superclass/home-object metadata and possibly wider function heritage support beyond the current L3 slot model.

## 5) Verification steps
1. Lexer tests for `class`, `extends`, and `super` tokenization plus contextual rejection policy for unsupported class-body modifiers/accessors.
2. Parser tests for:
   - minimal class declaration
   - class with constructor + multiple instance methods
   - omitted-constructor case
   - duplicate constructor rejection
   - rejection of `extends`, `super`, `static foo() {}`, fields, `get foo() {}`, `set foo(v) {}`, computed names, class expressions, malformed members
3. Evaluator tests for:
   - instance creation through `new`
   - method dispatch through prototype-installed methods
   - extracted method fallback to L3 `this = null`
   - omitted-constructor/default-constructor behavior
   - `Box()` without `new` rejection
   - class method calling another method through `this`
   - use-before-declaration / block-local visibility / redeclaration behavior under the chosen no-TDZ declaration model
   - `Name.prototype` read/write parity under the existing L3 object-or-null rules
4. VM parity tests for the same supported subset and rejection matrix, including the chosen declaration model.
5. Bytecode/disassembly tests showing class lowering routes through existing constructor/method/new infrastructure. Acceptable proof is: instance construction still uses `BC_CALL_NEW`, instance method calls still use `BC_CALL_METHOD`, plain helper calls still use `BC_CALL`, and no new class-specific invocation opcode appears in disassembly.
6. End-to-end example run for a class-based fixture on both default and VM backends when the feature is inside the supported VM subset.
7. Full regression:
   - `make clean && make test`
   - `make clean && make test CFLAGS='-std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g -fsanitize=address -fno-omit-frame-pointer'`
8. Docs verification:
   - baseline/status/checklist text matches the bounded L4 surface and still defers `extends` / `super`
   - docs explicitly state the chosen non-TDZ declaration model for first-slice classes

## 6) RALPLAN-DR summary
### Principles
1. Build L4 as syntax over the landed L3 constructor/prototype runtime, not as a parallel object model.
2. Preserve evaluator/VM semantic parity through one explicit lowering contract.
3. Keep the first class landing narrow enough to verify exhaustively.
4. Prefer explicit rejections over implicit scope creep.
5. Let diagnostics/documentation make the class boundary as clear as the implementation.

### Decision drivers
1. **L3 substrate reuse**
2. **Scope control under high risk**
3. **Backend parity as a deliberate milestone choice**

### Viable options
- **Option A — Parser-time pure desugaring to existing function/prototype AST**
  - Pros: smaller evaluator/compiler delta; maximal reuse of L3 forms
  - Cons: weaker diagnostics, worse future staging, less transparent source mapping
- **Option B — Explicit class AST with deterministic lowering to L3 runtime semantics** **(recommended)**
  - Pros: clearer diagnostics, boundaries, staging, and parity reasoning
  - Cons: touches more surfaces and must own class-call + declaration-binding contracts explicitly
- **Option C — Tree-walk-first class lowering with delayed VM parity gate**
  - Pros: smaller first landing, aligns with semantic-authority-first thinking
  - Cons: weaker closure durability and higher rewrite risk
- **Option D — Direct runtime class support with new class-specific machinery**
  - Pros: could fit future `super`/static/field work more naturally
  - Cons: too broad for the first L4 slice and violates the syntax-over-L3 boundary

### Recommended option
**Option B** — explicit class AST with deterministic lowering to current L3 constructor/prototype semantics.

### Pre-mortem
1. Tree-walk lands, VM parity slips.
2. Class syntax quietly widens into half-implemented ES classes.
3. Future inheritance work is boxed in by the first L4 shape.

### Expanded test plan
- **Unit:** tokenization, AST construction, parser rejection diagnostics
- **Integration:** constructor/method behavior, construct-only rejection, declaration-model cases, `prototype` contract parity
- **End-to-end:** class example on tree-walk and VM backends
- **Observability:** bytecode/disassembly snapshots, parser error text, ASan output, final evidence bundle

## 7) ADR
### Decision
Implement bounded class declarations with explicit class AST nodes lowered onto the existing L3 constructor/prototype runtime model, plus the minimum class metadata needed to make classes construct-only.

### Drivers
- L4 must build on completed L3 substrate.
- First class landing must stay materially smaller than full ES classes.
- Source-aware diagnostics and backend parity matter enough to avoid parser-only magic.
- This plan chooses parity-first closure instead of a temporary VM deferral gate.

### Alternatives considered
- Pure parser desugaring to L3 AST
- Tree-walk-first lowering with delayed VM parity
- Direct class-specific runtime machinery

### Why chosen
It keeps the mental model clean: users get class syntax, but runtime behavior remains the L3 constructor/prototype system.

### Consequences
- Cross-cutting milestone touching frontend, evaluator, compiler, VM, tests, docs
- Attractive class features remain deferred by design
- Docs must state the bounded class surface explicitly
- A narrow class metadata flag/contract is likely needed so `Box()` without `new` does not silently behave like a plain function call

### Follow-ups
1. Add an L4 section to `docs/milestone-verification-checklist.md`.
2. Treat `extends` / `super` as later work requiring superclass/home-object metadata and possibly wider function heritage affordances.
3. Reassess after L4 whether a later L4.x follow-up should own inheritance/static surface separately.

## 8) Available-agent-types roster + staffing guidance
### Available agent types
- `explore` — low
- `planner` — medium
- `architect` — high
- `executor` — high
- `debugger` — high
- `test-engineer` — medium
- `verifier` — high
- `writer` — high
- `critic` — high

### Ralph path
- Primary: `executor` (high)
- Checkpoint: `architect` (high)
- Regression: `test-engineer` (medium)
- Closure: `verifier` (high)
- Optional docs: `writer` (high)

### Team path
Recommended execution mode: `team`

Suggested staged lanes:
1. `2:architect` — boundary/rejection matrix + lowering/construct-only contract
2. `3:executor` — lexer/parser/AST, evaluator/runtime, compiler/VM parity
3. `2:test-engineer` — positive/rejection matrix + bytecode/ASan/example proof
4. `1:verifier` — evidence bundle and closure gate

### Team verification path
- canonical PRD + test spec finalized before launch
- dedicated VM parity lane from day one
- fresh evidence for parser diagnostics, evaluator tests, VM tests, bytecode snapshots, examples, and ASan
- completion gate requires no pending tasks, docs updated, bounded out-of-scope surface still rejected, verifier approval

### Launch hints
```text
$team 2:architect "Roadmap L4 for jsrt: finalize bounded class declaration surface, lowering contract to current L3 constructor/prototype runtime, construct-only class-call semantics, and rejection matrix for out-of-scope class features"
```

```text
$team 3:executor "Roadmap L4 for jsrt: implement bounded class declarations on top of the landed L3 constructor/prototype runtime; split lexer/parser/AST, evaluator/runtime lowering, and compiler/VM parity lanes; preserve explicit rejection of extends/super/static/fields/computed names"
```

```text
$team 2:test-engineer "Roadmap L4 verification for jsrt: prove bounded class support with parser diagnostics, evaluator/VM parity, bytecode evidence, example runs, construct-only rejection, and ASan output"
```

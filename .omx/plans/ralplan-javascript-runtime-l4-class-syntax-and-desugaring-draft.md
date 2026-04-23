# RALPLAN-DR Draft — JavaScript Runtime L4 Class Syntax and Desugaring/Direct Support

## Scope and intent
Roadmap milestone L4 is the first higher-level OO syntax layer over the landed L3 substrate. The current repo already supports prototype-chain lookup, receiver-bound `this`, parenthesized `new`, and the narrow function `prototype` meta-surface, while `class` / `super` remain explicitly unsupported in the current baseline. This draft converts that state into a bounded, execution-ready consensus plan for L4 without reopening broader object-model work.

## Evidence baseline
- `.omx/plans/prd-javascript-runtime-roadmap.md:280-288` defines L4 as `class syntax and desugaring/direct support`, high risk, recommended mode `team`.
- `docs/semantics-baseline.md:71-79` records the landed L3 substrate: prototype chains, receiver-bound methods, parenthesized `new`, constructor prototype wiring.
- `docs/semantics-baseline.md:100-107` still lists classes / `super` / broader `new` chaining as deferred.
- `docs/project-status-and-architecture.md:11-12` keeps the tree-walk evaluator as semantic authority, while `docs/project-status-and-architecture.md:225-227` documents the existing VM call-family lowering.
- `docs/project-status-and-architecture.md:391-398` positions L4 as the next long-horizon OO step after completed L3.
- `.omx/plans/prd-l3-this-binding-and-constructor-semantics.md` explicitly framed L3 as the constructor/prototype substrate for future class lowering.
- The stale generic milestone-4 artifacts (`.omx/plans/prd-javascript-runtime-milestone-4.md`, `.omx/plans/test-spec-javascript-runtime-milestone-4.md`) are early-project loop/logical-operator docs and are **not** canonical for roadmap L4.

## Proposed canonical artifact names
- Canonical PRD: `.omx/plans/prd-javascript-runtime-l4-class-syntax-and-desugaring.md`
- Canonical test spec: `.omx/plans/test-spec-javascript-runtime-l4-class-syntax-and-desugaring.md`
- This draft artifact: `.omx/plans/ralplan-javascript-runtime-l4-class-syntax-and-desugaring-draft.md`

## 1) Requirements summary for bounded, execution-ready L4 scope
### In scope
1. Add **class declarations** in bounded L4 form:
   - `class Name { ... }`
   - optional `constructor(...) { ... }`
   - zero or more instance methods with identifier names
2. Lower class behavior onto the existing L3 constructor/prototype runtime:
   - the class name becomes a constructable function value with class metadata
   - instance methods are installed on `Name.prototype`
   - `new Name(args)` reuses existing L3 constructor and prototype semantics
3. Define the missing class runtime contract explicitly:
   - `Name()` without `new` is rejected with `RUNTIME_ERROR_TYPE`
   - class declarations use the repo’s existing sequential declaration model: the binding becomes available when the declaration executes / compiles, with no TDZ emulation in this bounded L4 slice
4. Preserve existing L3 behavior under the class surface:
   - receiver-bound `this`
   - prototype-chain method lookup
   - constructor return rules already established in L3
5. Add bounded class diagnostics:
   - duplicate constructors rejected
   - invalid method syntax rejected
   - unsupported class members rejected with explicit parser/diagnostic coverage
6. Make evaluator/VM parity milestone-blocking for this L4 slice.
7. Update docs, milestone verification checklist, and at least one end-to-end example.

### Explicitly out of scope for this first L4 landing
- `extends`
- `super`
- static methods, static fields, and instance fields
- getters/setters
- computed method names
- class expressions / anonymous classes
- private fields / methods
- arbitrary function-object property bags beyond the current L3 `prototype` slot
- changes to constructor-result chaining or broader `new` syntax

### Boundary rule
L4 should ship **syntax sugar over the current L3 runtime model**, not a second OO runtime. If a proposed change would require a new non-L3 object/constructor semantic unrelated to the bounded class contract above, it should be deferred to a later milestone or ADR revision.

### Explicit keyword / rejection policy
- Reserve `class`, `extends`, and `super` in the lexer so unsupported class/heritage syntax is parser-owned instead of degrading into identifier/name failures.
- Keep `static`, `get`, and `set` as ordinary identifiers outside class bodies. Inside class bodies, reject modifier/accessor forms such as `static foo() {}`, `get foo() {}`, and `set foo(v) {}` explicitly, while still allowing ordinary identifier-named methods like `get() {}` and `set() {}`.
- Rejection quality matters: unsupported class syntax must fail in parser/diagnostic coverage, not drift into later runtime errors.

## 2) Acceptance criteria
1. Lexer/parser recognize `class` and produce bounded class declaration syntax with source diagnostics.
2. `class Box { constructor(v) { this.value = v; } get() { return this.value; } } let b = new Box(3); b.get();` succeeds on tree-walk and VM backends.
3. When no constructor is declared, `new` still produces an instance using the class prototype and a default empty constructor behavior.
4. Instance methods are installed on the class prototype, not copied per instance.
5. Methods called through instances bind `this` exactly like current L3 prototype methods.
6. Extracted class methods behave like ordinary extracted functions under L3 (`this = null`), unless a future milestone explicitly changes that rule.
7. `Box()` without `new` fails with stable runtime type diagnostics on both backends.
8. Class declarations follow the current repo declaration model intentionally: no TDZ emulation or alternate hoisting contract is introduced in this first L4 slice, and that choice is parity-tested for use-before-declaration, block-local visibility, and redeclaration behavior.
9. Bounded L4 class constructors keep the current L3 `prototype` contract: `Name.prototype` / `Name["prototype"]` remain readable and writable under the existing object-or-null rules, and class lowering installs methods through that same slot.
10. Duplicate `constructor` definitions are rejected.
11. Unsupported members and syntax (`static foo() {}`, fields, `get foo() {}`, `set foo(v) {}`, computed names, `extends`, `super`, class expressions) fail with stable parser/diagnostic coverage rather than silently widening scope.
12. Bytecode/disassembly evidence shows the lowered class forms continue to use the existing L3 call-family/runtime model rather than introducing a hidden second dispatch path. Metadata-only additions are allowed, but no new class-specific call/dispatch opcodes are introduced in this first L4 slice.
13. `docs/semantics-baseline.md` and `docs/project-status-and-architecture.md` are updated to mark bounded class declarations as supported while keeping `extends`/`super` and broader class surface deferred.

## 3) Implementation steps with likely file references
1. **Front-end syntax slice**
   - Add `TOKEN_CLASS`, `TOKEN_EXTENDS`, and `TOKEN_SUPER`.
   - Add parser-owned contextual rejection for accessor/modifier forms inside class bodies: `static foo() {}`, `get foo() {}`, `set foo(v) {}`. Ordinary method names `get()` and `set()` remain valid.
   - Likely files: `include/jsrt/token.h`, `src/lexer.c`, `tests/test_lexer.c`.
2. **AST and parser slice**
   - Add explicit class declaration/member AST nodes instead of desugaring entirely inside the parser.
   - Parse the bounded member grammar: optional single constructor + identifier-named instance methods.
   - Reject out-of-scope members deliberately and with source diagnostics.
   - Likely files: `include/jsrt/ast.h`, `src/ast.c`, `src/parser.c`, `tests/test_parser.c`, `tests/test_diagnostics.c`.
3. **Lowering contract slice**
   - Define one canonical lowering contract from class declaration to existing L3 constructs:
     - constructor function binding
     - prototype method installation
     - default constructor when omitted
     - construct-only class-call rejection contract
   - Keep this lowering shared conceptually across evaluator and compiler so diagnostics and runtime behavior do not drift.
   - Likely files: `src/eval.c`, `src/compiler.c`, `include/jsrt/bytecode.h`, `src/bytecode.c`, `tests/test_bytecode.c`.
4. **Tree-walk implementation slice**
   - Evaluate class declarations by constructing the function value + prototype methods using current L3 primitives.
   - Reuse the existing `this`, `new`, and function `prototype` machinery; add only the minimum class metadata needed to reject plain calls.
   - Likely files: `src/eval.c`, `include/jsrt/value.h`, `src/value.c`, `src/gc.c`, `tests/test_eval.c`.
5. **Compiler / VM parity slice**
   - Lower class declarations into bytecode that preserves the same L3 constructor/prototype semantics.
   - Reuse `BC_CALL`, `BC_CALL_METHOD`, and `BC_CALL_NEW` where possible; metadata-only additions are acceptable if needed for construct-only class behavior, but new class-specific call/dispatch opcodes are out of scope for first-slice L4.
   - Parity is milestone-blocking for this L4 slice, not a deferred follow-up.
   - Likely files: `src/compiler.c`, `src/vm.c`, `include/jsrt/bytecode.h`, `src/bytecode.c`, `tests/test_vm.c`, `tests/test_bytecode.c`.
6. **Docs / examples / milestone verification slice**
   - Add a dedicated class example using constructor + instance methods.
   - Update baseline/status/checklist docs to describe the bounded L4 surface precisely.
   - Add an ADR note that later `extends` / `super` likely need superclass/home-object metadata and possibly wider function heritage affordances.
   - Likely files: `examples/`, `docs/semantics-baseline.md`, `docs/project-status-and-architecture.md`, `docs/milestone-verification-checklist.md`.

## 4) Risks and mitigations
- **Risk 1: parser-only desugaring obscures diagnostics and makes future L4 growth brittle.**
  - Mitigation: preserve explicit class AST nodes, then lower in evaluator/compiler with one shared semantic contract.
- **Risk 2: class support accidentally widens into half-implemented ES classes.**
  - Mitigation: freeze the first landing to declarations + constructor + instance methods only; reject `extends`, `super`, static members, fields, computed names.
- **Risk 3: evaluator and VM diverge on omitted-constructor/default-constructor behavior or construct-only call rejection.**
  - Mitigation: add mirrored positive/negative tests and bytecode evidence for default-constructor lowering and `Box()` rejection.
- **Risk 4: method installation may accidentally rely on unsupported general function property bags.**
  - Mitigation: keep installation path centered on the existing narrow `prototype` meta-surface from L3.
- **Risk 5: hidden allocation/rooting changes cause GC regressions when class declarations synthesize constructor/prototype objects.**
  - Mitigation: require ASan regression and GC-sensitive example/test runs as part of milestone proof.
- **Risk 6: later `extends` / `super` work is underestimated.**
  - Mitigation: state now that subclassing/super likely require explicit superclass/home-object metadata and possibly wider function heritage support beyond the current L3 slot model.

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
   - ensure baseline/status/checklist text matches the bounded L4 surface and still defers `extends` / `super`
   - ensure the docs explicitly state the chosen non-TDZ declaration model for first-slice classes

## 6) RALPLAN-DR summary
### Principles
1. Build L4 as syntax over the landed L3 constructor/prototype runtime, not as a parallel object model.
2. Preserve evaluator/VM semantic parity through one explicit lowering contract.
3. Keep the first class landing narrow enough to verify exhaustively.
4. Prefer explicit rejections over implicit scope creep.
5. Let diagnostics/documentation make the class boundary as clear as the implementation.

### Decision drivers (top 3)
1. **L3 substrate reuse:** the runtime already supports the constructor/prototype semantics that classes should ride on.
2. **Scope control under high risk:** classes are roadmap-high-risk and need a smaller first landing than “mini-ES classes.”
3. **Backend parity:** this plan deliberately treats evaluator/VM parity as milestone-blocking so L4 does not create a second semantics gap after L3.

### Viable options
#### Option A — Parser-time pure desugaring to existing function/prototype AST
- **Pros:** potentially smaller evaluator/compiler delta; maximal reuse of L3 forms.
- **Cons:** class-specific diagnostics become harder to preserve; future L4 growth (`extends`, class expressions) becomes harder to stage cleanly; source-to-runtime mapping is less transparent.

#### Option B — Explicit class AST with deterministic lowering to L3 runtime semantics **(recommended)**
- **Pros:** clearer diagnostics, clearer boundaries, easier future staging, better evaluator/VM parity reasoning, still grounded in the L3 runtime.
- **Cons:** touches more surfaces up front (`ast`, parser, evaluator, compiler, tests); requires owning the class-call and declaration-binding contracts explicitly.

#### Option C — Tree-walk-first class lowering with delayed VM parity gate
- **Pros:** smaller first landing; aligns with tree-walk semantic authority; uses the roadmap’s temporary post-M8 VM gate.
- **Cons:** weakens the milestone’s parity promise, risks a second rewrite for VM lowering, and makes the first L4 closure less durable.

#### Option D — Direct runtime class support with new class-specific object/function machinery
- **Pros:** could map more naturally to future `super`/static/field work.
- **Cons:** materially expands scope, risks duplicating constructor/prototype semantics, and weakens the roadmap promise that L4 layers onto the chosen runtime model.

### Explicit recommended option
Recommend **Option B: explicit class AST with deterministic lowering to current L3 constructor/prototype semantics**.

Reason: it preserves source-aware diagnostics and future staging flexibility while still keeping the runtime contract grounded in the L3 substrate. It is the best tradeoff between short-term simplicity and long-term maintainability for this repo.

### Pre-mortem (3 scenarios)
1. **Scenario: “L4 ships but only tree-walk works reliably.”**
   - Failure mode: parser/evaluator path lands, VM lowering diverges or is left incomplete.
   - Countermeasure: keep VM parity as an explicit closure requirement and assign a dedicated parity lane from day one.
2. **Scenario: “Class syntax quietly widens into half-implemented ES classes.”**
   - Failure mode: `extends`, `super`, fields, or static members partially parse or partially run.
   - Countermeasure: reject all non-L4-first-slice members explicitly and lock them with parser/diagnostic tests.
3. **Scenario: "Future inheritance work is boxed in by the first L4 shape."**
   - Failure mode: first-slice class lowering forgets that `super` will later need home-object/superclass metadata.
   - Countermeasure: document that future heritage support is a follow-up requiring explicit metadata design, not assumed free extension of first-slice lowering.

### Expanded test plan
#### Unit
- tokenization of `class`, `extends`, `super`
- AST construction for class declarations/members
- parser rejection diagnostics for out-of-scope syntax, especially modifier/accessor forms versus ordinary method names

#### Integration
- evaluator coverage for constructor + methods + omitted constructor
- construct-only rejection for plain calls
- chosen no-TDZ declaration behavior across use-before-declaration, block-local visibility, and redeclaration cases
- class constructor `prototype` read/write behavior matching the current L3 contract
- prototype installation and method lookup behavior
- extracted method behavior and `this` parity with L3

#### End-to-end
- example class script on tree-walk backend
- same fixture on VM backend when supported subset parity is declared for this feature
- doc example output checks where applicable

#### Observability / evidence
- bytecode/disassembly snapshots for lowered class forms
- parser snapshot/error text for unsupported forms
- ASan regression output for allocation/rooting-sensitive changes
- final milestone evidence bundle: tests, example runs, VM parity proof, and docs diff summary

## 7) ADR section
### Decision
Implement the first L4 landing as **bounded class declarations with explicit class AST nodes lowered onto the existing L3 constructor/prototype runtime model**, while adding the minimum class metadata needed to make classes construct-only.

### Drivers
- L4 must build on the completed L3 constructor/prototype substrate.
- The first class landing needs a smaller scope than full ES-style classes.
- Source-aware diagnostics and backend parity matter enough to avoid parser-only magic.
- This plan chooses parity-first closure instead of a temporary VM deferral gate.

### Alternatives considered
- **Pure parser desugaring to L3 AST:** simpler short term, weaker diagnostics/staging.
- **Tree-walk-first lowering with delayed VM parity:** smaller first landing, but less durable closure and higher rewrite risk.
- **Direct class-specific runtime machinery:** more future-looking, too broad for the first L4 slice.

### Why chosen
This approach keeps the mental model clean: users get class syntax, but the runtime continues to be the L3 constructor/prototype system. It is the most execution-safe path that still leaves room for later `extends` / `super` work without locking the project into brittle parser rewrites.

### Consequences
- The milestone will touch front-end, evaluator, compiler, VM, tests, and docs.
- Several attractive class features stay deferred by design.
- The docs must explain the bounded class surface unambiguously so readers do not over-assume ECMAScript compatibility.
- A narrow class metadata flag/contract is likely needed so `Box()` without `new` does not silently behave like a plain function call.

### Follow-ups
1. Create the canonical L4 PRD and test spec under the proposed artifact names.
2. Add an L4 section to `docs/milestone-verification-checklist.md` before implementation starts.
3. Treat `extends` / `super` as later work requiring superclass/home-object metadata and possibly wider function heritage affordances beyond the current `prototype` slot.
4. Reassess after L4 whether a later L4.x / L5-style follow-up should own inheritance/static surface separately.

## 8) Available-agent-types roster and follow-up staffing guidance
### Available-agent-types roster for this milestone
| Agent type | Best use for L4 | Suggested reasoning |
|---|---|---|
| `explore` | impact mapping, current L3 substrate lookup, fixture discovery | low |
| `planner` | PRD/test-spec refinement and execution sequencing | medium |
| `architect` | class-lowering contract, scope boundary decisions, parity design | high |
| `executor` | parser/runtime/compiler/VM implementation lanes | high |
| `debugger` | parity or GC/rooting failures | high |
| `test-engineer` | rejection matrix, regression fixtures, evidence bundle shaping | medium |
| `verifier` | completion gate, docs/test proof review | high |
| `writer` | docs/baseline/checklist/example polish | high |
| `critic` | challenge class-scope expansion and design leakage | high |

### `$ralph` follow-up staffing guidance
Use `$ralph` only if the approved L4 slice is narrowed enough that one owner can sequence work safely after design decisions are locked.

**Recommended Ralph lane shape**
- Primary lane: `executor` — **high**
- Design checkpoint: `architect` — **high**
- Regression lane: `test-engineer` — **medium**
- Final closure: `verifier` — **high**
- Optional docs lane: `writer` — **high**

**Best-fit Ralph use**
- post-consensus implementation of the bounded first slice after the canonical PRD + test spec exist
- bug-fix follow-up if Team finds one or two contained parity gaps rather than broad design churn

**Ralph verification path**
1. confirm canonical PRD + test spec exist
2. architect sign-off on the explicit lowering contract
3. implement in one owner sequence
4. run full test + ASan lanes
5. run example + VM parity proof
6. verifier sign-off against milestone checklist and docs updates

### `$team` follow-up staffing guidance
`$team` is the recommended execution mode for L4 because the milestone is high-risk and cross-cutting.

**Recommended staged Team staffing**
1. **Design phase** — `2:architect`, reasoning **high**
   - Lane A: class surface boundary + rejection matrix + keyword policy
   - Lane B: lowering contract + construct-only class-call contract + future-heritage note
2. **Implementation phase** — `3:executor`, reasoning **high**
   - Lane A: lexer/parser/AST
   - Lane B: evaluator/runtime wiring
   - Lane C: compiler/VM parity lowering
3. **Verification phase** — `2:test-engineer`, reasoning **medium**
   - Lane A: positive/rejection matrix + examples
   - Lane B: bytecode/VM parity + ASan evidence
4. **Final sign-off phase** — `1:verifier`, reasoning **high**
   - integrate evidence bundle, checklist validation, docs alignment

**Concrete team verification path**
- Pre-launch: finalize canonical PRD + test spec and define the bounded rejection matrix in the prompt.
- During implementation: keep one explicit lane responsible for VM parity from the start.
- During verification: require fresh evidence for parser diagnostics, evaluator tests, VM tests, bytecode snapshots, example runs, and ASan.
- Completion gate: no pending tasks, no failed tasks, docs updated, bounded out-of-scope surface still rejected, verifier approves the evidence bundle.

**Launch hints**
```text
$team 2:architect "Roadmap L4 for jsrt: finalize bounded class declaration surface, lowering contract to current L3 constructor/prototype runtime, construct-only class-call semantics, and rejection matrix for out-of-scope class features"
```

```text
$team 3:executor "Roadmap L4 for jsrt: implement bounded class declarations on top of the landed L3 constructor/prototype runtime; split lexer/parser/AST, evaluator/runtime lowering, and compiler/VM parity lanes; preserve explicit rejection of extends/super/static/fields/computed names"
```

```text
$team 2:test-engineer "Roadmap L4 verification for jsrt: prove bounded class support with parser diagnostics, evaluator/VM parity, bytecode evidence, example runs, construct-only rejection, and ASan output"
```

## Draft changelog after Architect review
- added explicit class-call rejection contract for `Name()` without `new`
- added explicit first-slice class declaration binding behavior (no TDZ emulation)
- made keyword reservation / contextual rejection policy concrete
- added a future `extends` / `super` metadata warning to the ADR and implementation plan
- made evaluator/VM parity a stated milestone-blocking choice instead of an implicit assumption

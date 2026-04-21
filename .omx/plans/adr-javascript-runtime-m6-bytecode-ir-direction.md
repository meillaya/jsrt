# ADR — M6 Bytecode / IR Direction for jsrt

## Status
Accepted for roadmap handoff to M7/M8.

## Decision
Proceed with a **stack-based bytecode architecture** as the next backend experiment, while keeping the existing tree-walk evaluator as the canonical semantic authority until explicit parity is proven.

The chosen direction is:
- compile the existing AST into a linear bytecode chunk format
- execute bytecode on a small stack VM with call frames and explicit locals
- keep frontend ownership unchanged: lexer/parser/AST still feed both backends
- keep runtime heap scope unchanged: `Env`, `JsFunction`, `JsArray`, and `JsObject`
- treat closures/upvalues, `switch`, `do...while`, and full current-surface parity as later expansion, not M7 defaults

## Why this synthesis
The current repo now has the prerequisites that M6 needed:
- bounded managed heap and GC observability from N8/M1
- a clarified aggregate/reference model from M3
- a more explicit runtime failure surface from M5
- a stable AST and evaluator that can remain the semantic oracle

That means the project can define a backend path without pretending it must replace the tree-walk evaluator immediately.

## Options considered

### 1. Register-based IR first
**Pros**
- more optimization-friendly long term
- can reduce stack shuffling

**Cons**
- higher compiler complexity for the first backend experiment
- less aligned with the repo's current learning-first, incremental posture

### 2. Stack-based bytecode **(chosen)**
**Pros**
- smallest credible compiler/backend step from the current AST
- easy to debug with a disassembler
- clear mapping from tree-walk expression evaluation to bytecode execution

**Cons**
- less optimization-friendly than a richer SSA/register IR
- may require later translation into another IR if optimization work grows

### 3. Skip bytecode and jump to a lower-level optimized IR
**Pros**
- potentially stronger long-term optimization story

**Cons**
- too large a conceptual jump from the current interpreter
- weakens the repo's learning-first progression

## Chosen M7/M8 parity target

M7/M8 should proceed on a **bounded subset**, not the entire current language:

### Included in the first backend target
- literals: number, bool, null, string
- variable declaration/load/store (`let`, `const`)
- arithmetic/comparison operators
- logical operators with short-circuit lowering
- blocks and expression statements
- `if` / `else`
- `while`
- `for` lowered to jumps
- user-defined non-capturing function declarations/calls/returns
- built-in function calls (`print`, `len`, `typeOf`)

### Explicitly deferred beyond the first backend target
- closure capture / upvalues
- function expressions that capture outer locals
- `switch`
- `do...while`
- `break` / `continue` if they materially complicate the first compiler pass
- array/object property/index mutation parity
- full aggregate semantics parity

## Instruction set draft

Core stack/value instructions:
- `CONST <k>`
- `NULL`
- `TRUE`
- `FALSE`
- `POP`
- `DUP`

Variable / slot instructions:
- `LOAD_LOCAL <slot>`
- `STORE_LOCAL <slot>`
- `LOAD_GLOBAL <symbol>`
- `STORE_GLOBAL <symbol>`
- `DEFINE_GLOBAL <symbol>`

Arithmetic / logical / comparison:
- `NEG`
- `NOT`
- `ADD`
- `SUB`
- `MUL`
- `DIV`
- `EQ`
- `NEQ`
- `LT`
- `LTE`
- `GT`
- `GTE`

Control flow:
- `JUMP <offset>`
- `JUMP_IF_FALSE <offset>`
- `LOOP <offset>`
- `RETURN`

Calls / functions:
- `CALL <argc>`
- `MAKE_FUNCTION <function-id>`

Built-in / aggregate-oriented instructions can stay out of M7 unless the subset expands.

## Compiler boundary

Recommended layers:
1. **Frontend stays unchanged** — lexer/parser/AST remain source of truth.
2. **Compiler pass** — `AST -> BytecodeChunk`.
3. **VM runtime** — executes bytecode with:
   - operand stack
   - call frame stack
   - locals array per frame
   - shared `Value` representation where feasible
4. **Parity harness** — compare tree-walk vs VM outputs for the supported subset.

This keeps M7/M8 additive instead of replacing the current evaluator prematurely.

## Runtime stack model

Per VM state:
- operand stack of `Value`
- call frame stack
- each frame stores:
  - instruction pointer
  - pointer/reference to the active chunk
  - base slot index into the operand/local stack
  - callable metadata

Locals should be slot-based, not environment-chain-based, for the non-capturing subset.

Closures/upvalues are deferred because they require a second lifetime model beyond plain frame slots.

## Adoption criteria
Proceed with M7/M8 only if:
1. the bounded subset above is accepted as enough to validate the backend path
2. tree-walk remains the semantic authority until parity is explicitly demonstrated
3. M7 can avoid upvalue/closure machinery initially
4. the VM can reuse the current `Value` families without widening GC scope
5. the compiler boundary does not require parser/AST redesign

## Abort criteria
Stop or defer M7/M8 if:
1. closure capture/upvalues are required immediately for the backend to be useful
2. aggregate mutation parity forces a much broader VM/object-model rewrite too early
3. the instruction set cannot stay small enough for a learning-first first backend
4. backend work starts destabilizing the tree-walk path instead of remaining additive

## Rollback path
If the backend experiment becomes too risky:
- keep the tree-walk evaluator as the only runtime backend
- preserve the M6 docs as a design artifact only
- drop any partial M7/M8 implementation branches
- revisit backend planning after more runtime simplification

## Downstream decision

### M7
**Proceed** as a bounded compiler MVP for the chosen subset.

### M8
**Proceed conditionally** after M7 only if:
- disassembly/compiler outputs are stable enough to debug
- VM results match tree-walk behavior across the bounded parity suite

## Directive for M7/M8
Treat this ADR as the source of truth:
- **backend shape:** stack VM
- **semantic authority:** tree-walk first
- **initial subset:** non-capturing, bounded, additive
- **guardrails:** no forced full-language parity, no premature closure/upvalue rewrite

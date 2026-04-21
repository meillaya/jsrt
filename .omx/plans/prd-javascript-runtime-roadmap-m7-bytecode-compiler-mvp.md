# PRD — Roadmap M7: Bytecode Compiler MVP

## Goal
Compile a bounded subset of the current AST into a stack-oriented bytecode format and expose a disassembler for verification.

## Why this milestone exists
M6 chose a stack-based backend direction and bounded first subset. M7 turns that design into concrete compiler artifacts without yet executing bytecode.

## In Scope
- bytecode data structures and opcode enum
- AST-to-bytecode compiler for the bounded M6 subset
- disassembler output suitable for golden tests
- compiler fixtures/tests for successful lowering and explicit unsupported-surface failures
- current-facing docs/README updates that describe the compiler MVP accurately

## Out of Scope
- bytecode execution / VM loop
- full-language parity
- closures/upvalues
- aggregate lowering/parity
- function expressions, `switch`, `do...while`, and `break`/`continue` in the compiler MVP

## Deliverables
1. Bytecode program/function representation.
2. Compiler pass from AST to bytecode for the bounded subset.
3. Disassembler with stable output for tests.
4. Golden/fixture tests plus explicit unsupported-feature compiler diagnostics.

## Acceptance Criteria
1. The repo can compile a bounded subset of programs into bytecode.
2. Disassembly is stable enough for golden-style tests.
3. Unsupported backend-surface features fail with explicit compiler diagnostics.
4. `make clean && make test` passes.
5. ASan verification passes.

## Suggested Execution Mode
- Primary: `ralph`
- Rationale: bounded implementation with a tight verify loop

## Suggested Lanes
- `executor` — compiler/disassembler/tests/docs
- `test-engineer` — fixture/golden-shape review
- `verifier` — final milestone evidence pass

## Primary Risks
- scope creep toward VM execution work
- ambiguous lowering rules for unsupported language surface
- unstable disassembly format that makes tests noisy

## Risk Mitigations
- keep M7 compiler-only
- explicitly reject deferred constructs with compiler diagnostics
- use a small deterministic disassembly format

## Dependencies
- `.omx/plans/adr-javascript-runtime-m6-bytecode-ir-direction.md`
- `docs/bytecode-ir-architecture-spike.md`

## Exit Condition
The repo has a concrete bytecode compiler/disassembler artifact set that M8 can execute against without rediscovering the backend shape.

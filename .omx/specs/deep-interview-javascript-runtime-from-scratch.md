# Deep Interview Spec — javascript-runtime-from-scratch

## Metadata
- Profile: standard
- Context type: greenfield
- Final ambiguity: 0.16
- Threshold: 0.20
- Rounds: 6
- Context snapshot: ".omx/context/javascript-runtime-from-scratch-20260420T152400Z.md"
- Transcript: ".omx/interviews/javascript-runtime-from-scratch-20260420T153309Z.md"

## Clarity breakdown

| Dimension | Score | Notes |
| --- | ---: | --- |
| Intent | 0.85 | Main goal is low-level systems practice in C. |
| Outcome | 0.85 | Milestone 1 is a tiny interpreter. |
| Scope | 0.85 | Scope is intentionally narrow; major engine/runtime features are deferred. |
| Constraints | 0.75 | Learning-first, greenfield, no unnecessary complexity in v1. |
| Success Criteria | 0.90 | v1 must run small scripts with variables, conditionals, and functions. |
| Context | 1.00 | Repo is currently greenfield for this task. |

## Intent
Build this project primarily as a systems-programming learning exercise in C. The project should maximize direct exposure to low-level implementation concerns rather than rush toward broad JavaScript compatibility.

## Desired Outcome
Deliver a first milestone that is a tiny JavaScript interpreter written in C, with enough language support to demonstrate end-to-end execution of small scripts.

## In-Scope
- Greenfield project setup for a C-based interpreter/runtime learning repo
- Lexer/tokenizer for a small JavaScript-like subset
- Parser producing an AST
- AST evaluation / tree-walk interpreter
- Variables and lexical environments/scopes
- Arithmetic and basic expressions
- Conditionals (if / else)
- Function definitions and function calls
- Small example scripts and tests that prove the interpreter works end to end
- Manual memory management patterns appropriate for a small C codebase
- Milestone slicing and internal architecture decisions made autonomously by OMX

## Out-of-Scope / Non-goals
- Garbage collector in milestone 1
- Bytecode VM or JIT in milestone 1
- Async/event loop behavior
- Modules, classes, prototypes, and broader advanced language-model features
- Browser APIs, Node APIs, DOM, filesystem host bindings, or embedding-focused host APIs
- Broad JavaScript compatibility as a short-term goal

## Decision Boundaries
OMX may decide without confirmation:
- Project structure and build setup
- Parser/AST design details
- Test strategy and sample/demo programs
- Manual memory-management conventions
- Milestone slicing

OMX should still escalate:
- Any materially broader scope than the tiny-interpreter milestone
- Any irreversible decision that changes the educational direction away from low-level C systems practice
- Any adoption of major new external dependencies contrary to current repo guidance

## Constraints
- The project is explicitly a learning exercise, not a production JS engine effort
- Milestone 1 should stay small enough to finish and reason about end to end
- Complexity should be added only when it directly improves the intended learning outcome
- Prefer decisions that expose core interpreter/runtime internals in C over decisions that hide them behind abstractions
- No need to front-load features that are better deferred to later milestones

## Testable Acceptance Criteria
Milestone 1 is complete when all of the following are true:
1. The project builds cleanly with the chosen C build setup.
2. The interpreter can tokenize and parse the supported subset into an internal AST.
3. The interpreter can evaluate expressions and variable bindings.
4. The interpreter can execute conditionals.
5. The interpreter can define and call functions.
6. At least a few small scripts run successfully end to end and demonstrate variables, conditionals, and functions.
7. Tests/examples exist to lock the intended milestone-1 behavior.

## Assumptions Exposed + Resolutions
- **Assumption:** The first version should aim broadly at “a JavaScript runtime.”
  - **Resolution:** Narrowed to a tiny interpreter first.
- **Assumption:** More features would necessarily make the learning exercise better.
  - **Resolution:** Rejected; the priority is low-level C systems practice, so unnecessary features should be cut.
- **Assumption:** Success might be defined by partial subsystems only.
  - **Resolution:** Rejected; success requires end-to-end small script execution.

## Pressure-pass Findings
- The earlier milestone choice was revisited with a tradeoff question.
- That follow-up clarified the protected objective: low-level systems practice in C matters more than feature breadth.
- This pressure pass should guide later planning decisions when tradeoffs emerge.

## Brownfield Evidence vs Inference Notes
- Not brownfield.
- Evidence: top-level repo contents only show ".omx/" state/log files.
- Inference: implementation will start from a clean slate.

## Technical Context Findings
- No existing source tree, build system, or tests are present yet.
- The repo is ready for greenfield planning and scaffold decisions.

## Condensed Transcript
1. User asked for recommendation on first milestone target.
2. Tiny interpreter was recommended and accepted.
3. GC, VM/JIT, async/event loop, advanced language features, and host APIs were deferred.
4. OMX was authorized to make most local technical/planning decisions.
5. The core protected goal was clarified as low-level systems practice in C.
6. Completion was defined as successfully running small scripts with variables, conditionals, and functions.

## Recommended Handoff
Recommended next step: $ralplan

Suggested invocation contract:
$plan --consensus --direct .omx/specs/deep-interview-javascript-runtime-from-scratch.md

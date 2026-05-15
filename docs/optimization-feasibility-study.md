# L10 Optional JIT / Advanced Optimization Feasibility Study

This document records the bounded **L10 — Optional JIT / advanced optimization feasibility study**.

## Summary recommendation

Do **not** build a native JIT in the current project phase. The runtime is still optimized for learning, clarity, portability, and explicit semantics. A native JIT would add platform-specific executable-memory management, security review burden, debugging complexity, and a second correctness surface before the bytecode backend has enough profiling evidence to justify it.

Recommended next optimization path:

1. Add bytecode-level peephole optimizations behind tests.
2. Add compiler constant folding for side-effect-free literal expressions.
3. Add benchmark/conformance gates for any optimization.
4. Consider inline caches only after object/property access benchmarks show stable hotspots.
5. Revisit native JIT only after bytecode optimization and profiling evidence show a clear need.

## Options evaluated

| Option | Feasibility | Risk | Recommendation |
|---|---:|---:|---|
| Bytecode peephole optimization | High | Low | Best first implementation target. Local bytecode rewrites are testable and reversible. |
| Compiler constant folding | High | Low/Medium | Good if restricted to literals and no runtime-error behavior changes. |
| Specialized opcodes | Medium | Medium | Useful after profiling identifies repeated instruction patterns. |
| Inline caches | Medium | Medium/High | Defer until object shapes/property access patterns stabilize. |
| Native JIT | Low currently | High | Do not implement now. Requires executable memory, architecture-specific codegen, security policy, and deep debugging. |

## Decision

L10 closes as a feasibility milestone with no runtime optimization prototype. The safest project-native outcome is a documented optimization roadmap, not premature native code generation.

## Guardrails for future optimization work

- Tree-walk evaluator remains the semantic oracle.
- Optimizations must be semantics-preserving and covered by `make test` plus `make conformance`.
- Any bytecode optimization must have disassembly or execution tests proving the optimized form.
- Any optimization that changes diagnostics or runtime-error timing requires explicit docs and tests.
- Native/JIT work requires a separate ADR for executable memory, target architectures, sandbox/security posture, and fallback behavior.

## Suggested first follow-up milestone

**O1 — Bytecode peephole optimizer MVP**

Scope:
- Add an optional bytecode optimization pass after compilation.
- Start with safe local rewrites such as redundant `BC_POP` cleanup only where stack effects are proven.
- Preserve disassembly tests and VM parity tests.
- Add benchmark comparison using the existing benchmark harness.

## Verification evidence

- This study is intentionally documentation-only.
- L10 status is reflected in `docs/repo-milestone-status.md` and `docs/project-status-and-architecture.md`.

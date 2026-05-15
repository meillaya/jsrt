# jsrt Support-Tier Matrix (P2)

Status: accepted for production-subset planning on 2026-05-15.

Tier definitions:

- **Supported**: documented, tested, and allowed in production-subset claims.
- **Experimental**: present for learning or staging; behavior may change and must not be marketed as compatibility.
- **Rejected**: intentionally out of scope for the bounded runtime.
- **Future**: planned or plausible, but not a current support claim.

| Area | Supported | Experimental | Future | Rejected / out of scope |
|---|---|---|---|---|
| Syntax core | literals, `let`, `const`, blocks, assignments, expressions, calls | none | richer diagnostics recovery | `with`, generators, destructuring, spread/rest |
| Control flow | `if`, loops, `do...while`, `switch`, `break`, `continue`, `return` | none | per-feature conformance expansion | labels, exceptions, `try`/`catch` |
| Functions/classes | declarations, expressions, closures, bounded class declarations, bounded `this`/`new` | class implementation remains intentionally small | constructor/class conformance expansion | decorators, private fields, class expressions unless separately accepted |
| Aggregates/object model | arrays, objects, shared references, prototype linkage | none | richer object built-ins | proxies, symbols, full property descriptors |
| Modules | tree-walk static named import/export | VM module lowering lane | linker/export-table parity | dynamic import, default/namespace import, package resolution until accepted |
| VM backend | opt-in parity for current VM-supported subset | unsupported diagnostics audit | module, async, regex parity gates | implicit fallback or silent divergence |
| Host API | source/file evaluation with owned strings | none | opaque handles, accessors, callbacks, fuel, job pump | undocumented ABI stability claims |
| Async/jobs | `defer` / `runJobs` callback queue experiment | callback queue | jobs, promises, async/await if accepted | timers/I/O/browser event loop |
| RegExp/stdlib | `strContains`, `strSlice`, tiny `regexMatch` helper | tiny regex matcher | bounded RegExp subset, captures/classes/flags if accepted | full ECMAScript RegExp compatibility claim |
| Conformance | repo-local positive/negative harness | Test262 adapter spike | curated Test262 subset gates | wholesale Test262 pass claims |
| Quality gates | unit tests, conformance smoke, benchmark harness | fuzz-like deterministic gates | sanitizer/CI policy hardening | unverifiable support claims |

## README/status link policy

Current-facing docs must link to this matrix when they make support claims. Historical `.omx/plans/` files may preserve older milestone intent, but current docs must not present Future or Experimental surfaces as Supported.

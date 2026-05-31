## Prometheus Strict Plan

### Target Result
- Clean the repo’s user-facing documentation and wording by reducing `README.md` to a minimal project description plus `docs/MASTER.md` link, consolidating all product docs into `docs/MASTER.md`, deleting the old `docs/**` product-doc files, and replacing milestone-coded public wording with logical behavior names in docs, diagnostics, manifests, and tests while leaving historical `.omx/plans` and `.omx/specs` artifacts unchanged.

### Clarified Requirements (Metis)
- README shape: short project name/description plus a link to `docs/MASTER.md`; no long quickstart unless the executor decides one line is needed for the link context.
- Docs final shape: `docs/` contains exactly one product documentation file, `docs/MASTER.md`.
- Old docs policy: remove old docs outright after content is consolidated or explicitly marked obsolete in a pre-delete inventory.
- Historical scope: leave `.omx/plans` and `.omx/specs` as workflow/audit history; they may retain milestone language and should be excluded from de-milestoning grep gates.
- Code/data scope: remove user-visible milestone labels from emitted diagnostics, conformance manifest schema/values, tests that assert diagnostic text, examples/output docs, and current product docs. Do not rename internal symbols solely because they are historical unless they are user-visible.
- Verification floor: full gates.

### Critique Resolved (Momus)
- Destructive deletion risk -> execution must create a docs consolidation ledger before deleting, mapping every current `docs/**` file to `migrated to docs/MASTER.md` or `obsolete`.
- Uncommitted work collision -> execution must start with `git status --short`, preserve current Zig/build/docs changes, avoid reset/checkout/clean, and report any pre-existing changed files it edits.
- Manifest schema risk -> `conformance/test262-subset-manifest.csv` and `scripts/test262-adapter.sh` must change atomically if the `milestone` column is renamed/removed.
- Diagnostic/test coupling -> source diagnostic replacement strings and test expectations must update together, with behavior-preserving tests.
- User-visible boundary ambiguity -> verification uses targeted grep/audit over README, docs, src diagnostics, tests, conformance manifests, examples, and scripts; `.omx/plans`/`.omx/specs` excluded.
- Hidden link breakage -> final link/path scan must prove no references to deleted docs remain outside excluded historical OMX artifacts.

### Oracle Execution Plan
1. **Preflight and ownership lane — single executor**
   - Record `git status --short` before edits.
   - Treat existing uncommitted Zig migration files as pre-existing user work to preserve, not cleanup noise.
   - Do not run destructive git commands (`reset`, `checkout`, `clean`) during execution.
   - If a pre-existing modified file must be edited (`README.md`, current docs, `.gitignore`, `Makefile`, `build.zig`), edit in place and report the overlap.

2. **Docs inventory and master-document lane — single executor**
   - Enumerate every current `docs/**/*.md` file before deletion.
   - Build a temporary consolidation ledger (can be in the final report or an OMX artifact) mapping each file to `migrated` or `obsolete`.
   - Create `docs/MASTER.md` with clear behavior-first headers, at minimum:
     - Overview and scope
     - Build, run, and verification
     - Supported language/runtime behavior
     - Diagnostics and unsupported behavior policy
     - Modules and backend behavior
     - Host API and embedding
     - Memory management and GC behavior
     - Bytecode/VM behavior
     - Regex/stdlib/async behavior
     - Conformance, benchmark, sanitizer, and quality gates
     - Public C API summary
     - Known unsupported/future behavior, named by behavior only
   - Consolidate current useful product information from old docs into those sections using behavior names rather than milestone IDs.
   - Do not copy historical milestone chronology or closure records into the master doc unless restated as current behavior.

3. **README simplification lane — single executor**
   - Replace README with a minimal description and a link to `docs/MASTER.md`.
   - Remove the existing long doc index and roadmap links.

4. **User-visible de-milestoning lane — single executor**
   - Replace emitted diagnostic strings in `src/parser.c`, `src/eval.c`, `src/compiler.c`, and `src/vm.c` so they describe behavior boundaries instead of labels like `L3`, `L4`, `L5`.
   - Suggested replacement pattern:
     - `unsupported in L4 classes` -> `unsupported in jsrt class declarations`
     - `only function.prototype is supported in L3` -> `only function.prototype is supported for constructor/prototype behavior`
     - `modules are only supported in L5/tree-walk` -> `modules are only supported by the tree-walk module evaluator` or equivalent behavior-first wording.
   - Update all tests asserting those strings (`tests/test_parser.c`, `tests/test_eval.c`, `tests/test_vm.c`, `tests/test_diagnostics.c`) in the same change.
   - Rename or replace the `milestone` column in `conformance/test262-subset-manifest.csv` with a behavior-first field such as `feature` or `area`, update rows, and update `scripts/test262-adapter.sh` positional parsing and validation accordingly.
   - Avoid broad internal renames unless required by the manifest schema or user-visible output.

5. **Delete old docs lane — single executor**
   - After `docs/MASTER.md` exists and the consolidation ledger is complete, delete every old `docs/**/*.md` file except `docs/MASTER.md`.
   - Remove empty `docs/production/` if no longer needed.
   - Update references from README/code/tests/scripts to old docs paths or remove them.

6. **Verification and cleanup lane — single executor**
   - Run full verification matrix below.
   - If a grep hit remains, classify it as one of: historical `.omx` excluded, false positive in generated/build output excluded, or blocker requiring edit.
   - Run final cleanup/deslop and code review before completion if this plan is executed via `$ultragoal` final story.

### Verification Matrix
| Claim | Required evidence | Owner/lane |
| --- | --- | --- |
| Pre-existing work was preserved | initial and final `git status --short`; no reset/checkout/clean commands used | Preflight |
| README is minimal | review `README.md`; contains project description and `docs/MASTER.md` link; no long docs index | README |
| Single master doc exists | `test -f docs/MASTER.md`; review headers/sections | Docs |
| Old product docs are removed | `find docs -type f | sort` returns only `docs/MASTER.md` | Docs deletion |
| Docs content was not silently lost | consolidation ledger maps every pre-existing `docs/**/*.md` file to migrated/obsolete | Docs inventory |
| No stale doc links remain | grep/path scan finds no references to deleted `docs/*.md` / `docs/production/*.md` outside excluded `.omx/plans` and `.omx/specs` | Docs/link audit |
| User-visible milestone names removed | grep audit over README, docs, src, include, tests, scripts, conformance, examples excludes `.omx/plans` and `.omx/specs` and finds no user-visible `milestone`, `roadmap`, `L1`-style, `M1`-style, `N8`, `P3`, `A4` labels | De-milestoning |
| Diagnostics still test same behavior | `zig build test` and targeted diagnostic/conformance tests pass after string updates | Code/tests |
| Test262 manifest schema remains valid | `zig build test262-adapter` passes | Manifest/script |
| Runtime/conformance behavior unchanged | `zig build verify-production`; `make verify-production` | Verification |
| Benchmark and sanitizer gates remain green | `zig build bench-smoke`; `make bench-smoke`; `zig build asan` | Verification |
| Formatting/shell checks pass | `git diff --check`; `zig fmt --check build.zig`; `bash -n scripts/*.sh` | Verification |

### Artifact
- Durable plan path: `.omx/plans/prometheus-strict/repo-docs-cleanup-demilestone.md`

### Handoff
- Recommended next workflow: `$ultragoal ".omx/plans/prometheus-strict/repo-docs-cleanup-demilestone.md"`
- `$team` is not recommended initially because docs consolidation, diagnostic string updates, test expectations, and manifest schema updates are tightly coupled and deletion-heavy.
- Stop condition: README is minimal, `docs/MASTER.md` is the only product doc under `docs/`, user-visible milestone naming is removed outside excluded historical OMX artifacts, all full gates pass, and final review is clean.
- Rollback/escalation condition: if consolidation cannot preserve essential current behavior docs in `docs/MASTER.md`, stop before deleting old docs; if diagnostics/manifest de-milestoning changes behavior rather than wording/schema only, revert that slice and report blockers.

### Clean-Room Credit
Inspired by OMO Prometheus (`code-yeongyu/oh-my-openagent`), reimplemented from concept under MIT.

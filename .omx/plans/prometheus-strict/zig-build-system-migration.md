## Prometheus Strict Plan

### Target Result
- Migrate `jsrt` so Zig 0.16.0+ `build.zig` is the authoritative build graph, while preserving the existing Make target interface as thin compatibility wrappers and preserving `./build/jsrt` plus `./build/test_jsrt` artifact paths.

### Clarified Requirements (Metis)
- Boundary: Zig becomes primary; Make remains as wrapper compatibility only.
- Artifact contract: `zig build`, `make all`, and test/conformance workflows must leave executable `./build/jsrt`; unit-test workflows must also leave executable `./build/test_jsrt`.
- Documentation scope: update README and current-facing docs such as `docs/project-status-and-architecture.md`, `docs/production/verification-targets.md`, `docs/production/release-checklist.md`, and `docs/benchmark-and-profiling-harness.md` as needed. Do not rewrite historical `.omx/plans` or milestone evidence records.
- Acceptance depth: full gates, including build, unit tests, conformance, Test262 manifest validation, VM parity, benchmark smoke, shell syntax checks, and sanitizer/ASan-equivalent lane where practical.
- Version floor: Zig 0.16.0+.
- Makefile fate: keep a thin Makefile delegating to Zig.

### Critique Resolved (Momus)
- Vague “Zig primary” -> define exact authoritative commands and wrapper target mapping.
- Zig output path mismatch -> explicitly install/copy Zig artifacts into `./build/jsrt` and `./build/test_jsrt`; tests/scripts currently hardcode `./build/jsrt`.
- C parity risk -> build.zig must encode C17, `-Wall -Wextra -Werror -pedantic`, `-Iinclude`, debug info, libc linkage, optional debug macro, and sanitizer lane.
- Test runtime dependency -> `zig build test` must build/copy `jsrt` before running `test_jsrt`.
- Production gate ambiguity -> verification matrix below names commands.
- Docs churn risk -> update current docs only; leave historical OMX artifacts intact.

### Oracle Execution Plan
1. **Build graph lane — `build.zig`**
   - Add `build.zig` using Zig 0.16 APIs: `b.standardTargetOptions(.{})`, `b.standardOptimizeOption(.{})`, `b.addExecutable(.{ .root_module = b.createModule(...) })`, module-level `addCSourceFiles`, `addIncludePath`, and `link_libc = true`.
   - Define shared C source lists for runtime library sources, CLI source (`src/main.c`), and test sources.
   - Encode default C flags equivalent to Make: `-std=c17`, `-Wall`, `-Wextra`, `-Werror`, `-pedantic`, and debug info via Zig Debug mode. Add `-DJSRT_DEBUG` on the `debug` step.
   - Provide a Zig version guard or clear failure if `builtin.zig_version < 0.16.0`.
   - Add steps: default/all, `debug`, `test`, `conformance`, `test262-adapter`, `vm-parity`, `verify-production`, `bench`, `bench-vm`, `bench-compare`, `bench-smoke`, `asan`/`sanitize` where Zig 0.16 supports it cleanly, and `clean` if practical.
   - Preserve `./build/jsrt` and `./build/test_jsrt` via install/copy/update-source step after compilation, not by changing tests/scripts first.
   - Ensure script run steps use existing scripts and forward required env vars/arguments: `BIN=./build/jsrt`, benchmark `ITERATIONS`, `WARMUPS`, `FIXTURE`, `FILTER`/`BACKEND` as applicable.

2. **Make wrapper lane — `Makefile`**
   - Replace independent compile/link rules with thin wrappers around `zig build` steps.
   - Preserve targets: `all`, `debug`, `test`, `conformance`, `test262-adapter`, `vm-parity`, `verify-production`, `bench`, `bench-vm`, `bench-compare`, `bench-smoke`, `clean`.
   - Preserve user knobs where they still apply: `ITERATIONS`, `WARMUPS`, `FIXTURE`, `FILTER`; document any unsupported legacy `CC`/`CFLAGS` passthrough. If feasible, map sanitizer use to `zig build asan` instead of requiring `CFLAGS=...`.

3. **Harness compatibility lane**
   - Keep existing shell scripts unchanged unless a minimal update is required for Zig invocation; prefer wrappers/env to script rewrites.
   - If any script assumes `build/` exists, create the directory in build steps before script execution.
   - Run `bash -n scripts/*.sh` after any script-affecting changes.

4. **Current docs lane**
   - Update README build/test sections to use Zig-primary commands and mention Make compatibility wrappers.
   - Update current production/verification docs to replace current recommended `make ... CFLAGS=...` ASan flow with the new Zig sanitizer lane, while preserving historical evidence logs unchanged.
   - Audit current-facing docs for stale “Make is primary” language.

5. **Integration validation lane — single `$ultragoal` executor**
   - Run the verification matrix. Fix build.zig/Make/docs until all mandatory gates pass or a carried-forward blocker is documented.
   - If Zig 0.16 sanitizer support does not provide AddressSanitizer parity on the local target, document the limitation and keep the old Make/CFLAGS sanitizer wrapper only as a temporary compatibility exception, but do not leave it as the primary path without noting the blocker.

### Verification Matrix
| Claim | Required evidence | Owner/lane |
| --- | --- | --- |
| Zig version is acceptable | `zig version` prints `0.16.0` or newer; build.zig guard rejects older versions | Build graph |
| `build.zig` is formatted | `zig fmt --check build.zig` | Build graph |
| Zig builds runtime binary at preserved path | `zig build`; `test -x ./build/jsrt`; direct `./build/jsrt examples/arithmetic.js` smoke | Build graph |
| Zig builds test binary and tests can shell out to runtime | `zig build test`; `test -x ./build/test_jsrt` | Build graph |
| Makefile is a thin Zig wrapper | review/grep shows normal build/test/bench targets delegate to `zig build` and independent `$(CC)`/`$(CFLAGS)` compile-link rules are removed, except any explicitly documented temporary sanitizer fallback | Make wrapper |
| Make wrapper preserves default build | `make clean && make all`; `test -x ./build/jsrt` | Make wrapper |
| Make wrapper preserves unit test target | `make test` | Make wrapper |
| Conformance wrapper still works | `make conformance` | Make wrapper/harness |
| Test262 manifest validator still works | `make test262-adapter` | Make wrapper/harness |
| VM parity wrapper still works | `make vm-parity` | Make wrapper/harness |
| Production composite remains green in Zig and Make wrapper paths | `zig build verify-production`; `make verify-production` | Build graph/Make wrapper |
| Benchmark smoke remains green in Zig and Make wrapper paths | `zig build bench-smoke`; `make bench-smoke` | Build graph/Make wrapper |
| Sanitizer lane exists or blocker documented | `zig build asan` or `zig build sanitize`; if unsupported, blocker note added to README and `docs/production/verification-targets.md` with temporary fallback command | Build graph/Docs |
| Shell scripts remain syntactically valid | `bash -n scripts/*.sh` | Harness |
| Current docs reflect Zig-primary workflow | grep/review README + current docs; no stale current recommendation of Make-primary build | Docs |

### Artifact
- Durable plan path: `.omx/plans/prometheus-strict/zig-build-system-migration.md`

### Handoff
- Recommended next workflow: `$ultragoal ".omx/plans/prometheus-strict/zig-build-system-migration.md"`
- `$team` is not recommended initially because `build.zig`, `Makefile`, README, and current docs are tightly coupled. A later docs-only lane can split after build behavior is green.
- Stop condition: all mandatory verification matrix gates pass, or the sanitizer lane is the only remaining blocker and is explicitly documented with a temporary fallback.
- Rollback condition: if Zig migration cannot preserve `./build/jsrt`/`./build/test_jsrt` and production gates in reasonable repair cycles, restore independent Makefile behavior and keep any useful build.zig work behind an alternate, non-authoritative path.

### Clean-Room Credit
Inspired by OMO Prometheus (`code-yeongyu/oh-my-openagent`), reimplemented from concept under MIT.

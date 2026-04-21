# ADR — M2 Aggregate Semantics Direction for jsrt

## Status
Accepted for roadmap handoff to M3.

## Decision
Adopt **shared-reference mutation semantics for arrays and objects** as the target direction for the runtime, while keeping the migration explicitly bounded:
- numbers, booleans, `null`, and strings remain value-like
- functions keep their current identity-preserving behavior
- arrays and objects should stop deep-cloning across environment read/write boundaries in M3
- property/index mutation should become visible through aliased bindings that point at the same aggregate
- current deep structural equality and stringification may remain unchanged during M3 unless migration evidence proves they must move too

This means **M3 is required**, not cancelled: it becomes the implementation milestone that replaces today's copy-isolated aggregate behavior with shared aggregate identity.

## Why this synthesis
The runtime already paid the hardest prerequisite cost in N7/N8/M1:
- arrays/objects now live on the bounded managed heap
- the collector has explicit root handling and observability
- the repo has a frozen semantics/ownership baseline that clearly documents the current copy model

That makes M2 the right place to resolve the semantic question directly instead of carrying an intentionally non-JS-like aggregate model deeper into built-ins, backend planning, and future host/runtime work.

## Evidence-backed current state
### Current aggregate semantics are intentionally copy-isolated
The current runtime clones arrays/objects at environment boundaries:
- `env_define` stores `value_clone(value)` in `src/env.c`
- `env_assign` replaces bindings with `value_clone(value)` in `src/env.c`
- `env_get` returns `value_clone(entry->value)` in `src/env.c`
- `value_clone` deep-copies arrays/objects in `src/value.c`

### Current tests lock in alias isolation today
`tests/test_eval.c` explicitly proves that alias mutation does not affect the original binding:
- `let arr = [1, 2]; let alias = arr; alias[0] = 99; arr[0];` still evaluates to `1`

### Assignment already carries the smell M3 should remove
`src/eval.c` currently:
1. reads the root binding via `env_get`
2. mutates the cloned root along an assignment path
3. writes the whole root back with `env_assign`

That is correct for today's semantics, but it is an awkward fit for future built-ins, native interop, and backend work where aggregate identity matters.

## Options considered

### 1. Keep copy-isolated aggregate semantics long-term
**Pros**
- smallest short-term risk
- matches today's docs/tests exactly
- keeps mutation reasoning local to one binding

**Cons**
- diverges sharply from user expectations for array/object aliasing
- complicates built-ins/native APIs that should observe shared mutation
- keeps env/value cloning on the hot path
- forces future VM/backend work to preserve a bespoke aggregate model

### 2. Move arrays/objects to shared-reference semantics **(chosen)**
**Pros**
- better matches JavaScript-style aggregate intuition without requiring full ECMAScript scope
- removes the biggest semantic mismatch between functions and aggregates
- simplifies later built-ins/host interop design
- gives M6 backend planning a cleaner value model

**Cons**
- breaks current alias-isolation tests/examples
- requires careful GC/root/ownership review during M3
- may surface follow-on questions about equality and diagnostics

### 3. Hybrid model: keep env copies, but add special aliasing rules in selected paths
**Pros**
- could preserve some old tests while enabling a few reference-like features
- smaller diff than a full aggregate-semantic migration

**Cons**
- conceptually muddy
- harder to teach than either clean copy semantics or clean reference semantics
- risks baking in exceptions that M6 and built-ins must work around later

## Compatibility matrix

| Concern | Current copy-isolated model | Chosen reference-oriented direction | M3 impact |
|---|---|---|---|
| `let alias = arr` | alias receives a deep clone | alias points at same aggregate | breaking semantic change |
| nested object/array mutation | visible only through the rewritten root binding | visible through every alias sharing that aggregate | breaking semantic change |
| env reads/writes | clone arrays/objects on get/set | preserve aggregate identity on get/set | core runtime rewrite |
| function arguments / returns | aggregate values tend to de-alias at env boundaries | aggregate identity should survive ordinary binding flow | behavior expansion |
| property/index assignment | read-clone-mutate-write-root-back | mutate shared aggregate in place | evaluator simplification opportunity |
| equality (`==`) | deep structural for arrays/objects | keep structural equality initially unless M3 proves otherwise | intentionally deferred sub-decision |
| GC scope | bounded managed heap for env/function/array/object | unchanged GC scope | no new GC object families |
| docs/examples | describe copy isolation | must describe alias-visible mutation | targeted updates required |

## Adoption criteria
Proceed with M3 only if all of the following remain true:
1. arrays/objects can share identity without widening GC beyond `Env`, `JsFunction`, `JsArray`, and `JsObject`
2. evaluator temporary-root handling remains explicit and bounded
3. the migration can keep strings/source/tokens/AST ownership rules unchanged
4. the breakage can be captured in a narrow, explicit test/doc update set rather than a repo-wide semantic blur
5. M6/backend planning clearly benefits from the simplified aggregate model

## Abort criteria
Stop or defer M3 if any of the following become true:
1. preserving aggregate identity requires a much broader handle/refcount system than the current runtime can absorb safely
2. function-call, return-value, or temporary-expression paths require invasive redesign beyond the aggregate model itself
3. equality, diagnostics, and built-in groundwork all need to change at once to keep the runtime coherent
4. observability evidence shows alias-preserving aggregates destabilize the current bounded GC/root model

## Rollback path
If M3 begins but becomes too risky:
- restore env/value aggregate cloning at environment boundaries
- keep current assignment-path rewrite behavior
- retain the current semantics baseline as the active runtime contract
- record the failed migration boundary explicitly and defer shared-reference semantics to a later redesign milestone

The rollback boundary is intentionally clean because M2 itself does not change runtime code; it records the decision and the stop conditions before implementation starts.

## Required M3 scope
M3 should treat these as in-scope by default:
1. env/value aggregate identity preservation
2. alias-visible property/index mutation
3. regression updates for aliasing/nested mutation behavior
4. docs/examples/baseline updates describing the new aggregate model

M3 should treat these as explicitly out of scope unless migration evidence forces escalation:
- new GC object families
- string interning or shared-string identity
- ECMAScript-style `===`
- prototype/class semantics
- built-in library surface expansion beyond what is needed to validate the aggregate model

## Downstream impact
### Unlocks
- M3 mutable aggregate/reference implementation
- M4 built-ins on a less surprising aggregate model
- M6 backend/IR planning on a value model with stable aggregate identity

### Defers
- equality redesign beyond current deep structural behavior
- any move toward broader ECMAScript compatibility
- any expansion of GC scope beyond the current bounded runtime objects

## Directive for M3
Treat this ADR as the source of truth:
- **target semantics:** shared-reference arrays/objects
- **delivery style:** bounded migration focused on env/value/evaluator aggregate handling
- **guardrails:** no GC-scope widening, no frontend ownership rewrite, no opportunistic JS-compatibility sprawl

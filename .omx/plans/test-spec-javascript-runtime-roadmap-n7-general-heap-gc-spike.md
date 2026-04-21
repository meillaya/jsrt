# Test Spec — Roadmap N7: General Heap GC Architecture Spike

## Coverage Goals
- the spike is grounded in the current runtime structure rather than generic GC advice
- option comparison includes at least the three required directions
- root/ownership inventory is concrete enough to guide implementation
- adoption/abort/rollback/unlock decisions are explicit
- the artifact clearly prepares N8 without silently implementing it

## Expected Evidence
- references to current runtime ownership/control points (for example `value`, `env`, `gc`, evaluator cleanup boundaries)
- a written ADR or equivalent decision record
- a migration-phase breakdown
- explicit downstream milestone impacts

## Verification Commands / Checks
This is a planning spike, so verification is artifact-centric:
```sh
sed -n '1,260p' .omx/plans/prd-javascript-runtime-roadmap-n7-general-heap-gc-spike.md
sed -n '1,260p' .omx/plans/test-spec-javascript-runtime-roadmap-n7-general-heap-gc-spike.md
```

Recommended grounding checks before approval:
```sh
grep -R "value_free\|value_release_reference\|env_destroy\|gc_collect\|env_mark_reachable\|function_sweep_unreachable" -n src include
```

## Notes for Execution
- if external GC strategy guidance is needed, gather it explicitly and distinguish it from repo-local facts
- if the spike concludes “defer broader GC,” that is still a valid successful outcome if justified
- do not let N7 drift into N8 implementation work

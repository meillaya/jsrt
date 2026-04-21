# Test Spec — Roadmap M2: Aggregate Semantics ADR (`copy` vs `reference`)

## Coverage Goals
- the ADR matches the current runtime's actual aggregate behavior
- the compatibility matrix names the concrete user-visible behaviors that would change under M3
- adoption / abort / rollback conditions are specific enough to guide implementation stop/go decisions
- repo-facing docs point to the new M2 artifact set without claiming semantics have already changed

## Expected Evidence
- artifact review against `docs/semantics-baseline.md`
- artifact review against `docs/ownership-guardrails.md`
- artifact review against `src/env.c`, `src/value.c`, and `src/eval.c`
- artifact review against aggregate-oriented tests in `tests/test_eval.c`
- fresh `make clean && make test`
- fresh example runs covering current aggregate behavior

## Verification Commands
```sh
make clean && make test
./build/jsrt examples/arrays_objects.js
./build/jsrt examples/assignment.js
```

## Notes for Execution
- do not silently change the semantics baseline during M2; record future-facing changes as M3 impacts instead
- if the chosen direction requires widening GC scope or redesigning equality immediately, stop and report against the ADR abort criteria
- keep evidence-vs-inference boundaries explicit in the ADR and companion docs

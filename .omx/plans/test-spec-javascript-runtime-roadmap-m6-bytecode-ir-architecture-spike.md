# Test Spec — Roadmap M6: Bytecode / IR Architecture Spike

## Coverage Goals
- the ADR and companion docs match the current repo's frontend/runtime structure
- the first backend subset is explicit enough to bound M7 implementation
- adoption/abort/rollback criteria are specific enough to govern the go/no-go gate
- repo-facing docs point future work at the M6 artifact set without claiming a VM/compiler already exists

## Expected Evidence
- artifact review against `include/jsrt/ast.h`
- artifact review against `include/jsrt/eval.h`
- artifact review against current runtime/baseline docs
- fresh `make clean && make test`

## Verification Commands
```sh
make clean && make test
```

## Notes for Execution
- keep M6 documentation-only
- clearly separate the chosen backend target from the current implemented runtime
- if the planned subset is too small to justify M7 or too large to implement safely, stop and revise the M6 ADR before proceeding

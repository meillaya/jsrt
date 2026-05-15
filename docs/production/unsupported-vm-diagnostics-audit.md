# Unsupported VM Diagnostics Audit (V2)

Status: accepted on 2026-05-15.

The VM backend must reject unsupported surfaces deterministically. It must not silently fall back to tree-walk execution, partially evaluate unsupported syntax, or produce misleading output.

Current retained audit cases:

- module syntax under `JSRT_BACKEND=vm` fails with the compiler diagnostic `modules are supported by the tree-walk backend only`
- non-VM fixtures stay out of `conformance/vm-parity-manifest.csv` until implemented
- supported VM fixtures are covered by `make vm-parity`

Audit commands:

```sh
make conformance
make vm-parity
```

Any future unsupported VM surface must add a negative conformance fixture or unit test before the related milestone can close.

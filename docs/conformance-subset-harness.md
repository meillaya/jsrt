# L9 Conformance Subset Harness

This document records the bounded **L9 — Conformance subset harness**.

## Purpose

The harness is repo-local and intentionally small. It is not Test262 and does not imply broad ECMAScript compatibility.

## Layout

```text
conformance/manifest.csv
conformance/positive/*.js
conformance/negative/*.js
scripts/conformance.sh
```

## Manifest schema

```csv
id,path,backends,expect,stdout,stderr_contains
```

- `id`: stable case identifier.
- `path`: fixture path relative to the repository root.
- `backends`: `tree`, `vm`, or `both`.
- `expect`: `success` or `failure`.
- `stdout`: exact expected stdout for successful cases, without trailing newline.
- `stderr_contains`: substring required in stderr for failure cases.

## Running

```sh
make conformance
```

The script writes per-case stdout/stderr artifacts under `build/conformance/`.

## Current coverage

- positive arithmetic on both backends
- positive module import on tree-walk backend
- negative const assignment on tree-walk backend
- negative division-by-zero on both backends
- negative unexported import on tree-walk backend

## Adding cases

1. Add a fixture under `conformance/positive/` or `conformance/negative/`.
2. Add one manifest row.
3. Choose the narrowest backend selector that matches supported behavior.
4. Prefer stable stdout or diagnostic substrings over whole diagnostic snapshots.

# Module Production Status (M2-M6)

Status: accepted on 2026-05-15.

## M2 — Module cache / path / cycle diagnostics

The tree-walk backend has the current production module implementation. It resolves relative paths from the importing module path, caches module records by resolved path, rejects unsupported cyclic imports, reports lex/parse/load failures with module context, and rejects missing exports deterministically.

Retained evidence:

- `tests/test_eval.c` module import/export and missing export cases
- `conformance/positive/module_main.js`
- `conformance/negative/import_hidden.js`
- `conformance/manifest.csv` positive and negative module rows
- `docs/module-system.md`

## M3-M6 — VM module representation, import bindings, diagnostics, and parity gate

The production decision is to keep modules tree-walk-supported and VM-rejected until an explicit linker/export-table implementation lands. The current milestone closure artifact is therefore a deferral gate, not a VM support claim.

Completion criteria for a future implementation:

1. compiler bytecode representation for module initializer and export table,
2. VM import binding resolution against linked module records,
3. module parser/evaluator/compiler diagnostics with source-path context,
4. `make vm-parity` module fixtures promoted from negative diagnostics into parity manifest,
5. `make conformance` module cases passing on `both` backends.

Current retained gate:

- `negative-module-vm` in `conformance/manifest.csv` requires deterministic VM rejection for module syntax.
- `docs/production/unsupported-vm-diagnostics-audit.md` forbids silent fallback.

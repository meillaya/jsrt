# L5 Module System

This document records the bounded **L5 — Module system** surface implemented in `jsrt`.

## Supported syntax

L5 supports static named imports from string-literal module specifiers:

```js
import { name, exported as localName } from "./module.js";
```

L5 supports declaration exports only:

```js
export const answer = 42;
export let counter = 0;
export function read() { return answer; }
export class Box { get() { return answer; } }
```

`export { name }`, default exports, namespace imports, side-effect-only imports, dynamic `import()`, package resolution, and circular module graphs are intentionally unsupported.

## Resolution and execution model

- Module specifiers are plain file paths.
- Relative specifiers resolve against the importing file's directory.
- Imported bindings are defined as `const` bindings in the importing module/script.
- Modules execute in a fresh top-level environment with the normal built-ins.
- Only names declared with `export` are importable.
- Module evaluation is tree-walk only in L5. `JSRT_BACKEND=vm` rejects module syntax with an explicit compiler diagnostic.
- Circular imports are detected and rejected as unsupported.

The module loader currently keeps module artifacts alive for the duration of the evaluation result so imported functions can safely reference their module AST/environment while the top-level program runs.

## Examples

```sh
./build/jsrt examples/modules.js
```

Expected output:

```text
82
```

## Verification evidence

Current L5 coverage lives in:

- `tests/test_lexer.c` — `import` / `export` / `from` / `as` tokens
- `tests/test_parser.c` — import specifiers, aliases, and export declaration wrappers
- `tests/test_eval.c` — relative module loading, exported constants/functions, alias imports, and rejection of unexported names
- `examples/module_math.js` and `examples/modules.js` — CLI-level module smoke example

Retained verification for the L5 closure:

- `make clean && make test` → `Tests passed: 1062 assertions`
- `./build/jsrt examples/modules.js` → `82`

## Boundaries and risks

- L5 does not claim ECMAScript module completeness.
- There is no package resolver or module cache invalidation contract.
- VM backend support is intentionally deferred until a later backend-widening milestone chooses to lower module syntax.
- Cyclic imports are rejected instead of partially initialized.

# Built-in Library Foundation

This note is the execution-facing companion for roadmap milestone **M4 — built-in library foundation**.

## Current built-ins

The runtime now exposes three global built-ins:

- `print(value)` — writes the value's string form plus a newline to stdout, then returns `null`
- `len(value)` — returns:
  - string length for strings
  - element count for arrays
  - property count for objects
- `typeOf(value)` — returns one of:
  - `number`
  - `bool`
  - `null`
  - `string`
  - `function`
  - `array`
  - `object`

## Guardrails

M4 intentionally stays narrow:
- built-ins are fixed-arity global functions
- there is no broader host-embedding API yet
- `print` is the only built-in with direct observable I/O
- misuse still reports source-based runtime diagnostics

## Example

```js
print(typeOf({name: "jsrt"}) + ":" + len("hello") + ":" + len([1, 2, 3]));
```

Expected output:

```text
object:5:3
```

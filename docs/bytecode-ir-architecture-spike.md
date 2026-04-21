# Bytecode / IR Architecture Spike

This note is the execution-facing companion for roadmap milestone **M6 — bytecode / IR architecture spike**.

## Decision summary

The repo now chooses a **stack-based bytecode VM** as the first compiled-backend experiment.

Important guardrail:
- the existing tree-walk evaluator remains the semantic authority until VM parity is explicitly demonstrated

## Initial M7/M8 target subset

### Included first
- literals and basic values
- `let` / `const`
- arithmetic / comparison
- logical operators
- blocks
- `if` / `else`
- `while`
- `for`
- non-capturing function declarations/calls/returns
- built-in calls (`print`, `len`, `typeOf`)

### Deferred initially
- closures / upvalues
- full aggregate mutation parity
- `switch`
- `do...while`
- full current-language parity

## Draft instruction families

Value/stack:
- `CONST`
- `NULL`
- `TRUE`
- `FALSE`
- `POP`
- `DUP`

Variables:
- `LOAD_LOCAL`
- `STORE_LOCAL`
- `LOAD_GLOBAL`
- `STORE_GLOBAL`
- `DEFINE_GLOBAL`

Arithmetic/comparison:
- `NEG`
- `NOT`
- `ADD`
- `SUB`
- `MUL`
- `DIV`
- `EQ`
- `NEQ`
- `LT`
- `LTE`
- `GT`
- `GTE`

Control/calls:
- `JUMP`
- `JUMP_IF_FALSE`
- `LOOP`
- `CALL`
- `RETURN`
- `MAKE_FUNCTION`

## AST-to-bytecode mapping samples

### Variable declaration

Source:

```js
let x = 3;
```

Sample lowering:

```text
CONST 3
DEFINE_GLOBAL x
```

### Conditional

Source:

```js
if (x < 10) { x = x + 1; } else { x = 0; }
```

Sample lowering sketch:

```text
LOAD_GLOBAL x
CONST 10
LT
JUMP_IF_FALSE else_label
LOAD_GLOBAL x
CONST 1
ADD
STORE_GLOBAL x
JUMP end_label
else_label:
CONST 0
STORE_GLOBAL x
end_label:
```

### Function call

Source:

```js
twice(x);
```

Sample lowering:

```text
LOAD_GLOBAL twice
LOAD_GLOBAL x
CALL 1
POP
```

## Coverage matrix for the first backend target

| Current feature | M7/M8 initial plan |
|---|---|
| literals | included |
| arithmetic/comparison | included |
| `let` / `const` | included |
| `if` / `else` | included |
| `while` / `for` | included |
| built-ins | included |
| non-capturing calls/returns | included |
| closures/upvalues | deferred |
| `switch` | deferred |
| `do...while` | deferred |
| aggregate mutation parity | deferred |

## Go/no-go summary

Proceed to M7 because:
- the backend target is now explicit
- the subset is small enough to be implementable
- the tree-walk evaluator can remain the oracle

Stop and revisit if:
- closures/upvalues become required immediately
- backend work starts forcing full-language parity too early
- the VM cannot stay additive to the current runtime

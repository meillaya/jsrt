# jsrt reference

jsrt is a small JavaScript-like runtime written in C. The build produces a command line interpreter (CLI) and a static library, and the library provides a public C API. The tree-walk evaluator defines the language semantics, and the bytecode VM runs a tested subset of the same language.

This reference covers version 0.2.0: the command line, the language, the runtime services, and the build.

## Command line

```text
jsrt [options] [file]
```

jsrt runs `file` as the top-level module, evaluates the code passed with `-e`, or reads standard input. When standard input is a terminal and the command line names no file or code, jsrt starts a REPL. The argument `-` reads standard input explicitly.

The CLI prints the value of the final statement when that value is not null. Exit status 0 means the run completed, 1 means a script failed or an input could not be read, and 2 means a command line misuse.

| Option | Effect |
| --- | --- |
| `-e, --eval CODE` | Evaluate CODE and exit. |
| `--eval=CODE` | Same as `-e CODE`. |
| `--vm` | Run on the bytecode VM backend. |
| `-h, --help` | Print usage to stdout and exit 0. |
| `-V, --version` | Print `jsrt 0.2.0` and exit 0. |

| Variable | Effect |
| --- | --- |
| `JSRT_BACKEND=vm` | Select the VM backend, the same as `--vm`. |
| `JSRT_GC_TRACE=1` | Write one collector trace line per collection to stderr. |

The REPL keeps one environment for the session. It prompts with `jsrt> ` and continues an unfinished entry with `  ... `. The commands `quit`, `exit`, `.quit`, `.exit`, `:quit`, and `:exit` end the session. The REPL always uses the tree-walk evaluator; on a terminal, `--vm` prints a note to stderr and then starts the tree-walk REPL.

## Language scope

The parser accepts the forms in this table. Unsupported syntax stops at the lexer, the parser, or the VM compiler.

| Form | Syntax | Behavior |
| --- | --- | --- |
| Values | numbers, `true`, `false`, `null`, `"text"`, `'text'` | Numbers are 64-bit floating point values. Strings use single or double quotes and accept the escapes `\n`, `\t`, `\r`, `\b`, `\f`, `\v`, `\\`, `\"`, `\'`, `\xHH`, and `\uHHHH`. An unknown escape keeps its character. NUL bytes and surrogate escapes are rejected. |
| Comments | `// text`, `/* text */` | Line and block comments. An unterminated block comment is a lexer error. |
| Declarations | `let name;`, `let name = expr;`, `const name = expr;` | `const` bindings reject assignment. Redeclaring a name in one scope is an error. |
| Assignment | `name = expr;` | The target reference and any index expressions are evaluated before the right side. The target must be a declared binding. |
| Blocks | `{ ... }` | A block opens a lexical scope. |
| Conditionals | `if (condition) statement` with an optional `else statement` | Truthiness follows the rules in Values and equality. |
| Loops | `while (condition) statement`, `for (init; condition; step) statement`, `do statement while (condition);` | `break` and `continue` work inside these loops. A `for (let ...)` loop binds a fresh variable for each iteration. |
| Switch | `switch (expr) { case value: ... default: ... }` | Cases fall through until `break` or the end of the switch. |
| Functions | declarations, expressions, `return` | Functions close over their defining environment and support recursion. Calls check arity. |
| `this` | method calls, constructor calls | A method call binds `this` to the receiver. A call on a bare name binds `null`. |
| Arrays | `[expr, ...]`, `array[index]` | Arrays have a fixed length. Indexes are numbers inside the length. Trailing commas are allowed. |
| Objects | `{key: expr, ...}`, `object.key`, `object["key"]` | Keys are strings, and reserved words are valid keys and property names. Duplicate keys keep the last value, and `len` counts unique keys. Reading a missing key raises a lookup error. Trailing commas are allowed. |
| Prototypes | `Constructor.prototype`, `setPrototypeOf(object, prototype)` | Property reads follow the prototype chain. Property writes create own keys. |
| Classes | `class Name { constructor(...) { ... } method(...) { ... } }` | Methods install on the prototype. See Functions, constructors, prototypes, and classes. |
| Modules | `import { name, name as alias } from "path.js";`, `export const`, `export function` | Static named imports and declaration exports. Tree-walk backend only, top level only. An import runs where it appears, so imports are not hoisted. |
| Semicolons | `;` | Required between statements. The Limits section lists the two insertion points. |

The parser accepts `+`, `-`, `*`, `/`, unary `-`, the comparisons `<`, `<=`, `>`, `>=`, the equality operators `==` and `!=`, and the logical operators `&&`, `||`, and `!`. Addition adds two numbers and concatenates otherwise. Comparisons accept numbers. The logical operators return one operand, as JavaScript does. The parser rejects `%`, bitwise operators, `++` and `--`, compound assignment, the ternary conditional, `===` and `!==`, `var`, arrow functions, `try`, spread syntax, destructuring, `for...of`, and `for...in`.

## Values and equality

| Kind | Representation | Equality |
| --- | --- | --- |
| number | 64-bit floating point | Compares by value. |
| bool | `true` or `false` | Compares by value. |
| null | the single null value | Equals null. |
| string | byte sequence | Compares by contents. |
| function | declaration or expression plus closure | Compares by identity. |
| array | fixed-length value list | Compares elements in order. |
| object | string-key map plus prototype | Compares own keys and values. |

Arrays and objects have reference semantics. Assigning an aggregate or passing it as an argument copies the reference, so mutation through one reference is visible through the other. Equality is a separate operation: `==` on two arrays compares elements, and `[1, 2] == [1, 2]` is true. A cycle guard keeps cyclic values comparable, so two cycles of the same shape compare equal. Numbers compare exactly, so `0.1 + 0.2 == 0.3` is false. The runtime has no NaN value: zero divided by zero raises a math error.

`false`, `null`, `0`, and the empty string are falsy. Every other value is truthy, including nonzero numbers, empty arrays, and empty objects.

Stringification produces the display form that `print` and the CLI use. Numbers print with 15 significant digits, booleans print as `true` and `false`, null prints as `null`, strings print without quotes, arrays print as `[1, 2]`, objects print as `{name: value}`, and functions print as `<function name>`. A repeated aggregate prints as `<cycle>`. The CLI adds a newline when it prints the final value.

Both structural operations are bounded. Equality compares shared subgraphs once, so a diamond shaped value compares in time linear in its distinct nodes. Equality past 512 levels of nesting returns false. Stringification cuts off past 512 levels with `<depth limit>` and past one mebibyte of output with `<output limit>`.

## Functions, constructors, prototypes, and classes

Function values close over the environment where they appear. A call creates a child environment, binds the parameters, and compares the argument count with the declared arity; a mismatch raises an arity error. A declaration takes effect at the statement where it appears, so a call before the declaration fails with an undefined name error.

`this` carries the receiver for method calls and the new object for constructor calls. A call on a bare function name sets `this` to `null`.

`new C(args)` needs parentheses around the argument list. It creates an object whose prototype is `C.prototype`, runs the function with that object as `this`, and returns the object. Assigning a value to a function's `prototype` property replaces the method set that new instances see. The only readable property on a function is `prototype`; other property reads raise a type error.

`setPrototypeOf(object, prototype)` sets the explicit prototype link. The receiver must be an object, and the prototype must be an object or `null`. A self link or a cycle raises an assignment error. Reads follow the chain and writes land on the receiver, so a write to an inherited name shadows the prototype value.

Class declarations lower to a constructor function with methods on its prototype. An instance method and the class prototype property hold the same function value, so `box.get == Box.prototype.get` is true. Class syntax covers a constructor and instance methods. `extends`, `super`, static members, class fields, and class expressions fail to parse.

## Modules

```js
import { base, addBase as add } from "./module_math.js";

export const answer = 42;

export function addOne(n) {
    return n + 1;
}
```

An import declaration runs at the statement where it appears, so an import after a use of the name fails with an undefined name error. An import binds each named export into the importing environment as a const binding. The binding holds a snapshot of the exported value, so a later change to the module binding does not update the import. Assigning to the imported name raises an assignment error.

A specifier that starts with `/` is used as an absolute path. Any other specifier resolves against the directory of the importing file. The specifier includes the extension; jsrt does not add one. For `-e` and standard input, relative imports resolve against the current working directory.

The loader raises a lookup error when it cannot read a module or when the module does not export the requested name. Relative specifiers are normalized before the loader compares module records, so a cyclic import fails with `Runtime error [internal] ... cyclic module import is unsupported`. Static module declarations work at the top level of the tree-walk backend. The VM rejects module syntax with a compiler error.

Module records are cached for one top-level eval call, so a module imported twice in one eval runs once. The next eval call re-reads and re-evaluates each module it imports. Imported functions stay callable in later eval calls because their environments keep the parsed module program alive. Top-level declarations persist in the runtime environment, so a second eval that declares or imports the same local name fails with a redeclaration error. Evaluate one entry script per runtime.

## Built-in functions

Builtins live in the global environment. Calls enforce arity. The tree-walk backend defines all nine; the VM defines the first four.

| Builtin | Signature | Backends | Behavior |
| --- | --- | --- | --- |
| `print` | `print(value)` | tree, vm | Writes the display form plus a newline to stdout and returns null. |
| `len` | `len(value)` | tree, vm | Returns the byte length of a string, the element count of an array, or the key count of an object. |
| `typeOf` | `typeOf(value)` | tree, vm | Returns `number`, `bool`, `null`, `string`, `function`, `array`, or `object`. |
| `setPrototypeOf` | `setPrototypeOf(object, prototype)` | tree, vm | Sets the prototype link and returns the receiver. |
| `defer` | `defer(callback)` | tree | Appends a callback to the job queue and returns null. |
| `runJobs` | `runJobs()` | tree | Runs every queued callback and returns null. |
| `strContains` | `strContains(text, needle)` | tree | Returns true when text contains needle. |
| `strSlice` | `strSlice(text, start, end)` | tree | Returns the slice from start to end, excluding end, using byte offsets. |
| `regexMatch` | `regexMatch(text, pattern)` | tree | Returns true when pattern matches text. |

`len` accepts strings, arrays, and objects; other argument kinds raise a type error. `strContains` and `strSlice` require string arguments. `strSlice` bounds are `0 <= start <= end <= byte length`, and an out-of-range call raises an index error.

## Jobs

`defer(callback)` appends a callback to a job queue. `runJobs()` pops from the front and calls each callback with `this` set to null and no arguments. Declare callbacks without parameters, since a callback that expects arguments fails the arity check.

A queue lives for one eval call. The evaluator frees the queue when the top-level call returns, so a job that `runJobs` never reaches is dropped. A job runs only when `runJobs` reaches it. jsrt has no event loop.

## Regular expressions

`regexMatch(text, pattern)` returns true when the pattern matches a substring of the text. The matcher supports literal bytes, `.` for any one byte, character classes such as `[a-c]` and `[^x]`, the classes `\d`, `\D`, `\w`, `\W`, `\s`, and `\S`, the quantifiers `*`, `+`, and `?`, and the `^` and `$` anchors. All matching works on bytes.

Groups, alternation, and counted repetition are rejected with a runtime error whose message starts `unsupported regex pattern:`. A malformed pattern such as `[` or a quantifier with no preceding atom raises the same error. Patterns longer than 1024 bytes are rejected as unsupported. The matcher also counts its backtracking steps; a pattern that would run past the budget stops with `regex pattern is too complex for the bounded matcher`.

The string lexer consumes backslashes before the matcher sees the pattern, so a source string needs `"\\d"` to produce the pattern `\d`. Both arguments must be strings. The function keeps no state between calls.

## Diagnostics

Diagnostics go to stderr. A source diagnostic names the phase and position, then prints the offending source line with a caret under the token or expression:

```text
Parser error at 1:9: expected expression
let x = ;
        ^
```

The phases use these formats:

| Format | Source |
| --- | --- |
| `Lexer error at L:C: message` | Invalid characters and malformed string literals. |
| `Parser error at L:C: message` | Syntax errors and unsupported syntax. |
| `Compiler error at L:C: message` | Constructs the VM lowering rejects. |
| `Runtime error [kind] at L:C: message` | Evaluation failures. |
| `jsrt: message` | CLI failures such as an unreadable file or an unknown option. |

The line and column are 1-based. The caret run spans the construct that failed, so its width varies.

Runtime error kinds:

| Kind | Meaning |
| --- | --- |
| `none` | No runtime error; lexer and parser failures use this kind. |
| `type` | Wrong operand or argument type. |
| `name` | Undefined variable or redeclaration. |
| `arity` | Wrong argument count. |
| `lookup` | Missing object key or unreadable module. |
| `assignment` | Assignment to a const binding or an invalid prototype link. |
| `control-flow` | `return`, `break`, or `continue` outside its context, or an import outside the top level. |
| `index` | Array index or slice range outside its bounds. |
| `math` | Division by zero. |
| `internal` | A limit or host condition, including call depth, a cyclic import, and an interrupted run. |

## Limits

| Limit | Value | Behavior |
| --- | --- | --- |
| Semicolon insertion | end of file, or directly before `}` | A missing semicolon anywhere else is a parser error. |
| User call depth | 256 (`JSRT_MAX_CALL_DEPTH` in `include/jsrt/eval.h`) | `Runtime error [internal] ... maximum call depth exceeded`, exit 1, on both backends. |
| Parser nesting | 200 (`JSRT_MAX_PARSE_DEPTH` in `include/jsrt/parser.h`) | `Parser error ... source nesting is too deep`. |
| Expression chain | 64 operators (`JSRT_MAX_EXPR_CHAIN` in `include/jsrt/parser.h`) | A longer binary, member, or call chain is rejected as `expression chain is too long`. |
| Parsed tree depth | 512 (`JSRT_MAX_AST_DEPTH` in `include/jsrt/parser.h`) | A tree deeper than this is rejected after parsing as `program nesting is too deep`. |
| Parsed tree size | 400000 nodes (`JSRT_MAX_AST_NODES`) | A larger program is rejected as `program is too large`. |
| Parser diagnostics | 10 errors (`JSRT_MAX_PARSE_ERRORS`) | Further errors are dropped, and the message ends with a stopping note. |
| Value nesting | 512 (`JSRT_MAX_VALUE_DEPTH` in `src/value.c`) | Structural equality past this depth returns false, and printing writes `<depth limit>`. |
| Printed value size | 1 MiB | Printing writes `<output limit>` when the display form passes this size. |
| Regex pattern | 1024 bytes, 2000000 match steps | A longer pattern is rejected as unsupported, and an exhausted step budget reports `regex pattern is too complex for the bounded matcher`. |
| Array indexes | 0 through length minus 1 | A read or write outside the range raises an index error. |
| `strSlice` range | 0 through the byte length | An invalid range raises an index error. |
| Job lifetime | one eval call | The queue is freed when the top-level call returns. |
| Module lifetime | one top-level eval call | Modules are re-read on the next call. |

## Memory and garbage collection

The runtime allocates environments, functions, arrays, and objects from a managed heap. Each object carries a `GcHeader` and joins a process-global registry. The collector marks from the runtime environments, the active evaluator environments, and temporary roots, then sweeps the unreachable objects and releases their storage.

Collections run at safe points between statements, when a runtime is destroyed, and at process cleanup. `JSRT_GC_TRACE=1` writes one line per collection to stderr:

```text
[gc] reason=eval-safe-point collections=2 live(env=1,function=9,array=1,object=0) swept(env=0,function=0,array=0,object=0) temp_roots(current=0,high_water=1)
```

Lower-level code that holds a `Value` or `Env` across an allocation protects it with `gc_temp_root_push_value` or `gc_temp_root_push_env`, then releases it with `gc_temp_root_pop`. The evaluator and the runtime wrapper already follow this rule.

The collector publishes counters through `include/jsrt/gc.h`. `gc_get_stats` fills a `GcStatsSnapshot`, `gc_reset_stats` clears the counters, `gc_live_env_count` and its siblings report current live counts, and `gc_set_trace_enabled` switches tracing.

A parsed program belongs to the environment that evaluated it. The environment releases the program when it is destroyed or swept, and closures keep their function bodies valid in the meantime.

Collector state and module records are process-global, so eval calls are not thread-safe.

## Bytecode VM

The bytecode compiler lowers the VM subset to bytecode, and the stack VM executes that bytecode. Select it with `--vm` or `JSRT_BACKEND=vm`.

The VM runs `if` and `else` statements, function declarations, nested functions, classes, `new`, and `this`. Closures work when they only read captured variables. Assigning to a captured variable from a nested function fails at compile time with `Compiler error ... the VM backend does not support assigning to a captured variable`. A named function expression cannot refer to its own name inside the body, and the name resolves as an undefined variable.

The VM defines `print`, `len`, `typeOf`, and `setPrototypeOf`. `defer`, `runJobs`, `strContains`, `strSlice`, and `regexMatch` are absent, and a call to one fails at runtime with an undefined variable error. Module syntax is tree-walk only, and the VM rejects it with `Compiler error ... modules are supported by the tree-walk module evaluator only; unset JSRT_BACKEND=vm`. The VM applies the same 256-frame call depth limit as the tree-walk evaluator, and a construct the compiler cannot lower fails with a compiler diagnostic and a nonzero exit status.

The tree-walk evaluator defines the semantics, and the parity fixtures define the VM subset. `make vm-parity` runs every fixture through both backends and compares stdout, stderr, and exit status.

## Embedding API

The supported embedding surface is `include/jsrt/runtime.h`, linked from `build/libjsrt.a` with `-lm`. The other installed headers expose the lexer, parser, evaluator, VM, values, and collector for code that needs the lower-level interfaces.

```sh
cc -Iinclude embed.c build/libjsrt.a -lm -o embed
```

`make install` lays out the binary, the library, the headers, and `jsrt.pc` under the prefix:

```sh
cc embed.c $(pkg-config --cflags --libs jsrt) -o embed
```

### Functions

| Function | Behavior |
| --- | --- |
| `const char *jsrt_runtime_version(void)` | Returns `"0.2.0"`. The version macros `JSRT_VERSION_MAJOR`, `JSRT_VERSION_MINOR`, and `JSRT_VERSION_PATCH` match. |
| `JsrtRuntime *jsrt_runtime_create(void)` | Creates a runtime with one global environment. |
| `void jsrt_runtime_destroy(JsrtRuntime *runtime)` | Destroys the runtime and releases its environment and collected objects. A null pointer is a no-op. |
| `JsrtRunResult jsrt_runtime_eval_source(JsrtRuntime *runtime, const char *source, const char *module_path)` | Lexes, parses, and evaluates source in the runtime environment. module_path supplies the importer path for relative imports; pass NULL for code with no imports. |
| `JsrtRunResult jsrt_runtime_eval_file(JsrtRuntime *runtime, const char *path)` | Reads path and evaluates it as the top-level module. An unreadable file returns a lookup error. |
| `void jsrt_runtime_set_interrupt(JsrtRuntime *runtime, JsrtInterruptCallback callback, void *user_data)` | Installs a cancellation callback for the next eval call. Pass NULL to clear it. |
| `void jsrt_run_result_free(JsrtRunResult *result)` | Releases the strings in a result and zeroes its fields. A null pointer is a no-op. |

The runtime API always evaluates on the tree-walk backend. The bytecode VM is selected on the CLI.

### Result fields

| Field | Meaning |
| --- | --- |
| `ok` | True when the program ran to completion. |
| `has_value` | True when the final value is not null. |
| `value_text` | Display form of the final value, or null when `has_value` is false. |
| `error_message` | Diagnostic text, or null on success. |
| `runtime_error_kind` | `RuntimeErrorKind` value for a runtime failure. |
| `error_line` | 1-based source line for a runtime failure, or 0. |
| `error_column` | 1-based source column for a runtime failure, or 0. |

Runtime failures fill `error_line` and `error_column`. Lexer and parser failures return `RUNTIME_ERROR_NONE` with line and column 0, and the message carries the position.

### Lifetime and ownership

A runtime keeps one environment for its whole life. A name declared with `let`, `const`, or `function` in one eval call stays visible to later calls on the same runtime. Declaring an existing name again returns a redeclaration error. One runtime corresponds to one independent run, and the REPL keeps its bindings the same way.

The environment owns the parsed program of each eval call. The lower-level entry points in `include/jsrt/eval.h`, `eval_program` and `eval_program_with_path`, take ownership of the parsed `StmtArray`: they reset the incoming array, so freeing it after the call is a no-op. The runtime wrapper parses the source, evaluates it, and frees the tokens itself.

Module records are per eval call: the evaluator frees them when the outermost call returns, and the next call re-reads each imported module. Imported functions remain callable after the eval returns. Jobs queued with `defer` do not outlive the eval call.

### Interrupts

Install an interrupt callback with `jsrt_runtime_set_interrupt(runtime, callback, user_data)`. The callback runs on the eval thread between statements and between VM instructions, and it returns a `bool`. Returning true stops the program with `Runtime error [internal] ... execution interrupted` and `ok` set to false. Pass `NULL` to clear the callback. The callback must not call back into jsrt.

The interrupt path is the supported way to stop a runaway loop such as `while (true) { }` from a host program.

### Concurrency

Eval calls are not thread-safe. The collector, the runtime list, and the module records are process-global, so serialize eval calls across threads or run one process per independent run.

## Verification

| Target | What it runs |
| --- | --- |
| `make test` | Runs the C unit suite and prints the assertion count. |
| `make conformance` | Runs `scripts/conformance.sh` over `conformance/manifest.csv`. |
| `make vm-parity` | Runs `scripts/vm-parity.sh` over `conformance/vm-parity-manifest.csv`. |
| `make test262-adapter` | Runs `scripts/test262-adapter.sh` over `conformance/test262-subset-manifest.csv`. |
| `make cli-test` | Runs `scripts/cli.sh`, the CLI contract checks. |
| `make prose` | Runs `scripts/prose.sh` over the tree. |
| `make verify-production` | Runs `test`, `conformance`, `test262-adapter`, `vm-parity`, `prose`, and `cli-test`. |
| `make bench-smoke` | Runs one benchmark iteration per fixture. |
| `make asan` | Builds the CLI and unit tests with AddressSanitizer through `zig build asan` and runs the suite. |
| `zig build test` | Runs the same unit suite through the Zig build. |

The conformance manifest has the columns `id,path,backends,expect,stdout,stderr_contains`. The `backends` column takes `both`, `tree`, or `vm`, and `expect` takes `success` or `failure`. A failure row can require a substring in stderr.

The Test262 subset manifest has the columns `id,feature,area,tier,source,local_path,backends,expect,stdout,stderr_contains,skip_reason,xfail_reason`. The `tier` column takes `supported`, `experimental`, `future`, or `rejected`; `backends` takes `tree`, `vm`, `both`, or blank; `expect` takes `success`, `failure`, `skip`, or `xfail`. A skip row needs `skip_reason`, an expected failure needs `xfail_reason`, and a non-skip row with `local_path` must point to an existing file.

The GitHub Actions workflow in `.github/workflows/ci.yml` runs three jobs: the plain C build with the gates, the sanitizer lane, and the Zig lane. Set the repository variable `CI_RUNNER` to a managed runner label to move the jobs to another GitHub Actions runner host without editing the workflow; leave it unset to use `ubuntu-latest`.

## Benchmarks

The benchmark harness runs fixtures from `benchmarks/manifest.csv` and prints CSV rows to stdout.

| Mode | Target | Backend |
| --- | --- | --- |
| `bench` | `make bench` | Tree-walk evaluator. |
| `bench-vm` | `make bench-vm` | VM. |
| `bench-compare` | `make bench-compare` | Both backends, paired rows. |
| `bench-smoke` | `make bench-smoke` | One iteration per fixture, to check the harness. |

The make variables `ITERATIONS`, `WARMUPS`, `FIXTURE`, and `FILTER` set the iteration count, warmup count, fixture, and text filter. `scripts/benchmark.sh` also accepts the flags `--iterations`, `--warmups`, `--fixture`, `--filter`, `--backend`, `--manifest`, and `--binary`.

The manifest has the columns `fixture,category,supported_backends,compare,expected_stdout,smoke_expectation`. Output rows have the columns `mode,fixture,category,backend,compare,iterations,warmups,total_ms,mean_ms,min_ms,max_ms,status,stdout_match`.

Times are wall-clock milliseconds on one machine. Compare runs on the same host, and do not read the numbers as portable performance claims.

## Known non-goals

| Area | Boundary |
| --- | --- |
| ECMAScript coverage | The parser accepts the subset listed in Language scope. |
| Hosts | The runtime runs inside a C program, a terminal, or a test harness. Browser, DOM, and Node environments are absent. |
| Modules | Static named imports of local files. Dynamic `import()` and package resolution are outside the loader, and cyclic graphs are rejected. |
| Asynchronous behavior | The `defer` and `runJobs` queue. Promise, async function, and event-loop semantics are outside the runtime. |
| Regular expressions | Literal bytes, `.`, character classes, the `\d`, `\w`, and `\s` classes, `*`, `+`, `?`, and anchors. Groups, alternation, counted repetition, and flags are outside the matcher. |
| Type coercion | Addition concatenates unless both operands are numbers, and `==` does not coerce. |
| Classes | A constructor and instance methods. `extends`, `super`, static members, class fields, and class expressions fail to parse. |
| Compilation | Tree-walk and bytecode interpreters only. Native code generation is absent. |
| VM coverage | The VM runs the compatible subset, checked by the parity fixtures. It leaves out captured-variable writes, named function expression self-reference, module syntax, and five of the nine builtins. |
| Embedding surface | Runtime lifecycle, source evaluation, diagnostics, and interrupts. Native host functions, cancellation tokens, and value handles are outside the API. |

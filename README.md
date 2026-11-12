# jsrt

jsrt is a small JavaScript-like runtime written in C. It parses a subset of JavaScript and runs it on a tree-walk evaluator or on a bytecode VM. The same engine ships as a command line interpreter and as a static library.

It is for engineers who need a readable engine to embed, extend, or study without pulling in a browser or a Node toolchain. The code holds a hand-written lexer, a recursive descent parser, a tree-walk evaluator, a bytecode compiler, and a managed heap.

Build it and evaluate one expression:

```sh
make
./build/jsrt -e '1 + 2'
```

The second command prints `3`.

## Build and install

`make` builds three files: `build/jsrt` (the interpreter), `build/libjsrt.a` (the static library), and `build/test_jsrt` (the unit test binary). You need a C17 compiler, `ar`, and `make`. The plain build does not need Zig.

`make install` copies the interpreter to `$(PREFIX)/bin`, the library to `$(PREFIX)/lib`, the headers to `$(PREFIX)/include/jsrt`, and a pkg-config file to `$(PREFIX)/lib/pkgconfig`. Stage the install tree with `DESTDIR`:

```sh
make install PREFIX=/usr/local DESTDIR=/tmp/jsrt-stage
```

Zig 0.17 is the second build path. `zig build` builds the CLI, the library, and the headers, and `zig build test` runs the unit suite through Zig. Both build paths write `build/jsrt`, `build/libjsrt.a`, and `build/test_jsrt`.

## Run scripts

`jsrt file.js` runs a file. `jsrt -e 'code'` evaluates a string. `jsrt -` reads standard input explicitly. When the command line names no file or code, jsrt reads standard input if it is not a terminal and starts a REPL if it is.

Add `--vm` or set `JSRT_BACKEND=vm` to run on the bytecode backend. The REPL always uses the tree-walk evaluator; with a terminal on stdin, `--vm` prints a note on stderr and starts the tree-walk REPL.

The interpreter prints the value of the final statement when that value is not null. It exits 0 on success, 1 when a script fails or an input file cannot be read, and 2 on a command line mistake.

The REPL prompts with `jsrt> ` and continues a multi-line entry with `  ... `. Enter `quit`, `exit`, or `:quit` to leave. Bindings survive across lines in one session.

## A script

```js
function Counter(start) {
    this.value = start;
}

Counter.prototype.next = function () {
    this.value = this.value + 1;
    return this.value;
};

let counter = new Counter(40);
counter.next() + ":" + counter.next();
```

Save it as `counter.js` and run it with `./build/jsrt counter.js`. The script prints `41:42`. It uses a constructor function, a method stored on the prototype, and a `let` binding. Constructor calls need parentheses around the arguments.

## Embed it in C

```c
#include <stdio.h>

#include "jsrt/runtime.h"

int main(void) {
    JsrtRuntime *runtime = jsrt_runtime_create();
    JsrtRunResult result = jsrt_runtime_eval_source(runtime, "let base = 40;", "embed.js");

    if (result.ok) {
        jsrt_run_result_free(&result);
        result = jsrt_runtime_eval_source(runtime, "base + 2;", "embed.js");
    }
    if (!result.ok) {
        fprintf(stderr, "%s\n", result.error_message);
        jsrt_run_result_free(&result);
        jsrt_runtime_destroy(runtime);
        return 1;
    }
    printf("%s\n", result.value_text);
    jsrt_run_result_free(&result);
    jsrt_runtime_destroy(runtime);
    return 0;
}
```

Compile it against the build tree and run it:

```sh
cc -Iinclude embed.c build/libjsrt.a -lm -o embed
./embed
```

That prints `42`.

A runtime owns one global environment, so `base` stays defined in the second eval call. Names declared with `let`, `const`, or `function` in one call are visible in later calls on the same runtime. Declaring the same name twice in the same runtime is an error. Create one runtime per independent run and destroy it when the run ends.

`jsrt_runtime_eval_source` returns a `JsrtRunResult`. Check `ok` first. On success, `value_text` holds the printed form of the final value and `has_value` says whether a value was present. On failure, `error_message` describes the problem, `runtime_error_kind` names the category, and `error_line` and `error_column` give the source position for runtime errors. Call `jsrt_run_result_free` once per result.

`jsrt_runtime_eval_file(runtime, path)` evaluates a file instead of a string. Each eval call re-reads the modules it imports. Eval calls in one process share the collector and module state, so do not run two of them at the same time from different threads.

For a runaway loop, install an interrupt callback with `jsrt_runtime_set_interrupt(runtime, callback, user_data)`. The callback runs between statements and returning true stops the eval with an `execution interrupted` error. Pass `NULL` to clear the callback, and do not call back into jsrt from inside it.

## What it supports

Numbers are 64-bit floating point values. The other value kinds are booleans, `null`, and strings in single or double quotes. Strings hold bytes, so `len` and `strSlice` count bytes. Comments use `//` or `/* */`.

Declarations use `let` and `const`; `var` is not part of the language. Control flow covers `if` and `else`, `while`, `for`, `do` and `while`, and `switch` with fallthrough. A `for (let ...)` loop binds a fresh variable for each iteration, so closures created in the body capture that iteration's value. Functions include declarations, expressions, closures, `return`, and recursion. Blocks create lexical scopes.

Arrays and objects have reference semantics: assigning an aggregate or passing it to a function copies the reference, so two holders see one object. Arrays take numeric indexes and have a fixed length, and an out-of-range index raises an error. Objects take string keys through dot or bracket access, and reading a missing key raises a lookup error. Trailing commas are allowed in literals, parameter lists, and call arguments, and duplicate object keys keep the last value. `len` returns the byte length of a string, the element count of an array, or the number of unique keys in an object.

Equality compares primitive values by contents and aggregates by structure, and it terminates on cycles. Numbers compare exactly, so `0.1 + 0.2 == 0.3` is false, and zero divided by zero raises a math error instead of producing a NaN. `==` and `!=` do not coerce: `1 == "1"` is false. The tree-walk evaluator is the semantic reference for both backends.

Constructor calls need parentheses after `new`, and `Function.prototype` holds the methods. `setPrototypeOf` sets a prototype link explicitly and rejects self-links and cycles. Class declarations lower to a constructor plus prototype methods. `extends`, `super`, static members, and class fields stay outside the language.

Modules use static named `import` and `export` on the tree-walk backend. The VM rejects module syntax with a compiler error. Imports resolve relative to the importing file and include the file extension. An import runs where it appears and holds a snapshot of the exported value. A cyclic import fails with an error.

The global environment defines nine builtins: `print`, `len`, `typeOf`, `setPrototypeOf`, `defer`, `runJobs`, `strContains`, `strSlice`, and `regexMatch`. The VM defines four of them: `print`, `len`, `typeOf`, and `setPrototypeOf`.

## Limits and non-goals

jsrt inserts a semicolon only at the end of a file or directly before a closing brace. Everywhere else the semicolon is required, so `let x = 1 let y = 2` is a parse error.

User calls nest to 256 frames, the parser accepts 200 levels of nesting, and one expression chain may hold 64 operators. The parsed tree, structural equality, and printing have depth and size caps. Past a cap the runtime reports an error or prints `<depth limit>` or `<output limit>` instead of crashing or expanding without limit.

The VM runs a subset of the tree-walk language. It handles `if` and `else`, functions and nested functions, classes, `new`, and `this`, and its closures can read captured variables. Writes to captured variables from a nested function fail at compile time, module syntax is rejected with a compiler error, and the VM defines only the four builtins listed above.

jsrt stops short of full ECMAScript. Browser and Node hosts, dynamic `import()`, package resolution, Promise, async and event-loop behavior, full regular expressions, broad type coercion, `extends` and `super`, class fields, JIT compilation, VM parity for every tree-walk feature, and advanced host callbacks are outside the project.

## Verify the build

`make test` runs the C unit suite and prints the assertion count. `make conformance` checks the script manifest, `make vm-parity` compares the tree-walk evaluator and the VM on the parity fixtures, and `make test262-adapter` validates the Test262 subset manifest. `make cli-test` exercises the CLI contract and `make prose` runs the prose checker. `make verify-production` runs all of them together.

`make bench-smoke` checks the benchmark harness. `make bench`, `make bench-vm`, and `make bench-compare` print CSV measurements; the timings describe one machine and are meant for comparing changes on that machine.

`make asan` builds the CLI and the unit tests with AddressSanitizer through Zig, and `zig build test` runs the same unit tests through the Zig lane.

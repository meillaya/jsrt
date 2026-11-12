#!/usr/bin/env bash
# CLI contract tests for jsrt.
#
# Run from the repository root against the default build:
#
#     bash scripts/cli.sh
#
# Point the script at another binary with BIN:
#
#     BIN=/path/to/jsrt bash scripts/cli.sh
#
# Fixtures and captured output land in TMP_DIR, build/cli by default.
set -euo pipefail

BIN=${BIN:-./build/jsrt}
TMP_DIR=${TMP_DIR:-build/cli}
REPO_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)

# The default checks expect the tree-walk backend and no collector trace.
unset JSRT_BACKEND JSRT_GC_TRACE

STDOUT_FILE="$TMP_DIR/stdout.txt"
STDERR_FILE="$TMP_DIR/stderr.txt"
checks=0
skips=0
last_status=0
last_stdout=
last_stderr=
expected_version=

setup_fail() {
  printf 'cli: %s\n' "$1" >&2
  exit 1
}

fail() {
  printf 'cli: FAIL - %s\n' "$1" >&2
  printf '  exit: %s\n' "${last_status:-n/a}" >&2
  printf '  stdout:\n%s\n' "${last_stdout:-<empty>}" >&2
  printf '  stderr:\n%s\n' "${last_stderr:-<empty>}" >&2
  exit 1
}

pass() {
  checks=$((checks + 1))
  printf 'ok %d - %s\n' "$checks" "$1"
}

skip() {
  skips=$((skips + 1))
  printf 'skip - %s\n' "$1"
}

cli() {
  last_status=0
  "$BIN" "$@" >"$STDOUT_FILE" 2>"$STDERR_FILE" || last_status=$?
  last_stdout=$(cat -- "$STDOUT_FILE")
  last_stderr=$(cat -- "$STDERR_FILE")
}

cli_pipe() {
  local source=$1
  shift
  last_status=0
  printf '%s' "$source" | "$BIN" "$@" >"$STDOUT_FILE" 2>"$STDERR_FILE" || last_status=$?
  last_stdout=$(cat -- "$STDOUT_FILE")
  last_stderr=$(cat -- "$STDERR_FILE")
}

cli_stdin() {
  local input=$1
  shift
  last_status=0
  "$BIN" "$@" <"$input" >"$STDOUT_FILE" 2>"$STDERR_FILE" || last_status=$?
  last_stdout=$(cat -- "$STDOUT_FILE")
  last_stderr=$(cat -- "$STDERR_FILE")
}

cli_env() {
  local assignment=$1
  shift
  last_status=0
  env "$assignment" "$BIN" "$@" >"$STDOUT_FILE" 2>"$STDERR_FILE" || last_status=$?
  last_stdout=$(cat -- "$STDOUT_FILE")
  last_stderr=$(cat -- "$STDERR_FILE")
}

assert_status() {
  local expected=$1
  local name=$2
  if [[ $last_status -ne $expected ]]; then
    fail "$name: expected exit $expected, got $last_status"
  fi
}

assert_stdout() {
  local expected=$1
  local name=$2
  if [[ "$last_stdout" != "$expected" ]]; then
    fail "$name: expected stdout '$expected', got '$last_stdout'"
  fi
}

assert_stdout_contains() {
  local needle=$1
  local name=$2
  if [[ "$last_stdout" != *"$needle"* ]]; then
    fail "$name: stdout does not contain '$needle'"
  fi
}

assert_stdout_absent() {
  local needle=$1
  local name=$2
  if [[ "$last_stdout" == *"$needle"* ]]; then
    fail "$name: stdout unexpectedly contains '$needle'"
  fi
}

assert_stderr_contains() {
  local needle=$1
  local name=$2
  if [[ "$last_stderr" != *"$needle"* ]]; then
    fail "$name: stderr does not contain '$needle'"
  fi
}

load_expected_version() {
  local header="$REPO_ROOT/include/jsrt/runtime.h"
  local major minor patch
  if [[ ! -r "$header" ]]; then
    setup_fail "cannot read $header"
  fi
  major=$(sed -n 's/^#define JSRT_VERSION_MAJOR //p' "$header")
  minor=$(sed -n 's/^#define JSRT_VERSION_MINOR //p' "$header")
  patch=$(sed -n 's/^#define JSRT_VERSION_PATCH //p' "$header")
  if [[ -z "$major" || -z "$minor" || -z "$patch" ]]; then
    setup_fail "cannot parse the version from $header"
  fi
  expected_version="jsrt $major.$minor.$patch"
}

write_recursion_fixture() {
  local path=$1
  local depth=$2
  cat >"$path" <<EOF
function f(n) {
    if (n <= 0) {
        return 0;
    }
    return 1 + f(n - 1);
}
f($depth);
EOF
}

prepare_fixtures() {
  if ! mkdir -p "$TMP_DIR"; then
    setup_fail "cannot create $TMP_DIR"
  fi
  cat >"$TMP_DIR/value.js" <<'FIXTURE'
let total = 20;
total + 22
FIXTURE
  cat >"$TMP_DIR/stdin.js" <<'FIXTURE'
1 + 2
FIXTURE
  cat >"$TMP_DIR/dash.js" <<'FIXTURE'
6 * 7
FIXTURE
  cat >"$TMP_DIR/no-semicolon.js" <<'FIXTURE'
let a = 1
let b = 2
FIXTURE
  cat >"$TMP_DIR/final-no-semicolon.js" <<'FIXTURE'
let a = 1; a + 1
FIXTURE
  rm -f "$TMP_DIR/missing.js"
  write_recursion_fixture "$TMP_DIR/recursion-100.js" 100
  write_recursion_fixture "$TMP_DIR/recursion-300.js" 300
}

test_version() {
  cli --version
  assert_status 0 "--version"
  assert_stdout "$expected_version" "--version"
  pass "--version prints $expected_version and exits 0"

  cli -V
  assert_status 0 "-V"
  assert_stdout "$expected_version" "-V"
  pass "-V prints $expected_version and exits 0"
}

test_help() {
  cli --help
  assert_status 0 "--help"
  assert_stdout_contains "Usage:" "--help"
  assert_stdout_contains "--eval" "--help"
  pass "--help prints usage and exits 0"

  cli -h
  assert_status 0 "-h"
  assert_stdout_contains "Usage:" "-h"
  pass "-h prints usage and exits 0"

  cli --no-such-option
  assert_status 2 "unknown option"
  assert_stdout "" "unknown option"
  assert_stderr_contains "unknown option" "unknown option"
  pass "an unknown option exits 2 with a message on stderr"
}

test_eval() {
  cli -e '1 + 2'
  assert_status 0 "-e"
  assert_stdout "3" "-e"
  pass "-e evaluates a source string"

  cli --eval 'let x = 2; x * 21'
  assert_status 0 "--eval"
  assert_stdout "42" "--eval"
  pass "--eval evaluates a statement list"
}

test_file_execution() {
  cli "$TMP_DIR/value.js"
  assert_status 0 "file execution"
  assert_stdout "42" "file execution"
  pass "a script file prints its final expression value"
}

test_stdin() {
  cli_pipe '1 + 2'
  assert_status 0 "piped stdin"
  assert_stdout "3" "piped stdin"
  assert_stdout_absent "jsrt>" "piped stdin"
  pass "piped stdin runs and does not start the REPL"

  cli_stdin "$TMP_DIR/dash.js" -
  assert_status 0 "dash argument"
  assert_stdout "42" "dash argument"
  pass "a dash argument reads the script from stdin"
}

test_vm_backend() {
  cli --vm -e 'let x = 1 + 2; x;'
  assert_status 0 "--vm"
  assert_stdout "3" "--vm"
  pass "--vm runs the VM backend and prints 3"

  cli_env JSRT_BACKEND=vm -e '1 + 2'
  assert_status 0 "JSRT_BACKEND=vm"
  assert_stdout "3" "JSRT_BACKEND=vm"
  pass "JSRT_BACKEND=vm selects the VM backend"

  cli --vm -e 'import { x } from "./missing.js";'
  assert_status 1 "VM module syntax"
  assert_stdout "" "VM module syntax"
  assert_stderr_contains "tree-walk" "VM module syntax"
  pass "module syntax fails on --vm instead of running with wrong output"
}

test_errors() {
  cli "$TMP_DIR/missing.js"
  assert_status 1 "missing file"
  assert_stderr_contains "cannot read" "missing file"
  pass "a missing file exits 1 with a cannot read message"

  cli -e 'let = ;'
  assert_status 1 "syntax error"
  assert_stderr_contains "Parser error" "syntax error"
  pass "a syntax error exits 1 with a parser error"

  cli -e 'null.x'
  assert_status 1 "runtime error"
  assert_stderr_contains "Runtime error" "runtime error"
  pass "a runtime error exits 1 with a runtime error"

  cli "$TMP_DIR/no-semicolon.js"
  assert_status 1 "missing semicolon"
  assert_stderr_contains "Parser error" "missing semicolon"
  pass "two statements on separate lines need a semicolon"

  cli "$TMP_DIR/final-no-semicolon.js"
  assert_status 0 "final statement without semicolon"
  assert_stdout "2" "final statement without semicolon"
  pass "the final statement can omit its semicolon"
}

test_recursion_limit() {
  cli "$TMP_DIR/recursion-300.js"
  assert_status 1 "tree-walk depth limit"
  assert_stderr_contains "maximum call depth exceeded" "tree-walk depth limit"
  pass "tree-walk recursion past 256 fails with a depth error"

  cli "$TMP_DIR/recursion-100.js"
  assert_status 0 "tree-walk depth under the limit"
  assert_stdout "100" "tree-walk depth under the limit"
  pass "tree-walk recursion under the limit succeeds"

  cli --vm "$TMP_DIR/recursion-300.js"
  assert_status 1 "VM depth limit"
  assert_stderr_contains "maximum call depth exceeded" "VM depth limit"
  pass "VM recursion past 256 fails with a depth error"

  cli --vm "$TMP_DIR/recursion-100.js"
  assert_status 0 "VM depth under the limit"
  assert_stdout "100" "VM depth under the limit"
  pass "VM recursion under the limit succeeds"
}

test_repl() {
  local repl_output normalized
  if ! command -v script >/dev/null 2>&1; then
    skip "the REPL check needs script from util-linux"
    return
  fi
  last_status=0
  repl_output=$(printf '1 + 2\nquit\n' | script -qec "$BIN" /dev/null 2>"$STDERR_FILE") || last_status=$?
  last_stderr=$(cat -- "$STDERR_FILE")
  normalized=$(printf '%s\n' "$repl_output" | tr -d '\r')
  if [[ $last_status -ne 0 ]]; then
    last_stdout=$normalized
    fail "REPL: expected exit 0, got $last_status"
  fi
  if [[ "$normalized" != *"jsrt> "* ]]; then
    last_stdout=$normalized
    fail "REPL: prompt is missing from the output"
  fi
  if ! printf '%s\n' "$normalized" | grep -Eq '^jsrt> 3$|^3$'; then
    last_stdout=$normalized
    fail "REPL: expected the result line 3"
  fi
  pass "the REPL starts on a tty, evaluates input, and quits cleanly"
}

if [[ ! -x "$BIN" ]]; then
  setup_fail "binary is not executable: $BIN (run make first)"
fi
load_expected_version
prepare_fixtures

test_version
test_help
test_eval
test_file_execution
test_stdin
test_vm_backend
test_errors
test_recursion_limit
test_repl

if [[ $checks -eq 0 ]]; then
  setup_fail "no checks ran"
fi
printf 'cli: %d checks passed\n' "$checks"

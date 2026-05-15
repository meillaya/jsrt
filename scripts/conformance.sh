#!/usr/bin/env bash
set -euo pipefail

BIN=${BIN:-./build/jsrt}
MANIFEST=${MANIFEST:-conformance/manifest.csv}
TMP_DIR=${TMP_DIR:-build/conformance}

mkdir -p "$TMP_DIR"
failures=0
cases=0

run_case() {
  local id=$1 path=$2 backend=$3 expect=$4 expected_stdout=$5 stderr_contains=$6
  local stdout_file="$TMP_DIR/$id-$backend.out"
  local stderr_file="$TMP_DIR/$id-$backend.err"
  local status=0

  cases=$((cases + 1))
  if [[ "$backend" == "vm" ]]; then
    JSRT_BACKEND=vm "$BIN" "$path" >"$stdout_file" 2>"$stderr_file" || status=$?
  else
    "$BIN" "$path" >"$stdout_file" 2>"$stderr_file" || status=$?
  fi

  if [[ "$expect" == "success" ]]; then
    if [[ $status -ne 0 ]]; then
      echo "not ok - $id [$backend] expected success, got status $status" >&2
      failures=$((failures + 1))
      return
    fi
    if [[ -n "$expected_stdout" ]]; then
      local actual
      actual=$(tr -d '\r' <"$stdout_file" | sed -e '${s/[[:space:]]*$//;}')
      if [[ "$actual" != "$expected_stdout" ]]; then
        echo "not ok - $id [$backend] expected stdout '$expected_stdout', got '$actual'" >&2
        failures=$((failures + 1))
        return
      fi
    fi
  else
    if [[ $status -eq 0 ]]; then
      echo "not ok - $id [$backend] expected failure" >&2
      failures=$((failures + 1))
      return
    fi
    if [[ -n "$stderr_contains" ]] && ! grep -Fq "$stderr_contains" "$stderr_file"; then
      echo "not ok - $id [$backend] stderr missing '$stderr_contains'" >&2
      failures=$((failures + 1))
      return
    fi
  fi

  echo "ok - $id [$backend]"
}

while IFS=, read -r id path backends expect stdout stderr_contains; do
  [[ "$id" == "id" || -z "$id" ]] && continue
  case "$backends" in
    both)
      run_case "$id" "$path" tree "$expect" "$stdout" "$stderr_contains"
      run_case "$id" "$path" vm "$expect" "$stdout" "$stderr_contains"
      ;;
    tree|vm)
      run_case "$id" "$path" "$backends" "$expect" "$stdout" "$stderr_contains"
      ;;
    *)
      echo "unknown backend selector '$backends' for $id" >&2
      failures=$((failures + 1))
      ;;
  esac
done <"$MANIFEST"

if [[ $failures -ne 0 ]]; then
  echo "Conformance failed: $failures/$cases case(s) failed" >&2
  exit 1
fi

echo "Conformance passed: $cases case(s)"

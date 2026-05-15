#!/usr/bin/env bash
set -euo pipefail

BIN=${BIN:-./build/jsrt}
MANIFEST=${MANIFEST:-conformance/vm-parity-manifest.csv}
TMP_DIR=${TMP_DIR:-build/vm-parity}
mkdir -p "$TMP_DIR"
failures=0
cases=0

while IFS=, read -r id path; do
  [[ "$id" == "id" || -z "$id" ]] && continue
  cases=$((cases + 1))
  tree_out="$TMP_DIR/$id.tree.out"
  tree_err="$TMP_DIR/$id.tree.err"
  vm_out="$TMP_DIR/$id.vm.out"
  vm_err="$TMP_DIR/$id.vm.err"
  tree_status=0
  vm_status=0
  "$BIN" "$path" >"$tree_out" 2>"$tree_err" || tree_status=$?
  JSRT_BACKEND=vm "$BIN" "$path" >"$vm_out" 2>"$vm_err" || vm_status=$?
  if [[ $tree_status -ne $vm_status ]]; then
    echo "not ok - $id status mismatch tree=$tree_status vm=$vm_status" >&2
    failures=$((failures + 1))
    continue
  fi
  if ! cmp -s "$tree_out" "$vm_out"; then
    echo "not ok - $id stdout mismatch" >&2
    diff -u "$tree_out" "$vm_out" >&2 || true
    failures=$((failures + 1))
    continue
  fi
  if [[ $tree_status -ne 0 ]] && ! cmp -s "$tree_err" "$vm_err"; then
    echo "not ok - $id stderr mismatch" >&2
    diff -u "$tree_err" "$vm_err" >&2 || true
    failures=$((failures + 1))
    continue
  fi
  echo "ok - $id"
done < "$MANIFEST"

if [[ $failures -ne 0 ]]; then
  echo "VM parity failed: $failures/$cases case(s)" >&2
  exit 1
fi

echo "VM parity passed: $cases case(s)"

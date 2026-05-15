#!/usr/bin/env bash
set -euo pipefail

MANIFEST=${MANIFEST:-conformance/test262-subset-manifest.csv}
failures=0
cases=0

if [[ ! -f "$MANIFEST" ]]; then
  echo "missing Test262 subset manifest: $MANIFEST" >&2
  exit 1
fi

while IFS=, read -r id feature milestone tier source local_path backends expect stdout stderr_contains skip_reason xfail_reason; do
  [[ "$id" == "id" || -z "$id" ]] && continue
  cases=$((cases + 1))
  case "$tier" in supported|experimental|future|rejected) ;;
    *) echo "not ok - $id invalid tier '$tier'" >&2; failures=$((failures + 1)); continue ;;
  esac
  case "$backends" in tree|vm|both|"") ;;
    *) echo "not ok - $id invalid backends '$backends'" >&2; failures=$((failures + 1)); continue ;;
  esac
  case "$expect" in success|failure|skip|xfail) ;;
    *) echo "not ok - $id invalid expect '$expect'" >&2; failures=$((failures + 1)); continue ;;
  esac
  if [[ "$expect" == "skip" && -z "$skip_reason" ]]; then
    echo "not ok - $id skip missing skip_reason" >&2
    failures=$((failures + 1))
    continue
  fi
  if [[ "$expect" == "xfail" && -z "$xfail_reason" ]]; then
    echo "not ok - $id xfail missing xfail_reason" >&2
    failures=$((failures + 1))
    continue
  fi
  if [[ "$expect" != "skip" && -n "$local_path" && ! -f "$local_path" ]]; then
    echo "not ok - $id missing local_path '$local_path'" >&2
    failures=$((failures + 1))
    continue
  fi
  echo "ok - $id [$tier/$expect]"
done < "$MANIFEST"

if [[ $failures -ne 0 ]]; then
  echo "Test262 subset manifest failed: $failures/$cases case(s)" >&2
  exit 1
fi

echo "Test262 subset manifest passed: $cases case(s)"

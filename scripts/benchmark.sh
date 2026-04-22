#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
readonly ROOT_DIR
readonly OUTPUT_HEADER='mode,fixture,category,backend,compare,iterations,warmups,total_ms,mean_ms,min_ms,max_ms,status,stdout_match'
readonly MANIFEST_HEADER='fixture,category,supported_backends,compare,expected_stdout,smoke_expectation'
readonly DEFAULT_MANIFEST="$ROOT_DIR/benchmarks/manifest.csv"
readonly DEFAULT_BINARY="$ROOT_DIR/build/jsrt"

MANIFEST=${MANIFEST:-$DEFAULT_MANIFEST}
BINARY=${BINARY:-$DEFAULT_BINARY}
ITERATIONS=${ITERATIONS:-5}
WARMUPS=${WARMUPS:-1}
FILTER=${FILTER:-}
FIXTURE=${FIXTURE:-}
BACKEND=${BACKEND:-}

FIXTURES=()
declare -A META_CATEGORY=()
declare -A META_SUPPORTED=()
declare -A META_COMPARE=()
declare -A META_EXPECTED=()
declare -A META_SMOKE=()

die() {
  echo "benchmark.sh: $*" >&2
  exit 1
}

usage() {
  cat <<USAGE
Usage: scripts/benchmark.sh <bench|bench-vm|bench-compare|bench-smoke> [options]

Options:
  --iterations N   Number of timed iterations (default: $ITERATIONS)
  --warmups N      Number of warmup runs before timing (default: $WARMUPS)
  --fixture PATH   Run one manifest fixture exactly
  --filter TEXT    Run fixtures whose path or category contains TEXT
  --backend NAME   Validate backend override for bench/bench-vm (treewalk|vm)
  --manifest PATH  Use a different manifest file
  --binary PATH    Use a different jsrt binary
  --help           Show this help text
USAGE
}

parse_args() {
  MODE=${1:-}
  shift || true
  [[ -n "$MODE" ]] || {
    usage >&2
    exit 1
  }

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --iterations)
        [[ $# -ge 2 ]] || die "missing value for --iterations"
        ITERATIONS=$2
        shift 2
        ;;
      --warmups)
        [[ $# -ge 2 ]] || die "missing value for --warmups"
        WARMUPS=$2
        shift 2
        ;;
      --fixture)
        [[ $# -ge 2 ]] || die "missing value for --fixture"
        FIXTURE=$2
        shift 2
        ;;
      --filter)
        [[ $# -ge 2 ]] || die "missing value for --filter"
        FILTER=$2
        shift 2
        ;;
      --backend)
        [[ $# -ge 2 ]] || die "missing value for --backend"
        BACKEND=$2
        shift 2
        ;;
      --manifest)
        [[ $# -ge 2 ]] || die "missing value for --manifest"
        MANIFEST=$2
        shift 2
        ;;
      --binary)
        [[ $# -ge 2 ]] || die "missing value for --binary"
        BINARY=$2
        shift 2
        ;;
      --help|-h)
        usage
        exit 0
        ;;
      *)
        die "unknown option: $1"
        ;;
    esac
  done
}

require_non_negative_int() {
  local name=$1
  local value=$2
  [[ $value =~ ^[0-9]+$ ]] || die "$name must be a non-negative integer (got: $value)"
}

require_positive_int() {
  local name=$1
  local value=$2
  [[ $value =~ ^[1-9][0-9]*$ ]] || die "$name must be a positive integer (got: $value)"
}

contains_backend() {
  local supported=$1
  local backend=$2
  case "$supported" in
    "$backend"|treewalk\|vm)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

load_manifest() {
  local header fixture category supported compare expected smoke key
  [[ -f "$MANIFEST" ]] || die "manifest not found: $MANIFEST"
  IFS= read -r header < "$MANIFEST" || die "manifest is empty: $MANIFEST"
  [[ "$header" == "$MANIFEST_HEADER" ]] || die "manifest header must be: $MANIFEST_HEADER"

  FIXTURES=()
  while IFS=, read -r fixture category supported compare expected smoke || [[ -n "${fixture}${category}${supported}${compare}${expected}${smoke}" ]]; do
    [[ -n "$fixture" ]] || continue
    [[ -n "$category" ]] || die "manifest row missing category for fixture: $fixture"
    [[ -n "$supported" ]] || die "manifest row missing supported_backends for fixture: $fixture"
    [[ -n "$compare" ]] || die "manifest row missing compare flag for fixture: $fixture"
    [[ -n "$smoke" ]] || die "manifest row missing smoke_expectation for fixture: $fixture"
    if [[ "$supported" != "treewalk" && "$supported" != "vm" && "$supported" != "treewalk|vm" ]]; then
      die "manifest row has unsupported backend list '$supported' for fixture: $fixture"
    fi
    case "$compare" in
      yes|no) ;;
      *) die "manifest row has invalid compare flag '$compare' for fixture: $fixture" ;;
    esac
    case "$smoke" in
      exact_stdout|exit_zero) ;;
      *) die "manifest row has invalid smoke expectation '$smoke' for fixture: $fixture" ;;
    esac
    if [[ "$smoke" == "exact_stdout" && -z "$expected" ]]; then
      die "manifest row with exact_stdout expectation must set expected_stdout: $fixture"
    fi
    if [[ "$compare" == "yes" && "$supported" != "treewalk|vm" ]]; then
      die "compare-enabled fixture must support both treewalk and vm: $fixture"
    fi
    if [[ -n ${META_CATEGORY[$fixture]+_} ]]; then
      die "manifest contains duplicate fixture entry: $fixture"
    fi
    key="$ROOT_DIR/$fixture"
    [[ -f "$key" ]] || die "manifest fixture not found: $fixture"
    FIXTURES+=("$fixture")
    META_CATEGORY["$fixture"]=$category
    META_SUPPORTED["$fixture"]=$supported
    META_COMPARE["$fixture"]=$compare
    META_EXPECTED["$fixture"]=$expected
    META_SMOKE["$fixture"]=$smoke
  done < <(tail -n +2 "$MANIFEST")

  ((${#FIXTURES[@]} > 0)) || die "manifest does not declare any fixtures"
}

select_fixtures() {
  local mode=$1
  local selected=()
  local fixture supported compare category match

  for fixture in "${FIXTURES[@]}"; do
    supported=${META_SUPPORTED[$fixture]}
    compare=${META_COMPARE[$fixture]}
    category=${META_CATEGORY[$fixture]}

    if [[ -n "$FIXTURE" && "$fixture" != "$FIXTURE" ]]; then
      continue
    fi

    if [[ -n "$FILTER" ]]; then
      match=0
      if [[ "$fixture" == *"$FILTER"* || "$category" == *"$FILTER"* ]]; then
        match=1
      fi
      (( match == 1 )) || continue
    fi

    case "$mode" in
      bench)
        contains_backend "$supported" treewalk || continue
        ;;
      bench-vm)
        contains_backend "$supported" vm || continue
        ;;
      bench-compare)
        contains_backend "$supported" treewalk || continue
        contains_backend "$supported" vm || continue
        [[ "$compare" == "yes" ]] || continue
        ;;
      *)
        die "unknown benchmark mode: $mode"
        ;;
    esac

    selected+=("$fixture")
  done

  if [[ -n "$FIXTURE" && ! -n ${META_CATEGORY[$FIXTURE]+_} ]]; then
    die "fixture not found in manifest: $FIXTURE"
  fi

  if [[ -n "$FIXTURE" && ${#selected[@]} -eq 0 ]]; then
    case "$mode" in
      bench)
        die "fixture does not support treewalk benchmark mode: $FIXTURE"
        ;;
      bench-vm)
        die "fixture does not support vm benchmark mode: $FIXTURE"
        ;;
      bench-compare)
        die "fixture is not compare-enabled: $FIXTURE"
        ;;
    esac
  fi

  ((${#selected[@]} > 0)) || die "no fixtures matched mode '$mode'${FILTER:+ and filter '$FILTER'}"
  printf '%s\n' "${selected[@]}"
}

format_ms_from_ns() {
  local ns=$1
  awk -v ns="$ns" 'BEGIN { printf "%.3f", ns / 1000000 }'
}

run_process() {
  local backend=$1
  local fixture=$2
  local stdout_file=$3
  local stderr_file=$4
  local start_ns end_ns status

  start_ns=$(date +%s%N)
  if [[ "$backend" == "vm" ]]; then
    if JSRT_BACKEND=vm "$BINARY" "$ROOT_DIR/$fixture" >"$stdout_file" 2>"$stderr_file"; then
      status=0
    else
      status=$?
    fi
  else
    if "$BINARY" "$ROOT_DIR/$fixture" >"$stdout_file" 2>"$stderr_file"; then
      status=0
    else
      status=$?
    fi
  fi
  end_ns=$(date +%s%N)
  LAST_DURATION_NS=$((end_ns - start_ns))
  LAST_STATUS=$status
}

emit_row() {
  local mode=$1
  local fixture=$2
  local backend=$3
  local total_ns=$4
  local min_ns=$5
  local max_ns=$6
  local status=$7
  local stdout_match=$8
  local total_ms mean_ms min_ms max_ms

  total_ms=$(format_ms_from_ns "$total_ns")
  mean_ms=$(awk -v total="$total_ns" -v count="$ITERATIONS" 'BEGIN { printf "%.3f", total / count / 1000000 }')
  min_ms=$(format_ms_from_ns "$min_ns")
  max_ms=$(format_ms_from_ns "$max_ns")

  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "$mode" \
    "$fixture" \
    "${META_CATEGORY[$fixture]}" \
    "$backend" \
    "${META_COMPARE[$fixture]}" \
    "$ITERATIONS" \
    "$WARMUPS" \
    "$total_ms" \
    "$mean_ms" \
    "$min_ms" \
    "$max_ms" \
    "$status" \
    "$stdout_match"
}

run_one_fixture_backend() {
  local mode=$1
  local fixture=$2
  local backend=$3
  local tmp_dir stdout_file stderr_file iteration total_ns min_ns max_ns expected actual stdout_match smoke status

  tmp_dir=$(mktemp -d)
  stdout_file="$tmp_dir/stdout"
  stderr_file="$tmp_dir/stderr"
  total_ns=0
  min_ns=-1
  max_ns=0
  stdout_match=na
  smoke=${META_SMOKE[$fixture]}
  expected=${META_EXPECTED[$fixture]}

  for ((iteration = 0; iteration < WARMUPS; iteration += 1)); do
    run_process "$backend" "$fixture" "$stdout_file" "$stderr_file"
    [[ $LAST_STATUS -eq 0 ]] || {
      local stderr_text
      stderr_text=$(cat "$stderr_file")
      rm -rf "$tmp_dir"
      die "warmup failed for $fixture ($backend): ${stderr_text:-exit code $LAST_STATUS}"
    }
  done

  for ((iteration = 0; iteration < ITERATIONS; iteration += 1)); do
    run_process "$backend" "$fixture" "$stdout_file" "$stderr_file"
    total_ns=$((total_ns + LAST_DURATION_NS))
    if (( min_ns < 0 || LAST_DURATION_NS < min_ns )); then
      min_ns=$LAST_DURATION_NS
    fi
    if (( LAST_DURATION_NS > max_ns )); then
      max_ns=$LAST_DURATION_NS
    fi

    if [[ $LAST_STATUS -ne 0 ]]; then
      local stderr_text
      stderr_text=$(cat "$stderr_file")
      emit_row "$mode" "$fixture" "$backend" "$total_ns" "$min_ns" "$max_ns" error no
      rm -rf "$tmp_dir"
      die "benchmark failed for $fixture ($backend): ${stderr_text:-exit code $LAST_STATUS}"
    fi

    actual=$(cat "$stdout_file")
    case "$smoke" in
      exact_stdout)
        if [[ "$actual" == "$expected" ]]; then
          stdout_match=yes
        else
          emit_row "$mode" "$fixture" "$backend" "$total_ns" "$min_ns" "$max_ns" error no
          rm -rf "$tmp_dir"
          die "stdout mismatch for $fixture ($backend): expected '$expected', got '$actual'"
        fi
        ;;
      exit_zero)
        stdout_match=na
        ;;
    esac
  done

  emit_row "$mode" "$fixture" "$backend" "$total_ns" "$min_ns" "$max_ns" ok "$stdout_match"
  rm -rf "$tmp_dir"
}

run_mode() {
  local mode=$1
  local selection_text
  local selection=()
  local fixture

  [[ -x "$BINARY" ]] || die "jsrt binary not found or not executable: $BINARY"
  require_positive_int iterations "$ITERATIONS"
  require_non_negative_int warmups "$WARMUPS"
  load_manifest

  case "$mode" in
    bench)
      if [[ -n "$BACKEND" && "$BACKEND" != "treewalk" ]]; then
        die "bench mode only supports backend 'treewalk'"
      fi
      selection_text=$(select_fixtures "$mode")
      mapfile -t selection <<<"$selection_text"
      ;;
    bench-vm)
      if [[ -n "$BACKEND" && "$BACKEND" != "vm" ]]; then
        die "bench-vm mode only supports backend 'vm'"
      fi
      selection_text=$(select_fixtures "$mode")
      mapfile -t selection <<<"$selection_text"
      ;;
    bench-compare)
      [[ -z "$BACKEND" ]] || die "bench-compare does not accept --backend; it runs both treewalk and vm"
      selection_text=$(select_fixtures "$mode")
      mapfile -t selection <<<"$selection_text"
      ;;
    *)
      die "unknown benchmark mode: $mode"
      ;;
  esac

  printf '%s\n' "$OUTPUT_HEADER"
  for fixture in "${selection[@]}"; do
    case "$mode" in
      bench)
        run_one_fixture_backend "$mode" "$fixture" treewalk
        ;;
      bench-vm)
        run_one_fixture_backend "$mode" "$fixture" vm
        ;;
      bench-compare)
        run_one_fixture_backend "$mode" "$fixture" treewalk
        run_one_fixture_backend "$mode" "$fixture" vm
        ;;
    esac
  done
}

assert_command_fails() {
  local expected=$1
  shift
  local stdout_file stderr_file
  stdout_file=$(mktemp)
  stderr_file=$(mktemp)
  if "$@" >"$stdout_file" 2>"$stderr_file"; then
    rm -f "$stdout_file" "$stderr_file"
    die "expected command to fail: $*"
  fi
  grep -F "$expected" "$stderr_file" >/dev/null || {
    echo "--- stdout ---" >&2
    cat "$stdout_file" >&2
    echo "--- stderr ---" >&2
    cat "$stderr_file" >&2
    rm -f "$stdout_file" "$stderr_file"
    die "failure output did not include expected text '$expected' for command: $*"
  }
  rm -f "$stdout_file" "$stderr_file"
}

run_smoke() {
  local bench_out vm_out compare_out filtered_out header fixture_count vm_rows compare_rows gc_rows filtered_rows expected_vm_rows expected_compare_rows
  [[ -x "$BINARY" ]] || die "jsrt binary not found or not executable: $BINARY"
  load_manifest

  fixture_count=${#FIXTURES[@]}
  (( fixture_count >= 5 )) || die "smoke expected at least 5 manifest fixtures, found $fixture_count"
  expected_vm_rows=0
  expected_compare_rows=0
  for fixture in "${FIXTURES[@]}"; do
    if contains_backend "${META_SUPPORTED[$fixture]}" vm; then
      expected_vm_rows=$((expected_vm_rows + 1))
    fi
    if [[ "${META_COMPARE[$fixture]}" == "yes" ]] && contains_backend "${META_SUPPORTED[$fixture]}" treewalk && contains_backend "${META_SUPPORTED[$fixture]}" vm; then
      expected_compare_rows=$((expected_compare_rows + 2))
    fi
  done

  bench_out=$("$0" bench --iterations 1 --warmups 0)
  vm_out=$("$0" bench-vm --iterations 1 --warmups 0)
  compare_out=$("$0" bench-compare --iterations 1 --warmups 0)
  filtered_out=$("$0" bench --iterations 1 --warmups 0 --filter vm_subset)

  header=$(printf '%s\n' "$bench_out" | head -n 1)
  [[ "$header" == "$OUTPUT_HEADER" ]] || die "bench header mismatch"
  header=$(printf '%s\n' "$vm_out" | head -n 1)
  [[ "$header" == "$OUTPUT_HEADER" ]] || die "bench-vm header mismatch"
  header=$(printf '%s\n' "$compare_out" | head -n 1)
  [[ "$header" == "$OUTPUT_HEADER" ]] || die "bench-compare header mismatch"

  vm_rows=$(printf '%s\n' "$vm_out" | tail -n +2 | wc -l | tr -d ' ')
  compare_rows=$(printf '%s\n' "$compare_out" | tail -n +2 | wc -l | tr -d ' ')
  gc_rows=$(printf '%s\n' "$vm_out" | grep -c 'benchmarks/gc_churn.js' || true)
  filtered_rows=$(printf '%s\n' "$filtered_out" | tail -n +2 | wc -l | tr -d ' ')

  (( vm_rows == expected_vm_rows )) || die "bench-vm row count did not match manifest-approved vm fixtures"
  (( compare_rows == expected_compare_rows )) || die "bench-compare row count did not match compare-enabled manifest fixtures"
  (( gc_rows == 0 )) || die "bench-vm unexpectedly included treewalk-only gc fixture"
  (( filtered_rows == 1 )) || die "bench filter should narrow the corpus to one vm_subset fixture"

  printf '%s\n' "$bench_out" | grep -F 'benchmarks/gc_churn.js,gc_churn,treewalk,no,1,0,' >/dev/null || die "bench output missing gc_churn treewalk row"
  printf '%s\n' "$bench_out" | grep -F ',ok,na' >/dev/null || die "bench output missing exit_zero smoke row with stdout_match=na"
  printf '%s\n' "$compare_out" | grep -F 'bench-compare,benchmarks/vm_subset.js,vm_subset,vm,yes,1,0,' >/dev/null || die "bench-compare output missing vm compare row"
  printf '%s\n' "$filtered_out" | grep -F 'benchmarks/vm_subset.js' >/dev/null || die "bench filter output did not include the vm_subset fixture"

  assert_command_fails "no fixtures matched mode 'bench' and filter 'does-not-exist'" "$0" bench --iterations 1 --warmups 0 --filter does-not-exist
  assert_command_fails "fixture is not compare-enabled: benchmarks/gc_churn.js" "$0" bench-compare --iterations 1 --warmups 0 --fixture benchmarks/gc_churn.js
  assert_command_fails "bench mode only supports backend 'treewalk'" "$0" bench --iterations 1 --warmups 0 --backend vm
  assert_command_fails "bench-vm mode only supports backend 'vm'" "$0" bench-vm --iterations 1 --warmups 0 --backend treewalk
  assert_command_fails "unknown option: --bogus" "$0" bench --bogus

  echo "bench-smoke: ok"
}

main() {
  parse_args "$@"
  case "$MODE" in
    bench|bench-vm|bench-compare)
      run_mode "$MODE"
      ;;
    bench-smoke)
      run_smoke
      ;;
    *)
      die "unknown benchmark mode: $MODE"
      ;;
  esac
}

main "$@"

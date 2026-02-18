#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SANITIZER="${1:-address}"
BUILD_DIR="${2:-$ROOT_DIR/build-p4-${SANITIZER}}"

case "$SANITIZER" in
  address|thread|undefined) ;;
  *)
    echo "Unsupported sanitizer: $SANITIZER"
    echo "Usage: $0 [address|thread|undefined] [build_dir]"
    exit 2
    ;;
esac

: "${LVRS_PERF_GATE_ROUNDS:=3}"
: "${LVRS_PERF_GATE_TASKS:=36}"
: "${LVRS_PERF_GATE_WORK_MS:=1}"
: "${LVRS_PERF_GATE_P95_MS:=180}"
: "${LVRS_PERF_GATE_P99_MS:=260}"
: "${LVRS_SOAK_ITERATIONS:=128}"

export LVRS_PERF_GATE_ROUNDS
export LVRS_PERF_GATE_TASKS
export LVRS_PERF_GATE_WORK_MS
export LVRS_PERF_GATE_P95_MS
export LVRS_PERF_GATE_P99_MS
export LVRS_SOAK_ITERATIONS

if [[ "$SANITIZER" == "address" ]]; then
  if [[ "$(uname -s)" == "Darwin" ]]; then
    export ASAN_OPTIONS="detect_leaks=0:halt_on_error=1:strict_init_order=1"
  else
    export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_init_order=1"
  fi
  export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"
elif [[ "$SANITIZER" == "undefined" ]]; then
  export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"
else
  export TSAN_OPTIONS="halt_on_error=1:history_size=7"
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -DLVRS_BUILD_TESTS=ON \
  -DLVRS_SANITIZER="$SANITIZER" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build "$BUILD_DIR" --target \
  LVRSTests_backend_io \
  LVRSTests_runtime_services \
  LVRSTests_performance_gate \
  LVRSTests_soak_runtime

ctest --test-dir "$BUILD_DIR" --output-on-failure \
  -R "LVRSTests_backend_io|LVRSTests_runtime_services|LVRSTests_performance_gate|LVRSTests_soak_runtime"

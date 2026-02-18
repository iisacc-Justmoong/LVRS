#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build-p4-quality}"

: "${LVRS_PERF_GATE_ROUNDS:=5}"
: "${LVRS_PERF_GATE_TASKS:=48}"
: "${LVRS_PERF_GATE_WORK_MS:=2}"
: "${LVRS_PERF_GATE_P95_MS:=120}"
: "${LVRS_PERF_GATE_P99_MS:=180}"
: "${LVRS_VISUAL_DIFF_CHANNEL_TOLERANCE:=8}"
: "${LVRS_VISUAL_DIFF_RATIO_MAX:=0.001}"

export LVRS_PERF_GATE_ROUNDS
export LVRS_PERF_GATE_TASKS
export LVRS_PERF_GATE_WORK_MS
export LVRS_PERF_GATE_P95_MS
export LVRS_PERF_GATE_P99_MS
export LVRS_VISUAL_DIFF_CHANNEL_TOLERANCE
export LVRS_VISUAL_DIFF_RATIO_MAX

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DLVRS_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --target \
    LVRSTests_performance_gate \
    LVRSTests_visual_regression \
    LVRSTests_soak_runtime

ctest --test-dir "$BUILD_DIR" --output-on-failure -L "p4" -LE "long"

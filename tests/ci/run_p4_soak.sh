#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build-p4-soak}"

: "${LVRS_SOAK_ITERATIONS:=5000}"
: "${LVRS_SOAK_WORK_MS:=1}"
: "${LVRS_SOAK_TIMEOUT_MS:=900000}"
: "${LVRS_SOAK_P99_LIMIT_MS:=800}"

export LVRS_SOAK_ITERATIONS
export LVRS_SOAK_WORK_MS
export LVRS_SOAK_TIMEOUT_MS
export LVRS_SOAK_P99_LIMIT_MS

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DLVRS_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --target LVRSTests_soak_runtime
ctest --test-dir "$BUILD_DIR" --output-on-failure -L "soak"

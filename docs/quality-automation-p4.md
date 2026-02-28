# P4 Quality Automation Operations Guide

Location: `tests/` / `tests/ci/` / `CMakeLists.txt`

This document defines the performance, visual, and stability automation framework introduced at P4.

## 1. P4 Status

| ID | Item | Implementation Status | Implementation Location |
|---|---|---|---|
| `P4-01` | Performance regression CI (p95/p99 thresholds) | Complete | `tests/tst_performance_gate.cpp`, `tests/ci/run_p4_quality.sh` |
| `P4-02` | Visual regression (golden image) | Complete | `tests/tst_visual_regression.cpp`, `tests/golden/visual_baseline_scene.png` |
| `P4-03` | TSAN/ASAN/UBSAN pipeline | Complete | `CMakeLists.txt` (`LVRS_SANITIZER`), `tests/ci/run_p4_sanitizers.sh` |
| `P4-04` | Long-running soak test automation | Complete | `tests/tst_soak_runtime.cpp`, `tests/ci/run_p4_soak.sh` |

## 2. Test Target Summary

### `LVRSTests_performance_gate`

- Purpose: block regressions in the `dispatchTask` latency distribution using p95/p99 thresholds.
- Method: evaluate multiple measurement rounds and gate by median(p95), median(p99).
- Key environment variables:
  - `LVRS_PERF_GATE_ROUNDS`
  - `LVRS_PERF_GATE_TASKS`
  - `LVRS_PERF_GATE_WORK_MS`
  - `LVRS_PERF_GATE_P95_MS`
  - `LVRS_PERF_GATE_P99_MS`

### `LVRSTests_visual_regression`

- Purpose: compare rendered output of the baseline scene against a golden image.
- Method: compute mismatch ratio using per-channel pixel tolerance.
- Key environment variables:
  - `LVRS_VISUAL_DIFF_CHANNEL_TOLERANCE`
  - `LVRS_VISUAL_DIFF_RATIO_MAX`
  - `LVRS_UPDATE_GOLDEN=1` (update golden)

### `LVRSTests_soak_runtime`

- Purpose: monitor queue leaks, cache overflow, and latency degradation under repeated IO+async load.
- Method: after repeated work, validate queue drain, backpressure drops, p99 upper bound, and cache/trace limits.
- Key environment variables:
  - `LVRS_SOAK_ITERATIONS`
  - `LVRS_SOAK_WORK_MS`
  - `LVRS_SOAK_TIMEOUT_MS`
  - `LVRS_SOAK_P99_LIMIT_MS`

## 3. Label Scheme

P4 tests are categorized by CTest labels.

- `p4`
- `quality`
- `ci`
- `performance`
- `visual`
- `soak`
- `long`

Recommended execution:

- PR/general gate: `ctest -L p4 -LE long`
- Weekly soak: `ctest -L soak`

## 4. Execution Scripts

### 4.1 PR Quality Gate

```bash
./tests/ci/run_p4_quality.sh
```

Behavior:

1. Configure a test build.
2. Build P4 gate targets.
3. Run `p4` label tests excluding `long`.

### 4.2 Sanitizer Matrix

```bash
./tests/ci/run_p4_sanitizers.sh address
./tests/ci/run_p4_sanitizers.sh undefined
./tests/ci/run_p4_sanitizers.sh thread
```

Behavior:

1. Configure a dedicated build using `LVRS_SANITIZER`.
2. Build core stability/P4 test targets.
3. Run CTest with sanitizer runtime options.

### 4.3 Soak Batch

```bash
LVRS_SOAK_ITERATIONS=5000 ./tests/ci/run_p4_soak.sh
```

Behavior:

1. Configure a soak-specific build.
2. Build soak test targets.
3. Run tests with the `soak` label.

## 5. CMake Sanitizer Option

- New option: `LVRS_SANITIZER`
- Allowed values: `none`, `address`, `thread`, `undefined`

Example:

```bash
cmake -S . -B build-asan -DLVRS_BUILD_TESTS=ON -DLVRS_SANITIZER=address
cmake --build build-asan --target LVRSTests_backend_io
ctest --test-dir build-asan --output-on-failure -R LVRSTests_backend_io
```

## 6. Golden Image Operation Rules

- Golden file path: `tests/golden/visual_baseline_scene.png`
- Update with `LVRS_UPDATE_GOLDEN=1` only when the UI change is intentional.
- PRs that update the golden image must include the reason/evidence for the diff.

## 7. Definition of Done

P4 is considered complete only when all of the following pass.

1. `run_p4_quality.sh`
2. `run_p4_sanitizers.sh address`
3. `run_p4_sanitizers.sh undefined`
4. `run_p4_soak.sh` (under policy-defined iteration count)
5. zero golden-image comparison failures

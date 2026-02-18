# P0 Baseline Report

Report timestamp (UTC): `2026-02-18T03:13:34Z`

## Environment

| Item | Value |
|---|---|
| Host | `Darwin Mac-Studio 25.3.0` |
| OS | `macOS 26.3 (25D125)` |
| CPU | `Apple M1 Max` |
| Memory | `34359738368 bytes (32 GiB)` |
| GPU/Metal | `Apple M1 Max / Metal 4` |

## Build/Test Context

| Item | Value |
|---|---|
| Build dir | `build-codex` |
| Qt runtime | `Qt 6.8.3` |
| Test binary 1 | `LVRSTests_backend_io` |
| Test binary 2 | `LVRSTests_runtime_services` |

## Measured Results

### 1) `LVRSTests_backend_io`

| Metric | Value |
|---|---|
| Qt summary | `8 passed, 0 failed, 430ms` |
| Wall clock (`time -lp`) | `real 0.58s` |
| Max RSS (`time -lp`) | `71335936 bytes` |
| Peak memory footprint (`time -lp`) | `14584064 bytes` |
| Involuntary context switches | `997` |

### 2) `LVRSTests_runtime_services`

| Metric | Value |
|---|---|
| Qt summary | `11 passed, 0 failed, 571ms` |
| Wall clock (`time -lp`) | `real 0.72s` |
| Max RSS (`time -lp`) | `71483392 bytes` |
| Peak memory footprint (`time -lp`) | `14600512 bytes` |
| Involuntary context switches | `995` |

## Notes

- This report is a P0 baseline snapshot for relative comparison in future optimization phases.
- Use identical machine/profile/settings for longitudinal comparisons.
- p95/p99 regression checks should be compared against this baseline using the same test commands.

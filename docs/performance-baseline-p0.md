<a id="p0-baseline-report"></a>

# P0 기준 보고서

보고서 타임스탬프(UTC): `2026-02-18T03:13:34Z`

<a id="environment"></a>

## 환경

|품목|값|
|---|---|
|호스트| `Darwin Mac-Studio 25.3.0` |
| OS | `macOS 26.3 (25D125)` |
| CPU | `Apple M1 Max` |
|메모리| `34359738368 bytes (32 GiB)` |
| GPU/Metal | `Apple M1 Max / Metal 4` |

<a id="buildtest-context"></a>

## 컨텍스트 구축/테스트

|품목|값|
|---|---|
|빌드 디렉토리| `build-codex` |
| Qt runtime | `Qt 6.8.3` |
|테스트 바이너리 1| `LVRSTests_backend_io` |
|테스트 바이너리 2| `LVRSTests_runtime_services` |

<a id="measured-results"></a>

## 측정 결과

### 1) `LVRSTests_backend_io`

|미터법|값|
|---|---|
|Qt 요약| `8 passed, 0 failed, 430ms` |
|벽시계 (`time -lp`)| `real 0.58s` |
|최대 RSS (`time -lp`)| `71335936 bytes` |
|최대 메모리 공간(`time -lp`)| `14584064 bytes` |
|비자발적 컨텍스트 스위치| `997` |

### 2) `LVRSTests_runtime_services`

|미터법|값|
|---|---|
|Qt 요약| `11 passed, 0 failed, 571ms` |
|벽시계 (`time -lp`)| `real 0.72s` |
|최대 RSS (`time -lp`)| `71483392 bytes` |
|최대 메모리 공간(`time -lp`)| `14600512 bytes` |
|비자발적 컨텍스트 스위치| `995` |

<a id="notes"></a>

## 메모

- 이 보고서는 향후 최적화 단계의 상대적 비교를 위한 P0 기준 스냅샷입니다.
- 종단적 비교를 위해 동일한 기계/프로파일/설정을 사용하십시오.
- p95/p99 회귀 검사는 동일한 테스트 명령을 사용하여 이 기준과 비교되어야 합니다.

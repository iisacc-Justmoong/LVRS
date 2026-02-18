# P4 품질 자동화 운영 가이드

Location: `tests/` / `tests/ci/` / `CMakeLists.txt`

본 문서는 P4 단계에서 도입된 성능·시각·안정성 자동화 체계를 정의한다.

## 1. P4 상태

| ID | 항목 | 구현 상태 | 구현 지점 |
|---|---|---|---|
| `P4-01` | 성능 회귀 CI(p95/p99 임계치) | 완료 | `tests/tst_performance_gate.cpp`, `tests/ci/run_p4_quality.sh` |
| `P4-02` | 시각 회귀(골든 이미지) | 완료 | `tests/tst_visual_regression.cpp`, `tests/golden/visual_baseline_scene.png` |
| `P4-03` | TSAN/ASAN/UBSAN 파이프라인 | 완료 | `CMakeLists.txt` (`LVRS_SANITIZER`), `tests/ci/run_p4_sanitizers.sh` |
| `P4-04` | 장시간 soak test 자동화 | 완료 | `tests/tst_soak_runtime.cpp`, `tests/ci/run_p4_soak.sh` |

## 2. 테스트 타깃 요약

### `LVRSTests_performance_gate`

- 목적: `dispatchTask` 지연 분포의 회귀를 p95/p99 임계치로 차단.
- 방법: 다회 라운드 측정 후 median(p95), median(p99) 기준 판정.
- 주요 환경변수:
  - `LVRS_PERF_GATE_ROUNDS`
  - `LVRS_PERF_GATE_TASKS`
  - `LVRS_PERF_GATE_WORK_MS`
  - `LVRS_PERF_GATE_P95_MS`
  - `LVRS_PERF_GATE_P99_MS`

### `LVRSTests_visual_regression`

- 목적: 기준 장면 렌더 결과와 골든 이미지 비교.
- 방법: 픽셀 채널 오차 허용치 기반 mismatch ratio 계산.
- 주요 환경변수:
  - `LVRS_VISUAL_DIFF_CHANNEL_TOLERANCE`
  - `LVRS_VISUAL_DIFF_RATIO_MAX`
  - `LVRS_UPDATE_GOLDEN=1` (골든 갱신)

### `LVRSTests_soak_runtime`

- 목적: 반복 IO+async 부하에서 큐 누수/캐시 초과/지연 열화 감시.
- 방법: 반복 작업 후 queue drain, backpressure drop, p99 상한, cache/trace 한계 검증.
- 주요 환경변수:
  - `LVRS_SOAK_ITERATIONS`
  - `LVRS_SOAK_WORK_MS`
  - `LVRS_SOAK_TIMEOUT_MS`
  - `LVRS_SOAK_P99_LIMIT_MS`

## 3. 라벨 체계

P4 테스트는 CTest 라벨로 분류된다.

- `p4`
- `quality`
- `ci`
- `performance`
- `visual`
- `soak`
- `long`

권장 실행:

- PR/일반 게이트: `ctest -L p4 -LE long`
- 주간 soak: `ctest -L soak`

## 4. 실행 스크립트

### 4.1 PR 품질 게이트

```bash
./tests/ci/run_p4_quality.sh
```

동작:

1. 테스트 빌드 구성
2. P4 게이트 타깃 빌드
3. `p4` 라벨 중 `long` 제외 테스트 실행

### 4.2 Sanitizer 매트릭스

```bash
./tests/ci/run_p4_sanitizers.sh address
./tests/ci/run_p4_sanitizers.sh undefined
./tests/ci/run_p4_sanitizers.sh thread
```

동작:

1. `LVRS_SANITIZER` 기반 별도 빌드 구성
2. 핵심 안정성/P4 테스트 빌드
3. CTest 실행 및 sanitizer 런타임 옵션 적용

### 4.3 Soak 배치

```bash
LVRS_SOAK_ITERATIONS=5000 ./tests/ci/run_p4_soak.sh
```

동작:

1. soak 전용 빌드 구성
2. soak 테스트 타깃 빌드
3. `soak` 라벨 테스트 실행

## 5. CMake sanitizer 옵션

- 신규 옵션: `LVRS_SANITIZER`
- 허용값: `none`, `address`, `thread`, `undefined`

예시:

```bash
cmake -S . -B build-asan -DLVRS_BUILD_TESTS=ON -DLVRS_SANITIZER=address
cmake --build build-asan --target LVRSTests_backend_io
ctest --test-dir build-asan --output-on-failure -R LVRSTests_backend_io
```

## 6. 골든 이미지 운영 규칙

- 골든 파일 경로: `tests/golden/visual_baseline_scene.png`
- UI 의도 변경 시에만 `LVRS_UPDATE_GOLDEN=1`로 갱신한다.
- 골든 갱신 PR에는 diff 근거(변경 의도)를 반드시 포함한다.

## 7. 완료 정의

P4 완료 판정은 다음을 모두 만족할 때 성립한다.

1. `run_p4_quality.sh` 성공
2. `run_p4_sanitizers.sh address` 성공
3. `run_p4_sanitizers.sh undefined` 성공
4. `run_p4_soak.sh` 성공(정책 반복 횟수 기준)
5. 골든 이미지 비교 실패 0건

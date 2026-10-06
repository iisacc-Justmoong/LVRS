<a id="p4-quality-automation-operations-guide"></a>

# P4 품질 자동화 운영 가이드

위치: `tests/` / `tests/ci/` / `CMakeLists.txt`

이 문서는 P4에 도입된 성능, 시각적 및 안정성 자동화 프레임워크를 정의합니다.

<a id="1-p4-status"></a>

## 1. P4 상태

| ID |품목|구현 상태|구현 위치|
|---|---|---|---|
| `P4-01` |성능 회귀 CI(p95/p99 임계값)|완료| `tests/tst_performance_gate.cpp`, `tests/ci/run_p4_quality.sh` |
| `P4-02` |시각적 회귀(골든 이미지)|완료| `tests/tst_visual_regression.cpp`, `tests/golden/visual_baseline_scene.png` |
| `P4-03` |TSAN/ASAN/UBSAN 파이프라인|완료| `CMakeLists.txt` (`LVRS_SANITIZER`), `tests/ci/run_p4_sanitizers.sh` |
| `P4-04` |장기 담금 테스트 자동화|완료| `tests/tst_soak_runtime.cpp`, `tests/ci/run_p4_soak.sh` |

<a id="2-test-target-summary"></a>

## 2. 테스트 대상 요약

### `LVRSTests_performance_gate`

- 목적: p95/p99 임계값을 사용하여 `dispatchTask` 대기 시간 분포의 회귀를 차단합니다.
- 방법: 여러 측정 라운드를 평가하고 중앙값(p95), 중앙값(p99)을 기준으로 게이트를 지정합니다.
- 주요 환경 변수:
  - `LVRS_PERF_GATE_ROUNDS`
  - `LVRS_PERF_GATE_TASKS`
  - `LVRS_PERF_GATE_WORK_MS`
  - `LVRS_PERF_GATE_P95_MS`
  - `LVRS_PERF_GATE_P99_MS`

### `LVRSTests_visual_regression`

- 목적: 기본 장면의 렌더링된 출력을 골든 이미지와 비교합니다.
- 방법: 채널별 픽셀 허용오차를 사용하여 불일치 비율을 계산합니다.
- 주요 환경 변수:
  - `LVRS_VISUAL_DIFF_CHANNEL_TOLERANCE`
  - `LVRS_VISUAL_DIFF_RATIO_MAX`
  - `LVRS_UPDATE_GOLDEN=1`(골든 업데이트)

### `LVRSTests_soak_runtime`

- 목적: 반복되는 IO+비동기 로드에서 대기열 누수, 캐시 오버플로 및 대기 시간 저하를 모니터링합니다.
- 방법: 반복 작업 후 대기열 배수, 배압 강하, p99 상한 및 캐시/추적 제한을 검증합니다.
- 주요 환경 변수:
  - `LVRS_SOAK_ITERATIONS`
  - `LVRS_SOAK_WORK_MS`
  - `LVRS_SOAK_TIMEOUT_MS`
  - `LVRS_SOAK_P99_LIMIT_MS`

<a id="3-label-scheme"></a>

## 3. 라벨 구성표

P4 테스트는 CTest 라벨로 분류됩니다.

- `p4`
- `quality`
- `ci`
- `performance`
- `visual`
- `soak`
- `long`

권장 실행:

- PR/일반 게이트: `ctest -L p4 -LE long`
- 주간 담그기: `ctest -L soak`

<a id="4-execution-scripts"></a>

## 4. 실행 스크립트

<a id="41-pr-quality-gate"></a>

### 4.1 PR 품질 게이트

```bash
./tests/ci/run_p4_quality.sh
```

행동:

1. 테스트 빌드를 구성합니다.
2. P4 게이트 타겟을 구축하세요.
3. `long`를 제외하고 `p4` 라벨 테스트를 실행합니다.

<a id="42-sanitizer-matrix"></a>

### 4.2 살균제 매트릭스

```bash
./tests/ci/run_p4_sanitizers.sh address
./tests/ci/run_p4_sanitizers.sh undefined
./tests/ci/run_p4_sanitizers.sh thread
```

행동:

1. `LVRS_SANITIZER`를 사용하여 전용 빌드를 구성합니다.
2. 코어 안정성/P4 테스트 타겟을 구축하세요.
3. 소독제 런타임 옵션과 함께 CTest를 실행합니다.

<a id="43-soak-batch"></a>

### 4.3 담금 배치

```bash
LVRS_SOAK_ITERATIONS=5000 ./tests/ci/run_p4_soak.sh
```

행동:

1. 흡수 전용 빌드를 구성합니다.
2. 흡수 테스트 대상을 구축합니다.
3. `soak` 라벨로 테스트를 실행하세요.

<a id="5-cmake-sanitizer-option"></a>

## 5. CMake 살균제 옵션

- 새로운 옵션: `LVRS_SANITIZER`
- 허용되는 값: `none`, `address`, `thread`, `undefined`

예:

```bash
cmake -S . -B build-asan -DLVRS_BUILD_TESTS=ON -DLVRS_SANITIZER=address
cmake --build build-asan --target LVRSTests_backend_io
ctest --test-dir build-asan --output-on-failure -R LVRSTests_backend_io
```

<a id="6-golden-image-operation-rules"></a>

## 6. 골든 이미지 운영 규칙

- 골든 파일 경로: `tests/golden/visual_baseline_scene.png`
- UI 변경이 의도적인 경우에만 `LVRS_UPDATE_GOLDEN=1`로 업데이트하세요.
- 골든 이미지를 업데이트하는 PR에는 차이점에 대한 이유/증거가 포함되어야 합니다.

<a id="7-definition-of-done"></a>

## 7. 완료의 정의

P4는 다음이 모두 통과된 경우에만 완료된 것으로 간주됩니다.

1. `run_p4_quality.sh`
2. `run_p4_sanitizers.sh address`
3. `run_p4_sanitizers.sh undefined`
4. `run_p4_soak.sh`(정책에 정의된 반복 횟수 미만)
5. 0 골드 이미지 비교 실패

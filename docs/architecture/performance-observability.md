<a id="performance-observability-p0"></a>

# 성능 관찰성(P0)

위치: `src/backend/io/backend.*` / `src/backend/runtime/renderingmonitor.*`

이 문서는 LVRS에서 사용하는 P0 성능 원격 측정 계약을 정의합니다.

<a id="purpose"></a>

## 목적

- CPU/GPU 인접 런타임 원격 측정에 대한 하나의 공유 지표 스키마를 설정합니다.
- 대기열/시작/종료 분석을 위해 요청별 비동기 타임라인 추적을 제공합니다.
- 샘플링 오버헤드 한계가 설정된를 유지하고 디버그 빌드에 대해 예측 가능합니다.

<a id="schemas"></a>

## 스키마

<a id="1-snapshot-schema-lvrsperformancev1"></a>

### 1) 스냅샷 스키마: `lvrs.performance.v1`

사용처:

- `Backend.performanceMetrics()`
- `RenderMonitor.performanceSnapshot()`

공통 필수 필드:

- `schema`
- `component`
- `epochMs`

백엔드 관련 필드:

- `asyncJobsInFlight`
- `asyncMaxConcurrency`
- `asyncIoMaxConcurrency`
- `asyncUtilityMaxConcurrency`
- `asyncRenderMaxConcurrency`
- `asyncQueueDepth`
- `asyncQueuePeakDepth`
- `asyncQueueDepthLimit`
- `asyncBackpressureDropCount`
- `asyncMergedRequestCount`
- `asyncCanceledRequestCount`
- `performanceTraceCount`
- `performanceTraceCapacity`
- `readTextCacheTtlMs`
- `readTextCacheCapacityBytes`
- `readTextCacheBytes`
- `readTextCacheEntryCount`
- `asyncLaneMetrics`
- `asyncLatencyByOperation` (`avg/p50/p95/p99/max/failureRate`)

RenderMonitor 관련 필드:

- `active`
- `fps`
- `lastFrameMs`
- `avgFrameMs`
- `p95FrameMs`
- `p99FrameMs`
- `frameCount`
- `droppedFrameCount`
- `droppedFrameThresholdMs`
- `recentSampleCount`
- `frameSampleCapacity`

<a id="2-timeline-trace-schema-lvrsperformancetracev1"></a>

### 2) 타임라인 추적 스키마: `lvrs.performance.trace.v1`

사용처:

- `Backend.recentPerformanceTrace()`

필수 입력 사항:

- `schema`
- `sequence`(단조 진행 중)
- `epochMs`
- `phase` (`queued`, `started`, `finished`, `canceled`)
- `requestId`
- `operation`
- `subject`
- `detail`(단계별 맵)

<a id="timeline-semantics"></a>

## 타임라인 의미론

- `queued`: 요청이 승인되고 `requestId`가 할당되었습니다.
- `started`: 작업자 스레드가 실제 작업 본문을 시작했습니다.
- `finished`: 작업이 완료되고 완료 신호가 발생했습니다.
- `canceled`: 취소 토큰이 요청되었습니다.

요청당 예상 순서:

1. `queued`
2. `started`(즉각적 빠른 경로에는 없을 수 있음)
3. `finished`

<a id="overhead-policy"></a>

## 간접비 정책

- 추적 보존은 `performanceTraceCapacity`의 한계가 설정된입니다.
- 대기 시간 샘플은 작업당 한계가 설정된(`m_asyncLatencySampleCapacity`)입니다.
- 렌더링 백분위수는 롤링 샘플 용량(`frameSampleCapacity`)을 기준으로 한계가 설정된입니다.

<a id="operational-usage"></a>

## 운영 사용량

1. 측정 창을 위해 `Backend.performanceMetrics()` 및 `RenderMonitor.performanceSnapshot()`를 함께 캡처합니다.
2. 대기열/대기 병목 현상 분석을 위해 `Backend.recentPerformanceTrace()`를 내보냅니다.
3. 평균뿐만 아니라 개정판 전체에서 p95/p99를 비교하십시오.

<a id="p4-quality-gate-integration"></a>

## P4 품질 게이트 통합

P4 자동화된 품질 게이트는 다음을 통해 이 스키마를 사용합니다.

- `tests/tst_performance_gate.cpp`(p95/p99 회귀 확인),
- `tests/ci/run_p4_quality.sh`(PR 게이트 진입점),
- `tests/ci/run_p4_sanitizers.sh`(동일한 메트릭 계약을 가진 살균제 매트릭스).

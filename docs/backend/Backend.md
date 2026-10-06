<a id="backend"></a>

# 백엔드

위치: `src/backend/io/backend.h` / `src/backend/io/backend.cpp`

`Backend`는 파일 시스템 유틸리티 및 미러링된 런타임 이벤트 캐싱을 위한 QML 브리지 싱글턴입니다.   의도적으로 단순합니다. 정책이 많은 분석이 아닌 전송 및 한계가 설정된 캐싱입니다.

<a id="1-scope-and-non-scope"></a>

## 1. 범위 및 비범위

`Backend` 소유:

- 원자적 텍스트 저장/읽기 도우미,
- 비차단 UI 흐름을 위한 비동기 작업자 풀 기반 파일/작업 디스패치,
- 쓰기 가능한 위치 조회 브리지,
- 런타임-이벤트 후크 수명주기가 `RuntimeEvents`로,
- 유형별 카운터가 있는 한계가 설정된 미러링 이벤트 캐시,
- 마지막으로 알려진 입력 상태 릴레이.

`Backend`는 ****를 소유하지 않습니다.

- 전역 이벤트 캡처 자체(`RuntimeEvents`가 이를 소유함),
- 텍스트 읽기/쓰기 이상의 파일 형식 구문 분석 의미,
- 장기 지속성/이력 저장.

<a id="2-property-api-contract"></a>

## 2. 속성 API 계약

|속성|유형|의미 체계|에지/클램프|
|---|---|---|---|
| `lastError` | `string` |마지막 작업 오류 메시지입니다. 운전 시작 시 클리어됩니다.|빈 문자열은 현재 오류가 없음을 의미합니다.|
| `userEventHooked` | `bool` |런타임 이벤트 스트림에 연결된 경우 참입니다.|런타임가 파괴되거나 연결 해제되면 거짓입니다.|
| `hookedEventCount` | `int` |현재 미러링된 이벤트 수입니다.|`hookedEventCapacity`를 상한으로 제한한다.|
| `hookedEventCapacity` | `int` |유지할 최대 미러링 이벤트입니다.|범위로 제한한다: `[64, 32768]`.|
| `lastHookedEvent` | `map` |가장 최근에 미러링된 이벤트입니다(`hookEpochMs` 사용).|첫 번째 이벤트 이전에는 비어 있습니다.|
| `lastHookedInputState` | `map` |페이로드/런타임 대체 경로의 최신 입력 스냅샷입니다.|후크를 풀면 오래될 수 있습니다.|
| `asyncJobsInFlight` | `int` |대기/실행 중인 비동기 요청 수입니다.|`0`를 하한으로 제한한다.|
| `asyncMaxConcurrency` | `int` |레인 풀을 파생하는 데 사용되는 전역 비동기 동시성 예산입니다.|범위로 제한한다: `[1, 64]`.|
| `asyncIoMaxConcurrency` | `int` |IO 레인 풀의 파생 동시성입니다.|`1`를 하한으로 제한한다.|
| `asyncUtilityMaxConcurrency` | `int` |유틸리티 레인 풀의 파생 동시성입니다.|`1`를 하한으로 제한한다.|
| `asyncRenderMaxConcurrency` | `int` |렌더링 레인 풀의 파생 동시성입니다.|`1`를 하한으로 제한한다.|
| `asyncQueueDepth` | `int` |레인 전체에서 현재 대기 중인 요청 수입니다.|`0`를 하한으로 제한한다.|
| `asyncQueuePeakDepth` | `int` |프로세스 시작 이후 관찰된 최대 대기열 깊이입니다.|단조 증가.|
| `asyncQueueDepthLimit` | `int` |배압에 대한 레인당 대기 깊이 제한.|범위로 제한한다: `[1, 16384]`.|
| `asyncBackpressureDropCount` | `uint64` |배압으로 인해 거부된 요청 수입니다.|단조 증가.|
| `asyncMergedRequestCount` | `uint64` |병합으로 병합된 요청 수입니다.|단조 증가.|
| `asyncCanceledRequestCount` | `uint64` |취소된 것으로 완료된 요청 수입니다.|단조 증가.|
| `performanceMetrics` | `map` |집계된 비동기 대기 시간/큐 지표 스냅샷입니다.|스키마: `lvrs.performance.v1`.|
| `performanceTraceCapacity` | `int` |Max는 성능 추적 이벤트를 유지했습니다.|범위로 제한한다: `[128, 16384]`.|
| `performanceTraceCount` | `int` |현재 보유된 성능 추적 이벤트입니다. 용량별|한계가 설정된.|
| `readTextCacheTtlMs` | `int` |TTL(메모리 내 텍스트 읽기 캐시용)|범위로 제한한다: `[100, 3600000]`.|
| `readTextCacheCapacityBytes` | `int64` |텍스트 캐시의 최대 메모리 예산입니다.|범위로 제한한다: `[64, 536870912]`.|
| `readTextCacheBytes` | `int64` |현재 텍스트 캐시 메모리 사용량입니다.|`0`를 하한으로 제한한다.|
| `readTextCacheEntryCount` | `int` |현재 텍스트 캐시 항목 수입니다.|`0`를 하한으로 제한한다.|

<a id="3-method-contract-detailed"></a>

## 3. 메소드 계약(상세)

<a id="31-file-apis"></a>

### 3.1 파일 API

#### `saveTextFile(path, text): bool`

- 원자 쓰기 의미 체계에 `QSaveFile`를 사용합니다.
- 페이로드 바이트에 대한 UTF-8 인코딩입니다.
- 열기/쓰기/커밋 실패 시 `false`를 반환하고 `lastError`를 설정합니다.

실패 사례:

- 비어 있음/공백 `path`,
- 디렉토리가 누락되어 호출자 워크플로에서 생성할 수 없습니다.
- 허가가 거부되었습니다.
- 부분적인 쓰기/커밋 실패.

#### `readTextFile(path): string`

- 텍스트 파일을 UTF-8 바이트로 읽습니다.
- 실패 시 빈 문자열을 반환하고 `lastError`를 설정합니다.

#### `ensureDir(path): bool`

- 없으면 재귀적으로 디렉토리를 생성합니다 ( `mkpath(".")` ).
- 이미 존재하는 경우 `true`를 반환합니다.
- 실패 시 `false` + `lastError`를 반환합니다.

#### `writableLocation(location): string`

- 기본적으로 `QStandardPaths::writableLocation(location)`를 반환합니다.
- Android / iOS 에서 요청된 위치가 빈 문자열로 해결되면, LVRS 는 다음 순서로 첫 번째 비어 있지 않은 앱 범용 위치로 되돌아감: `DocumentsLocation` -> `AppDataLocation` -> `AppLocalDataLocation` -> `CacheLocation` -> `TempLocation` .
- 호출자는 필요할 때 디렉터리를 생성할 책임이 있습니다.

<a id="32-async-apis-multi-thread-ui-non-blocking"></a>

### 3.2 비동기 API(다중 스레드, UI 비차단)

#### `saveTextFileAsync(path, text): qulonglong`

- 백엔드 작업자 스레드 풀에 원자성 UTF-8 텍스트 저장을 대기열에 추가합니다.
- 요청 ID를 즉시 반환합니다.
- 발생시키는 신호:
  - `asyncRequestQueued(requestId, "saveTextFile", path)`
  - 완료 시 `asyncRequestFinished(...)`.
- 완료 `result` 맵에는 다음이 포함됩니다:
  - `bytes`

#### `readTextFileAsync(path): qulonglong`

- 백엔드 작업자 스레드 풀에서 읽은 UTF-8 텍스트를 대기열에 넣습니다.
- 요청 ID를 즉시 반환합니다.
- 대기 중/완료 신호를 내보냅니다.
- 완료 `result` 맵에는 다음이 포함됩니다:
  - `text`
  - `length`

#### `ensureDirAsync(path): qulonglong`

- 작업자 풀에 재귀 디렉터리 보장(`mkpath(".")`)을 대기열에 추가합니다.
- 요청 ID를 즉시 반환합니다.
- 완료 `result` 맵에는 다음이 포함됩니다:
  - `ensured` (`bool`)

#### `dispatchAsyncTask(taskName, payload = {}, delayMs = 0): qulonglong`

- UI 오케스트레이션을 위한 범용 비동기 작업 디스패처입니다.
- `payload.lane` 또는 작업 이름 접두사를 사용하여 레인 풀로 라우팅합니다.
  - `io`
  - `utility`
  - `render`
- 기본적으로 즉시 실행으로 실행됩니다(`delayMs`는 메타데이터 호환성 필드로 유지됨).
- 선택 사항인 `payload.workMs`는 오케스트레이션/테스트를 위한 한계가 설정된 백그라운드 작업을 시뮬레이션합니다.
- 병합 지원:
  - `payload.coalesce=true`(기본값)를 설정하고,
  - 대기 중인 중복 작업을 병합하려면 `payload.coalesceKey`를 설정하세요.
- 차선 대기열 깊이가 `asyncQueueDepthLimit`를 초과하면 배압이 적용됩니다.
- `delayMs`는 호환성을 위해 메타데이터 입력(`requestedDelayMs`)으로 허용됩니다.
- 요청 ID를 즉시 반환합니다.
- 완료 `result` 맵은 페이로드를 에코하고 추가합니다.
  - `taskName`
  - `delayMs`(항상 `0`)
  - `requestedDelayMs`
  - `lane`
  - `coalesced`
  - `mergedIntoRequestId`(병합 시)

#### `setAsyncMaxConcurrency(value): void`

- 전역 동시성 예산을 제어합니다.
- 런타임는 이 값에서 레인당 풀 크기(`io/utility/render`)를 파생합니다.

#### `setAsyncQueueDepthLimit(value): void`

- 레인 수준 대기 깊이 배압 임계값을 제어합니다.
- 한도를 초과하는 요청은 배압 오류로 인해 거부됩니다.

#### `cancelAsyncRequest(requestId, reason = "Canceled by request"): bool`

- 요청 취소 토큰을 표시합니다.
- 요청이 시작되지 않은 경우 취소된 상태로 즉시 완료됩니다.
- 이미 실행 중인 경우 최종 완료는 취소된 결과로 정규화됩니다.

#### `setPerformanceTraceCapacity(value): void`

- P0 타임라인 추적 크기를 유지하도록 제어합니다.
- 용량이 줄어들면 이전 항목은 FIFO에서 제거됩니다.

#### `setReadTextCacheTtlMs(value): void`

- `readTextFile*`에 대한 TTL 기반 캐시 만료를 제어합니다.
- 만료된 항목은 액세스/업데이트 시 정리됩니다.

#### `setReadTextCacheCapacityBytes(value): void`

- 읽기 캐시 메모리 예산을 바이트 단위로 제어합니다.
- 캐시는 예산 초과 시 TTL + LRU와 유사한 가장 오래된 액세스 제거를 사용합니다.

<a id="33-runtime-hook-lifecycle-apis"></a>

### 3.3 런타임 후크 수명 주기 API

#### `hookUserEvents(): bool`

수명주기:

1. `RuntimeEvents` 싱글턴를 해결합니다.
2. 런타임 데몬을 시작합니다.
3. 이전 후크 링크를 분리합니다.
4. `RuntimeEvents::eventRecorded`에 연결합니다.
5. 런타임 소멸 핸들러를 연결합니다.
6. 백엔드 미러에 런타임 `recentEvents()`를 수집합니다.
7. `userEventHooked=true`를 설정합니다.

런타임 싱글턴를 사용할 수 없는 경우 `false`를 반환합니다.

#### `unhookUserEvents(): void`

- 이벤트 연결을 끊고 연결을 파괴합니다.
- 런타임 포인터를 지웁니다.
- `userEventHooked=false`를 설정합니다.
- 기존 미러링된 이벤트 캐시를 지우지 않습니다.

#### `clearHookedUserEvents(): void`

- 미러링된 이벤트, 유형 카운터, 마지막 후크된 스냅샷을 지웁니다.
- `hookedEventsChanged`를 방출합니다.

<a id="34-query-apis"></a>

### 3.4 쿼리 API

#### `hookedUserEvents(limit = -1): list`

- `limit <= 0` 또는 `limit >= count` -> 전체 목록.
- 양수 제한은 최신 N개 항목을 반환합니다.

#### `hookedUserEventSummary(): map`

포함:

- Hook 상태, 이벤트 개수, 용량,
- 마지막 이벤트,
- 현재 입력 스냅샷,
- 유형별 개수,
- 런타임 이벤트 시퀀스(런타임 포인터가 살아 있을 때).

#### `currentUserInputState(): map`

- 런타임 포인터를 사용할 수 있는 경우 라이브 런타임 입력 상태를 반환합니다.
- Else는 마지막으로 캐시된 입력 상태를 반환합니다.

#### `performanceMetrics(): map`

포함:

- 스키마/구성요소/에포크,
- `asyncJobsInFlight`, `asyncMaxConcurrency`,
- `asyncIoMaxConcurrency`, `asyncUtilityMaxConcurrency`, `asyncRenderMaxConcurrency`,
- `asyncQueueDepth`, `asyncQueuePeakDepth`, `asyncQueueDepthLimit`,
- `asyncBackpressureDropCount`, `asyncMergedRequestCount`, `asyncCanceledRequestCount`,
- `performanceTraceCount`, `performanceTraceCapacity`,
- `readTextCacheTtlMs`, `readTextCacheCapacityBytes`, `readTextCacheBytes`, `readTextCacheEntryCount`,
- `asyncLaneMetrics`(`io/utility/render` 대기 중/실행 중/최대/피크),
- 작업별 대기 시간 요약(`avg`, `p50`, `p95`, `p99`, `max`, `failureRate`).

#### `recentPerformanceTrace(limit = -1): list`

- 최신 성능 타임라인 이벤트를 반환합니다.
- 이벤트 스키마: `lvrs.performance.trace.v1`.
- 일반적인 단계:
  - `queued`
  - `started`
  - `finished`
  - `canceled`

#### `clearPerformanceTrace(): void`

- 보관된 성능 타임라인 이벤트를 지웁니다.

<a id="4-mirrored-cache-semantics"></a>

## 4. 미러링된 캐시 의미론

미러링된 각 이벤트:

- 런타임 이벤트 맵을 복사합니다.
- 미러링 시간에 `hookEpochMs`를 추가합니다.
- 유형별 카운터 업데이트,
- 페이로드 입력이 있는 경우 마지막 입력 스냅샷을 업데이트합니다.
- 용량이 초과되면 가장 오래된 항목을 제거합니다.

퇴거 정책:

- 게재 순서에 따라 엄격한 FIFO,
- 삭제된 항목에 대한 유형별 카운터가 감소합니다.

성능 추적 의미:

- 프로세스 범위의 추가 전용 시퀀스 ID(`sequence`),
- 한계가 설정된 `performanceTraceCapacity`에 의한 FIFO 보존,
- 요청당 단계 순서: `queued -> started -> finished`,
- 취소는 `canceled` 추적 및 `finished(canceled=true)`로 표시됩니다.

<a id="5-error-model"></a>

## 5. 오류 모델

범주별 일반적인 `lastError` 값:

|카테고리|일반적인 트리거|
|---|---|
|인수 검증|빈 경로 입력|
|파일 시스템|열기/읽기/쓰기/커밋 실패|
|런타임 통합|런타임 싱글턴 사용할 수 없음|

`lastError`는 새로운 작업이 시작될 때마다 덮어쓰여집니다. 기록 오류가 필요한 호출자는 외부에서 지속되어야 합니다.

<a id="6-usage-patterns"></a>

## 6. 사용 패턴

<a id="61-runtime-hook--export"></a>

### 6.1 런타임-후크 + 내보내기

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    if (!LV.Backend.hookUserEvents())
        console.warn("hook failed:", LV.Backend.lastError)
}

function exportEvents(path) {
    const payload = JSON.stringify(LV.Backend.hookedUserEvents(512), null, 2)
    if (!LV.Backend.saveTextFile(path, payload))
        console.warn("save failed:", LV.Backend.lastError)
}
```

<a id="62-async-non-blocking-ui-workflow"></a>

### 6.2 비동기 비차단 UI 워크플로

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.Backend.asyncMaxConcurrency = 4
}

Connections {
    target: LV.Backend
    function onAsyncRequestFinished(requestId, operation, subject, ok, result, error, elapsedMs) {
        if (!ok) {
            console.warn(operation, "failed:", error)
            return
        }
        if (operation === "readTextFile")
            console.log("loaded text:", result.text)
    }
}

function refreshModelFromFile(path) {
    LV.Backend.readTextFileAsync(path)
}
```

<a id="63-measurement-window-reset-pattern"></a>

### 6.3 측정 창 재설정 패턴

```qml
function beginMeasurementWindow() {
    LV.Backend.clearHookedUserEvents()
    if (!LV.Backend.userEventHooked)
        LV.Backend.hookUserEvents()
}
```

<a id="7-failuretroubleshooting-matrix"></a>

## 7. 오류/문제 해결 매트릭스

|증상|예상 원인|검증|조치|
|---|---|---|---|
|`hookUserEvents()` 실패|런타임 싱글턴 누락|`lastError` 검사|후크 전에 런타임 모듈 싱글턴 존재 여부 확인|
|반영된 이벤트는 성장하지 않습니다|후크되지 않거나 런타임 이벤트가 없음|`userEventHooked` , `hookedEventCount` 확인|런타임 후크하고 런타임 시작 여부 확인|
|유형별 개수 불일치 예상|용량 제거 발생|개수 대 용량 비교|버스트 창에 대한 용량 증가|
|오래된 입력 상태|런타임 분리됨/후크 해제됨|확인 `userEventHooked`|다시 후크 또는 직접 읽기 런타임 싱글턴|
|저장이 간헐적으로 실패함|경로/권한 문제|`lastError` 콘텐츠 검사|디렉터리 + 권한 + 디스크 상태 확인|
|파일/작업 워크플로 중 UI 끊김 현상|동기화 API가 핫 경로에서 사용되었습니다.|`saveTextFile/readTextFile/ensureDir`에 대한 호출을 검사합니다.|비동기 API로 마이그레이션하고 완료 신호를 처리합니다.|

<a id="8-codex-oriented-playbook"></a>

## 8. Codex 중심 플레이북

<a id="81-safe-patch-order-for-codex"></a>

### 8.1 Codex용 안전한 패치 주문

1. `hookUserEvents()` 수명 주기 순서를 유지합니다(해결 -> 시작 -> 다시 연결 -> 캐시 수집).
2. `hookedEventCapacity` 클램핑 및 FIFO 제거를 유지합니다.
3. 작업 진입점에서 `lastError` 지우기 동작을 유지합니다.
4. `hookedUserEventSummary()`에서 스키마 호환성을 유지합니다.

<a id="82-codex-anti-patterns"></a>

### 8.2 Codex 안티 패턴

- `unhookUserEvents()` 내부의 미러링된 캐시를 자동으로 지우지 마십시오. 이는 API 의미를 변경합니다.
- 런타임 후크 상태에서 파일 API를 차단하지 마십시오.
- `hookEpochMs`를 제거하지 마십시오. 하위 소비 측 툴링은 이에 의존할 수 있습니다.

<a id="83-codex-regression-checklist"></a>

### 8.3 코덱스 회귀 체크리스트

수정 후:

1. 후크/후크 해제가 `userEventHooked`를 올바르게 전환합니다.
2. 용량 클램프 + 제거 동작은 결정적으로 유지됩니다.
3. 비동기 대기열은 안정적인 스키마로 대기/완료 신호를 내보냅니다.
4. 요약 맵에는 예상 키가 포함됩니다.
5. 파일 저장/읽기 실패 경로가 여전히 `lastError`로 설정되어 있습니다.
6. 성능 추적 시퀀스는 단조롭게 유지됩니다.

<a id="9-validation-checklist"></a>

## 9. 검증 체크리스트

- 런타임 후크는 활성 런타임 싱글턴를 사용하여 성공합니다.
- 미러링된 수는 용량 한계를 따릅니다.
- 유형 카운터는 추가 및 제거 모두에서 조정됩니다.
- 비동기 작업은 `asyncJobsInFlight`를 올바르게 업데이트합니다.
- 파일 도우미 API는 원자적으로 작동하고 오류를 올바르게 보고합니다.

<a id="10-related-apis"></a>

## 10. 관련 API

- `RuntimeEvents`: 소스 이벤트 데몬.
- `DebugLogger`: 선택적 이벤트/로그 스트림 관찰자.
- `ApplicationWindow`: 런타임 후크 흐름을 트리거할 수 있는 일반적인 시작 호스트입니다.

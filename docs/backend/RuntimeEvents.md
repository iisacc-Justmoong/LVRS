# RuntimeEvents

위치: `src/backend/runtime/runtimeevents.h` / `src/backend/runtime/runtimeevents.cpp`

`RuntimeEvents`는 LVRS 입력/UI/프로세스 원격 측정을 위한 하위 수준 런타임 데몬 싱글턴입니다.   지연 시간과 세부 정보의 균형을 명시적으로 제어하면서 관찰 가능성을 제공하도록 설계되었습니다.

<a id="1-scope-and-responsibilities"></a>

## 1. 범위와 책임

`RuntimeEvents` 소유:

- 애플리케이션 수준 이벤트 필터 기반 캡처(키보드/포인터/터치/태블릿/제스처/컨텍스트/UI 수명 주기),
- 프로세스 상태 신호(가동 시간, RSS, 활성 상태, 하트비트)
- 정규화된 런타임 이벤트 스트림(`eventRecorded` + 링 버퍼),
- 파생된 스냅샷(`snapshot()`, `daemonHealth()`, `inputState()`),
- 비용 제어(`captureProfile`, 고주파수 스로틀, 포인터 적중 테스트 게이팅)를 포착합니다.

`RuntimeEvents`는 ****를 소유하지 않습니다.

- 상위 수준의 제스처 분류(`GestureEvents`가 누르기/스크롤/유지/드래그/스와이프/네이티브 제스처 정규화를 담당한다),
- 지속성/내보내기 정책(`Backend`는 미러링된 캐시/내보내기를 소유함)
- 렌더링 품질 정책(`RenderQuality`가 이를 소유함),
- UI 레이아웃 의미 체계(`ApplicationWindow`/QML가 이를 소유함)

<a id="2-property-api-detailed-contract"></a>

## 2. 속성 API(상세 계약)

<a id="21-runtime-lifecycle"></a>

### 2.1 런타임 수명주기

|속성|유형|의미 체계|
|---|---|---|
| `running` | `bool` |전역 이벤트 필터 및 타이머가 활성화된 경우 참입니다.|
| `daemonBootEpochMs` | `qint64` |데몬 구성 시간의 Epoch-ms입니다.|
| `eventSequence` | `quint64` |기록된 런타임 이벤트의 단조 시퀀스 번호입니다.|
| `lastEvent` | `map` |마지막으로 기록된 이벤트 페이로드입니다.|

<a id="22-keyboard-domain"></a>

### 2.2 키보드 도메인

|속성|유형|의미 체계|
|---|---|---|
| `keyPressCount`, `keyReleaseCount` | `quint64` |필터로 관찰된 총 키 누름/해제 횟수입니다.|
| `lastKey`, `lastKeyText`, `lastKeyModifiers` | `int/string/int` |마지막 키 메타데이터입니다.|
| `anyKeyPressed` | `bool` |누른 키 세트가 비어 있지 않으면 참입니다.|
| `pressedKeys`, `pressedKeyCodes` | `list/list` |현재 인간 라벨 + 원시 키 코드가 눌러져 있습니다.|

<a id="23-pointer-domain"></a>

### 2.3 포인터 도메인

|속성|유형|의미 체계|
|---|---|---|
| `mouseMoveCount`, `mousePressCount`, `mouseReleaseCount` | `quint64` |포인터 이벤트 카운터.|
| `lastMouseX`, `lastMouseY` | `qreal` |마지막 전역 포인터 좌표입니다.|
| `lastMouseButtons`, `lastMouseModifiers` | `int` |마지막 포인터 버튼/수정자 상태.|
| `mouseButtonPressed` | `bool` |활성 버튼 누르기가 추적되는 동안 참입니다.|
| `pointerUi` | `map` |UI 적중 테스트 스냅샷 또는 대체 경로 맵.|
| `lastMousePressEpochMs`, `lastMouseReleaseEpochMs` | `qint64` |마지막 보도/공개의 에포크 타임스탬프입니다.|
| `mousePressElapsedMs`, `mouseReleaseElapsedMs` | `qint64` |마지막 보도/해제 이후 경과 시간입니다(사용할 수 없는 경우 `-1`).|
| `activePressDurationMs` | `qint64` |현재 활성 언론 기간; 활성 프레스가 없을 때 `0`.|
| `pressedMouseButtonNames` | `list` |현재 버튼 비트마스크에 대한 버튼 레이블입니다.|
| `activeModifiers`, `activeModifierNames` | `int/list` |마우스 + 키보드 상태의 수정자 결합.|

<a id="24-ui-lifecycle-domain"></a>

### 2.4 UI 라이프사이클 도메인

|속성|유형|의미 체계|
|---|---|---|
| `uiCreatedCount`, `uiShownCount`, `uiHiddenCount`, `uiDestroyedCount` | `quint64` |추적된 트리의 UI 수명 주기 카운터|
| `lastUiEvent`, `lastUiObjectName`, `lastUiClassName` | `string` |마지막으로 추적된 UI 수명 주기 메타데이터입니다.|

<a id="25-idleprocessos-domain"></a>

### 2.5 유휴/프로세스/OS 도메인

|속성|유형|의미 체계|
|---|---|---|
| `idle` | `bool` |유휴 상태 플래그입니다.|
| `idleTimeoutMs` | `int` |유휴 임계값. 클램핑된 `[250, 86,400,000]`.|
| `idleForMs` | `qint64` |현재 유휴 기간입니다.|
| `lastActivityEpochMs` | `qint64` |마지막 활동 타임스탬프입니다.|
| `pid`, `processName`, `osName` |스칼라|프로세스/플랫폼 ID 메타데이터입니다.|
| `applicationActive` | `bool` |현재 앱 활성 상태 미러입니다.|
| `osSampleIntervalMs` | `int` |OS 원격 측정 샘플링 간격입니다. 클램핑된 `[250, 60,000]`.|
| `uptimeMs`, `rssBytes` | `qint64` |가동 시간 + 상주 세트 메모리 샘플.|

<a id="26-capture-cost-control-domain"></a>

### 2.6 캡처 비용 제어 도메인

|속성|유형|의미 체계|
|---|---|---|
| `captureProfile` | `int(enum)` |캡처 사전 설정: `FullCapture(0)`, `BalancedCapture(1)`, `LowLatencyCapture(2)`.|
| `pointerHitTestingEnabled` | `bool` |포인터 UI 적중 테스트 작업을 활성화/비활성화합니다.|
| `pointerHitTestMinIntervalMs` | `int` |포인터 적중 테스트 재계산을 위한 최소 간격입니다. 클램핑된 `[0,1000]`.|
| `uiTrackingEnabled` | `bool` |UI 수명 주기 트리 추적 및 관련 이벤트를 활성화/비활성화합니다.|
| `recentEventCapacity` | `int` |링 버퍼 용량. 클램핑된 `[16, 4096]`.|
| `recentEventCount` | `int` |현재 링 버퍼 크기입니다.|

<a id="3-capture-profile-presets"></a>

## 3. 프로필 사전 설정 캡처

`captureProfile`는 낮은 수준의 정책 토글 번들을 적용합니다.

|프로필|고주파 최소 간격|런타임 상태 신호 간격|포인터 적중 테스트|UI 추적|
|---|---:|---:|---|---|
| `FullCapture` | 16ms | 16ms |활성화됨|활성화됨|
| `BalancedCapture` | 24ms | 24ms |활성화됨 (`pointerHitTestMinIntervalMs=16`)|활성화됨|
| `LowLatencyCapture` | 33ms | 33ms |비활성화됨|비활성화됨|

참고:

- 사전 설정은 런타임 전환(디버그 빌드와 프로덕션 대화형 모드)을 위한 것입니다.
- 사전 설정 선택 후 명시적 속성 쓰기가 허용되며 동작을 미세 조정할 수 있습니다.

<a id="4-core-method-contract"></a>

## 4. 핵심 메소드 계약

<a id="41-lifecycle"></a>

### 4.1 수명주기

#### `start()`

- 글로벌 앱 이벤트 필터를 설치합니다.
- 유휴 및 OS 타이머를 시작합니다.
- `daemonStateChanged`, `runningChanged`를 방출합니다.
- `daemon-started` 이벤트를 기록합니다.

#### `stop()`

- 이벤트 필터를 제거합니다.
- 바인딩된 창과 추적된 UI 트리를 지웁니다.
- 타이머를 중지합니다.
- `daemonStateChanged`, `runningChanged`를 방출합니다.
- `daemon-stopped` 이벤트를 기록합니다.

#### `attachWindow(window)`

- `QObject*`를 허용합니다. `QQuickWindow*`만 유효합니다.
- 데몬이 시작되었는지 확인합니다.
- 이전에 연결된 창 바인딩을 대체합니다.
- `window-attached` 이벤트를 기록합니다.
- `uiTrackingEnabled=true`인 경우 UI 트리를 재귀적으로 추적합니다.
- 포인터 UI 스냅샷을 새로 고칩니다.
- 창 파괴 시 `window-detached`를 기록하고 첨부 파일을 지웁니다.

<a id="42-runtime-state-and-counters"></a>

### 4.2 런타임 상태 및 카운터

#### `markActivity()`

- 활동 타임스탬프를 업데이트합니다.
- 현재 유휴 상태인 경우 유휴 상태를 종료합니다.

#### `resetCounters()`

- 키보드/마우스/UI 카운터 및 마지막 상태 필드를 재설정합니다.
- 빈도가 높은 최종 기록 장부를 지웁니다.
- 데몬 실행 상태를 유지합니다(데몬을 중지하지 않음).
- `counters-reset` 이벤트를 기록합니다.

<a id="43-snapshot-and-query-apis"></a>

### 4.3 스냅샷 및 쿼리 API

#### `snapshot(): map`

광범위한 런타임 요약:

- 실행 중 + 카운터 + 유휴 + pid + rss + 가동 시간 + 시퀀스 + 마지막 이벤트 + 입력 상태.

#### `daemonHealth(): map`

건강 중심 요약:

- 실행 중 + attachedWindow + 부팅 에포크 + 시퀀스 + 링 상태 + 유휴 + pid + 마지막 이벤트 + 입력.

#### `inputState(): map`

입력 중심 상태 페이로드:

- 포인터 전역 좌표, 마우스 버튼 플래그, 키 상태, 수정자 결합, pointerUi, 누르기/놓기 타이밍.
- 터치 입력은 데스크톱 마우스 입력과 동일한 기본 포인터 계약에 정규화됩니다: 활성 터치 접촉은 `Qt::LeftButton` 로 보고되며, 업데이트는 마우스 카운터/시그널에 기여하고, 릴리스/취소는 버튼 상태를 해제합니다.
- 원시 `touch-event` 레코드는 인식자를 위해 네이티브 `QTouchEvent` / `QEventPoint` 세부 정보를 보존합니다: `fingerCount`, `activeFingerCount`, 위상 계수, `primaryPointId`, `multiTouch`, 릴리스/취소 플래그, `nativeTimestamp`, 장치 메타데이터 및 포인트별 프레스/마지막/전역 위치, 압력, 회전, 타원, 속도, 타임스탬프, 프레스 타임스탬프 및 `timeHeld`.
- 고수준 제스처 의미론은 여기에서 의도적으로 추가되지 않았으며, 소비자는 `GestureEvents`를 눌러야 합니다/스크롤/홀드/드래그/스와이프/ 네이티브 -제스처 분류를 사용해야 합니다.

#### `recentEvents(): list` / `clearRecentEvents()`

- 링 버퍼 읽기 및 재설정 작업.

#### `hitTestUiAt(globalX, globalY): map`

- 성공하면 자세한 히트 정보가 반환됩니다.
- 최상위 필드는 가장 깊은 기본 리프뿐만 아니라 가장 가까운 논리적 구성 요소 대상을 설명합니다.
- 원시 최심부 세부 정보는 `hitObjectName`, `hitClassName`, `hitPath`, `hitComponentName`, `hitQmlId`, `hitLocalX`, `hitLocalY`, `hitDepth`에 보존됩니다.
- 계층 구조 및 레이어 메타데이터는 `hierarchy`, `depth`, `layerKind`, `componentName`, `qmlId`, `qmlBaseUrl` 및 창/루트 필드를 통해 포함됩니다.
- 적중 테스트가 실패하거나 사용할 수 없는 경우 `"unknown"` 메타데이터와 함께 대체 경로 맵을 반환합니다.

<a id="5-high-frequency-event-behavior"></a>

## 5. 고주파수 이벤트 동작

고주파 등급:

- `mouse-move`
- `hover-move`

2단계 제어:

1. **페이로드 구성 샘플링** `shouldSampleHighFrequencyPayload()`는 풍부한 페이로드/히트 테스트 데이터를 구축할지 여부를 결정합니다.
2. **레코드 방출 스로틀링** `shouldSkipHighFrequencyRecord()`는 이벤트를 방출/저장할지 여부를 결정합니다.

의미:

- 제한된 창에서는 비용이 많이 드는 페이로드 작업을 일찍 건너뜁니다.
- 이벤트 시퀀스와 링 버퍼 볼륨은 동일한 간격 정책에 따라 한계가 설정된입니다.

<a id="6-pointer-hit-test-modes-and-costs"></a>

## 6. 포인터 적중 테스트 모드 및 비용

<a id="mode-matrix"></a>

### 모드 매트릭스

|구성|행동|비용|
|---|---|---|
|`pointerHitTestingEnabled=true`, 간격 `0`|각 새로 고침 요청 시 재계산|최고|
|`pointerHitTestingEnabled=true`, 간격 `>0`|간격당 최대 한 번 재계산|중간|
| `pointerHitTestingEnabled=false` |항상 대체 경로 포인터 맵|가장 낮은|

<a id="fallback-contract"></a>

### 대체 경로 계약

적중 테스트가 비활성화/사용 불가능한 경우 대체 경로에는 다음이 포함됩니다.

- `globalX`, `globalY`
- `insideWindow=false`
- `objectName="unknown"`, `className="unknown"`, `path="unknown"`
- `componentName="unknown"`, `layerKind="outsideWindow"| "unboundWindow"`
- 빈 `hierarchy`, `qmlId`, `qmlBaseUrl`

이는 필드가 존재해야 하는 소비자에 대한 스키마 안정성을 보장합니다.

<a id="7-ui-tracking-behavior"></a>

## 7. UI 추적 동작

`uiTrackingEnabled=true`일 때:

- 자식 추가는 첨부된 창 트리 아래에 재귀적으로 추적됩니다.
- 전환 표시/숨기기/생성/파괴는 카운터를 업데이트하고 `uiEvent`를 내보냅니다.
- `ui-event` 항목은 런타임 이벤트 스트림에 기록됩니다.

`uiTrackingEnabled=false`일 때:

- 추적된 트리가 분리되었습니다.
- UI 수명 주기 카운터가 새로운 수명 주기 전환으로 인해 증가하지 않습니다.
- 입력/포인터 상태 API는 계속 작동합니다.

<a id="8-integration-patterns"></a>

## 8. 통합 패턴

<a id="81-low-latency-production-profile"></a>

### 8.1 낮은 대기 시간 생산 프로필

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.LowLatencyCapture
    LV.RuntimeEvents.start()
    LV.RuntimeEvents.attachWindow(rootWindow)
}
```

<a id="82-balanced-interactive-diagnostics"></a>

### 8.2 균형 잡힌 대화형 진단

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.BalancedCapture
    LV.RuntimeEvents.pointerHitTestMinIntervalMs = 24
    LV.RuntimeEvents.start()
    LV.RuntimeEvents.attachWindow(rootWindow)
}
```

<a id="83-full-diagnostic-capture-short-sessions"></a>

### 8.3 전체 진단 캡처(짧은 세션)

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.FullCapture
    LV.RuntimeEvents.recentEventCapacity = 1024
}
```

<a id="9-failuretroubleshooting-matrix"></a>

## 9. 오류/문제 해결 매트릭스

|증상|예상 원인|검증|조치|
|---|---|---|---|
|`running=false` 예기치 않게|데몬이 시작되거나 중지되지 않았습니다.|시작 순서 검사|소비자가 구독하기 전에 `start()`를 호출합니다.|
|`pointerUi`에는 의미 있는 적중 정보가 부족합니다.|연결된 창이 없거나 적중 테스트가 비활성화되었습니다.|`daemonHealth().attachedWindow`를 확인하고, `pointerHitTestingEnabled`|창 연결, 필요한 경우 적중 테스트를 활성화합니다.|
|포인터 이동 중 CPU 전체|전체 캡처 + 이벤트별 히트 테스트|`captureProfile` 검사|`Balanced` 또는 `LowLatency` 로 이동|
|UI 카운터는 0으로 유지됩니다.|ui 추적 비활성화됨|확인 `uiTrackingEnabled`|수명 주기 원격 측정이 필요한 경우 추적 활성화|
|이벤트 목록이 너무 빨리 증가함|용량이 너무 큼 + 전체 캡처|`recentEventCapacity` 및 프로필|를 확인하여 용량을 줄이거나 프로필 수준|
|유휴 상태가 입력되지 않음|자주 `markActivity` 또는 낮은 시간 초과 불일치|`idleTimeoutMs` 검사, 외부 활동 호출|시간 초과 및 활동 호출 사이트 조정|

<a id="10-data-contract-notes-for-consumers"></a>

## 10. 소비자를 위한 데이터 계약 참고사항

- `eventRecorded(eventData)` 항목에는 다음이 포함됩니다.
  - `sequence`,
  - `type`,
  - `timestampEpochMs`,
  - `uptimeMs`,
  - 옵션 `payload`.
- 소비자는 모든 이벤트 유형에 대해 페이로드가 존재한다고 가정해서는 안 됩니다.
- 소비자는 알 수 없는/새로운 이벤트 유형을 향후 호환 가능한 입력으로 처리해야 합니다.

<a id="11-codex-oriented-playbook"></a>

## 11. Codex 중심 플레이북

이 섹션은 런타임/이벤트 파이프라인을 수정하는 Codex 작업 흐름을 위한 것입니다.

<a id="111-profile-first-patch-strategy"></a>

### 11.1 프로필 우선 패치 전략

지연 시간에 민감한 데모 또는 기능을 최적화하는 경우:

1. 먼저 `captureProfile`로 전환하고,
2. `pointerHitTestMinIntervalMs` 두 번째 조정,
3. `recentEventCapacity`를 마지막으로 조정하세요.

이는 의미적 드리프트를 최소화하여 캡처 비용을 줄이면서 기능을 유지합니다.

<a id="112-patch-safety-rules-for-codex"></a>

### 11.2 Codex용 패치 안전 규칙

- `eventRecorded` 스키마를 이전 버전과 호환되도록 유지합니다(`sequence`, `type`, 타임스탬프 키).
- 적중 테스트를 비활성화할 때 대체 경로 `pointerUi` 모양을 유지합니다.
- 기본적으로 데이터 손실 버그가 아닌 성능 정책으로 빈도 조절을 처리합니다.
- 수명 주기 멱등성을 유지합니다(`start()`, `stop()`, 반복된 `attachWindow()` 호출).

<a id="113-regression-checklist-for-codex"></a>

### 11.3 회귀 Codex 체크리스트

런타임 이벤트 편집 후 다음을 확인합니다.

1. 적어도 하나의 키보드/마우스 경로에 대해 카운터가 계속 증가합니다.
2. `daemonHealth().running` 및 `attachedWindow`는 진실을 유지합니다.
3. 대기 시간이 짧은 프로필은 예상대로 포인터/UI 추적을 비활성화합니다.
4. 링 버퍼는 용량에 따라 한계가 설정된로 유지됩니다.

<a id="12-validation-checklist"></a>

## 12. 검증 체크리스트

- `start()` + `attachWindow()`가 대상 창을 호출했습니다.
- `captureProfile`는 의도적으로 배포 모드로 선택되었습니다.
- `recentEventCapacity`는 예상 버스트 범위에 맞게 크기가 조정되었습니다.
- 포인터/UI 추적 토글은 기능 요구 사항과 일치합니다.
- 필요한 비즈니스 신호를 잃지 않고 고주파 소음이 조절됩니다.

<a id="13-related-apis"></a>

## 13. 관련 API

- `Backend`: 런타임 이벤트를 백엔드 소유 캐시에 미러링합니다.
- `GestureEvents`: `eventRecorded`에서 높은 수준의 제스처 의미 체계를 파생합니다.
- `EventListener` (QML): 상호 작용 처리를 위해 런타임/전역 이벤트를 사용합니다.
- `ApplicationWindow`: 시작 시 런타임 모니터링을 연결하는 일반적인 호스트입니다.

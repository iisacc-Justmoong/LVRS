<a id="debug-output-schema"></a>

# 디버그 출력 스키마

위치: `src/backend/runtime/debuglogger.h`, `src/backend/runtime/debuglogger.cpp`, `src/backend/runtime/runtimeevents.h`, `src/backend/runtime/runtimeevents.cpp`, `src/backend/runtime/gestureevents.h`, `src/backend/runtime/gestureevents.cpp`, `src/backend/io/backend.h`, `src/backend/io/backend.cpp`, `example/VisualCatalog/qml/Main.qml`, `src/qml/components/control/util/EventListener.qml`

이 문서는 LVRS 데모 앱에서 내보내지고 표시되는 디버그 및 런타임 이벤트 데이터에서 사용되는 스키마를 정의합니다. 이는 개발자가 stdout 로그, 런타임 이벤트 및 QML 모니터 보기 간의 관계를 모호함 없이 추적할 수 있도록 하기 위한 것입니다.

<a id="1-output-channels"></a>

## 1. 출력 채널

- 부트스트랩 stdout 진단: `LV.Debug`를 반드시 사용할 수 있기 전에 `preApplicationBootstrap`, `postApplicationBootstrap` 및 `runBootstrappedQmlApp`에서 내보냅니다.
- Stdout 텍스트 로그: `LV.Debug`에서 `printEntryToStdout()`를 통해 내보냅니다.
- Stdout JSON 로그: `LV.Debug.jsonOutput == true`일 때 추가 라인이 `[DEBUG-ENTRY] { ... }`로 생성됩니다.
- 디버그 메모리 버퍼: `LV.Debug.entries()/filteredEntries()/summary()`를 통해 사용 가능.
- RuntimeEvents 버퍼: `LV.RuntimeEvents.recentEvents()`를 통해 사용 가능합니다.
- GestureEvents 라이브 표면: `LV.GestureEvents.gestureSequence`, `LV.GestureEvents.lastGesture` 및 제스처 신호를 통해 사용할 수 있습니다.
- 백엔드 후크 버퍼: `LV.Backend.hookedUserEvents()`를 통해 사용 가능합니다.
- 데모 이벤트 리스너 모니터: `example/VisualCatalog/qml/Main.qml`의 `runtimeConsoleRows` 및 `eventMonitorSamplesModel`에 의해 표시됩니다.

<a id="2-current-default-output-policy-mainqml-bootstrap"></a>

## 2. 현재 기본 출력 정책(Main.qml 부트스트랩)

`Component.onCompleted` 및 `debuggerBootstrap()`에 의해 구성된 기본값은 다음과 같습니다.

- `LV.Debug.enabled = true`(stdout 출력 활성화됨)
- `LV.Debug.verboseOutput = false`
- `LV.Debug.jsonOutput = false`
- `LV.Debug.runtimeEchoEnabled = false`
- `LV.Debug.runtimeEchoMinIntervalMs >= 250`
- `LV.Debug.stdoutMinimumLevel = "WARN"`
- `LV.Debug.stdoutNoiseReductionEnabled = true`
- `LV.Debug.runtimeCaptureEnabled = true`

효과적인 동작은 고주파수 입력/수명주기 소음 억제 기능을 갖춘 경고 중심(`WARN/ERROR`) 출력입니다.

부트스트랩 진단은 `LV.Debug`와 별개입니다. 이는 항상 `LVRS bootstrap.` 접두사가 붙은 일반 표준 출력 라인이며 부트스트랩 옵션, 렌더링 기본값, 백엔드 검색, 가져오기 경로 확인 및 글꼴 정책 결정을 설명하는 컴팩트 JSON 페이로드를 전달합니다.

<a id="bootstrap-stdout-line-format"></a>

### 부트스트랩 stdout 라인 형식

- 접두사: `LVRS bootstrap.<stage>`
- 페이로드: 압축된 JSON 맵
- 레벨: 정상적인 진행의 경우 `INFO`, 대체 경로/소프트 저하의 경우 `WARN`, 치명적인 부트스트랩 오류의 경우 `CRITICAL`

일반적인 부트스트랩 단계:

- `pre.options`
- `pre.render-quality`
- `pre.quick-style`
- `graphics.probe`
- `graphics.selected`
- `graphics.fallback`
- `pre.complete`
- `post.application`
- `post.font-policy`
- `entry.import-paths`
- `entry.load-request`

<a id="3-lvdebug-entry-schema"></a>

## 3. LV.Debug 항목 스키마

`LV.Debug.log/warn/error` 및 `RuntimeEvents.eventRecorded`는 동일한 엔트리 버퍼(`m_entries`)에 저장됩니다.

<a id="common-fields"></a>

### 공통 필드

- `source`: `"logger"` 또는 `"runtime"`
- `level`: `"LOG" | "WARN" | "ERROR" | "RUNTIME"`
- `component`: e.g. `"Main"`, `"RenderMonitor"`, `"RuntimeEvents"`
- `event`: 이벤트 이름
- `message`: 기본 UI/stdout 라인 메시지
- `timestampEpochMs`: 에포크 ms
- `timestamp`: `HH:MM:SS.CS`(센티초)
- `timestampIso`: UTC ISO 문자열
- `sequence`: DebugLogger-로컬 시퀀스(1에서 증분)
- `sessionElapsedMs`: 디버그 세션 시작 이후 경과된 ms
- `processId`
- `threadId`
- `applicationName`
- `applicationVersion`
- `data`(옵션)
- `runtimeEventSequence`(RuntimeEvents 장착 시)

<a id="additional-fields-for-runtime-source"></a>

### `runtime` 소스에 대한 추가 필드

- `uptimeMs`
- `rawEvent`(원시 `RuntimeEvents` 이벤트)

<a id="4-stdout-output-rules"></a>

## 4. Stdout 출력 규칙

<a id="level-priority"></a>

### 레벨 우선순위

- `ERROR=3`, `WARN=2`, `RUNTIME=1`, `LOG=1`, `NONE=99`
- `shouldOutputLevel(entry.level) >= stdoutMinimumLevel`인 경우에만 출력이 방출됩니다.

<a id="noise-filter-stdoutnoisereductionenabledtrue"></a>

### 노이즈 필터(`stdoutNoiseReductionEnabled=true`)

- `LOG` 수준에서 `created`, `shown`, `hidden`, `destroyed`를 차단합니다.
- `LOG` 수준에서 `RenderMonitor.render-stats`를 차단합니다.
- 고주파수 `RUNTIME` 이벤트를 차단합니다.
  - `ui-event`, `mouse-move`, `hover-move`, `mouse-wheel`, `mouse-press`, `mouse-release`, `mouse-double-click`
  - `key-press`, `key-release`, `touch-event`, `tablet-event`, `tablet-proximity`, `native-gesture`

<a id="line-format"></a>

### 라인 형식

- `verboseOutput=false`: `entry.message`만 인쇄합니다.
- `verboseOutput=true`: `[timestamp] [level] #sequence component.event pid=... tid=... runtimeSeq=... data=...`
- `jsonOutput=true`: 위 줄 뒤에 `[DEBUG-ENTRY] {compact-json}`를 추가합니다.

<a id="5-runtimeevents-raw-event-eventrecorded-schema"></a>

## 5. RuntimeEvents 원시 이벤트(`eventRecorded`) 스키마

`recordRuntimeEvent()`에서 내보내는 공통 필드:

- `sequence`
- `type`
- `timestampEpochMs`
- `uptimeMs`
- `payload`(옵션)

주요 `type` 값 및 주요 `payload` 필드:

- `key-press`, `key-release`: `key`, `keyName`, `modifiers`, `autoRepeat`, `text`, `pressedKeys`, `pressedKeyCodes`, `activeModifierNames`
- `mouse-move`, `hover-move`: `x`, `y`, `buttons`, `pressedMouseButtons`, `modifiers`, `mouseButtonPressed`, `pointerUi`, `pointerObjectName`, `pointerClassName`, `pointerPath`
- `mouse-press`, `mouse-release`, `mouse-double-click`: 위의 키 + `button`, `lastMousePressEpochMs`/`lastMouseReleaseEpochMs`, 경과된 필드
- `mouse-wheel`: 위의 키 + `angleDeltaX/Y`, `pixelDeltaX/Y`, `phase`, `inverted`
- `touch-event` : `phase` , `pointCount` , `fingerCount` , `activeFingerCount` , 단계 수, `primaryPointId` , `multiTouch` , `released` , `cancelled` , `nativeTimestamp` , 터치 장치 메타데이터, `x` , `y` , `buttons` , `pressedMouseButtons` , `mouseButtonPressed` , `lastMousePressEpochMs` , `lastMouseReleaseEpochMs` , `releaseEpochMs` , `pressDurationMs` , 경과 시간 필드, 네이티브 `points[]` , `pointerUi` , `pointerObjectName` , `pointerClassName` , `pointerPath`
- `tablet-event`: `phase`, `pressure`, `rotation`, `xTilt`, `yTilt`, `pointerType` 등
- `tablet-proximity`: `phase`
- `native-gesture`: `gestureType`, `fingerCount`, `value`, `deltaX/Y`, 포인터/장치 정보
- `context-requested`: `x`, `y`, `modifiers`, `buttons`, `reason`, `pointerUi`
- `ui-event`: `eventType`, `objectName`, `className`, `visible`
- `daemon-started`, `daemon-stopped`, `window-attached`, `window-detached`, `counters-reset`

<a id="6-runtime-state-snapshot-api-output"></a>

## 6. 런타임 상태 스냅샷 API 출력

### `LV.RuntimeEvents.snapshot()`

- `running`
- 키/마우스/UI 카운터
- `idle`, `idleForMs`
- `pid`, `rssBytes`, `uptimeMs`
- `daemonBootEpochMs`, `eventSequence`, `recentEventCount`
- `lastEvent`
- `input`(`inputState()`의 결과)

### `LV.RuntimeEvents.daemonHealth()`

- `running`, `attachedWindow`
- `bootEpochMs`, `eventSequence`
- `recentEventCount`, `recentEventCapacity`
- `idle`, `idleForMs`
- `pid`
- `lastEvent`
- `input`

### `LV.RuntimeEvents.inputState()`

- 포인터 좌표/버튼/버튼 이름/눌린 상태
- 마지막 보도/공개 타임스탬프 및 경과 기간
- `activePressDurationMs`
- `pointerUi`(적중 테스트 결과)
- `anyKeyPressed`, `pressedKeys`, `pressedKeyCodes`
- `activeModifiers`, `activeModifierNames`
- `lastKey`, `lastKeyText`, `lastKeyModifiers`

<a id="7-backend-hook-output"></a>

## 7. 백엔드 후크 출력

`LV.Backend.hookUserEvents()`는 RuntimeEvents를 구독하고 별도의 버퍼를 유지합니다.

### `LV.Backend.hookedUserEvents(limit)`

- RuntimeEvents 원시 이벤트 목록과 `hookEpochMs`

### `LV.Backend.hookedUserEventSummary()`

- `hooked`
- `eventCount`
- `capacity`
- `lastEvent`
- `input`(`currentUserInputState()`의 결과)
- `typeCounts`(이벤트 유형별 집계)
- `runtimeEventSequence`(런타임 장착 시)

<a id="8-mainqml-event-listener-monitor-output"></a>

## 8. Main.qml 이벤트 리스너 모니터 출력

<a id="runtime-console-row-runtimeconsolerows-schema"></a>

### 런타임 콘솔 행(`runtimeConsoleRows[]`) 스키마

- `category`: `runtime|input|ui|render|navigation|system`
- `source`
- `type`
- `sequence`
- `timestampEpochMs`
- `uptimeMs`
- `payload`
- `summary`(유형별 요약 문자열)
- `detail`(축약된 페이로드 문자열)

디스플레이 타임스탬프는 `HH:MM:SS.mmm` 형식의 `runtimeConsoleTimestamp()`를 따릅니다.

카테고리 매핑은 `runtimeConsoleCategoryForType()`를 따릅니다.

- 입력 제품군: `key-*`, `mouse-*`, `touch-*`, `tablet-*`, `native-gesture*`, `hover-*`, `context-*`, `global-*` -> `input`
- `ui-event` -> `ui`
- `render-*` -> `render`
- `route-*`, `viewstack-*` -> `navigation`
- `daemon-*`, `window-*`, `catalog-*`, `counters-*` -> `runtime`
- 기타 -> `system`

<a id="eventlistener-sample-eventmonitorsamplesmodel-schema"></a>

### EventListener 샘플(`eventMonitorSamplesModel`) 스키마

- `trigger`
- `source`
- `timestampEpochMs`
- `payload`

샘플이 `eventMonitorMaxSamples`를 초과하면 가장 오래된 항목이 먼저 제거됩니다(FIFO).

<a id="9-eventlistener-callback-payload-schema"></a>

## 9. EventListener 콜백 페이로드 스키마

`src/qml/components/control/util/EventListener.qml` 기반:

- 로컬 포인터 트리거(`clicked|pressed|released`):
  - `x`, `y`, `globalX`, `globalY`, `button`, `buttons`, `modifiers`, `isGlobal=false`
  - `ui`(옵션, `includeUiHit=true`)
  - `input`(옵션, `includeInputState=true`)
  - `src/backend`(옵션, `includeBackendSummary=true`)
- 글로벌 트리거(`globalPressed|globalContextRequested`):
  - `x`, `y`, `globalX`, `globalY`, `buttons`, `modifiers`, `isGlobal=true`
  - `ui`(옵션, `includeUiHit=true`)
  - `input`(옵션, `includeInputState=true`)
  - `src/backend`(옵션)
  - 컨텍스트 이벤트의 경우 `reason` 및 `source(mouse|context)`가 추가됩니다.
- 제스처 트리거(`touchStarted|touchUpdated|touchEnded|touchCancelled|pressStarted|pressEnded|holdStarted|longPressed|dragStarted|dragUpdated|dragEnded|scrollStarted|scrollUpdated|scrollEnded|swipeDetected|nativeGestureDetected|gestureRecognized`):
  - 공통: `sequence`, `gestureType`, `interactionKind`, `classification`, `source`, `timestampEpochMs`, `x`, `y`, `globalX`, `globalY`
  - 터치에서 파생된: `sessionId`, `previous*`, `start*`, `delta*`, `totalDelta*`, `distance`, `durationMs`, `pressDurationMs`, `directionX`, `directionY`, `dominantAxis`, `holdActive`, `dragActive`, `scrollActive`, `phase`, `pointCount`, `fingerCount`, `activeFingerCount`, `maximumFingerCount`, `points`, 삭제되지 않은 필드, 포인터/버튼 상태, `ui`, `originUi`
  - `ui` / `originUi`는 논리적 대상 메타데이터(`objectName`, `className`, `componentName`, `qmlId`, `qmlBaseUrl`, `path`, `layerKind`, `hierarchy`) 및 원시 리프 메타데이터(`hitObjectName`, `hitClassName`, `hitPath`, `hitComponentName`, `hitQmlId`)
  - 스와이프 전용: `swipeDirection`, `velocityX`, `velocityY`, `speed`
  - 스크롤 특정: `scrollAxis`, `scrollDirection`, `scrollDeltaX/Y`
  - 네이티브 제스처별: `nativeGestureType`, `fingerCount`, `value`, `deltaX/Y`
  - 선택적 `input` / `src/backend` 강화는 다른 트리거와 동일한 옵트인 스위치를 따릅니다.
- 키/휠 트리거는 Qt 이벤트 객체를 직접 전달합니다.

중복 제거:

- 글로벌 프레스 중복 제거: `globalPressDedupMs`(기본값 24ms), 좌표 공차 `globalPressDedupTolerancePx`(기본값 2px)
- 컨텍스트 중복 제거: `contextDedupMs`(기본값 180ms), 좌표 공차 `contextDedupTolerancePx`(기본값 2px)

<a id="10-render-performance-alert-output-conditions"></a>

## 10. 렌더링 성능 경고 출력 조건

`evaluateRenderPerformance()`는 주기적으로 실행되며 `LV.Debug`를 통해 방출됩니다.

- 심각: `lastFrameMs >= 50` 또는 `fps < 18` -> `ERROR render-performance-severe` (최소 간격: 1600ms)
- 저하됨: `lastFrameMs >= 33` 또는 `fps < 30` -> `WARN render-performance-degraded`(최소 간격: 2000ms)
- 복구 : 3 초 동안 정상 상태가 유지된 후 `WARN render-performance-recovered`

일반적인 페이로드:

- `fps`
- `lastFrameMs`
- `frameCount`
- `src/backend` (`LV.RenderQuality.graphicsBackend`)

<a id="consumer-implementation-notes"></a>

## 소비자 구현 참고 사항

로그 뷰어 또는 원격 측정 파이프라인과 통합하는 경우:

- `sequence`를 정렬된 이벤트 커서로 처리합니다.
- `timestamp` 필드를 벽시계로 처리하고 외부적으로 대기 시간을 계산합니다.
- 향후 호환성을 위해 선택적 페이로드 필드에 대한 엄격한 스키마 구문 분석을 피하세요.

<a id="parser-safety-recommendations"></a>

## 파서 안전 권장 사항

- 항상 null 검사 중첩 맵(`payload`, `input`, `ui`),
- 이전 항목에 대한 누락된 키를 허용합니다.
- 향후 확장성을 보존하려면 알 수 없는 필드 통과를 유지하세요.

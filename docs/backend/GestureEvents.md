# GestureEvents

위치: `src/backend/runtime/gestureevents.h` / `src/backend/runtime/gestureevents.cpp`

`GestureEvents`는 LVRS 고급 제스처 인식 싱글턴입니다.   `RuntimeEvents` 위에 위치하며 원시 터치/네이티브 제스처 런타임 기록을 QML 친화적 제스처 이벤트로 전환합니다.

<a id="1-scope-and-responsibilities"></a>

## 1. 범위와 책임

`GestureEvents` 소유:

- `RuntimeEvents::eventRecorded` 위에 터치 세션 추적,
- `touchStarted`, `touchUpdated`, `touchEnded`, `touchCancelled`에 대한 상위 수준 제스처 분류,
- `pressStarted`, `pressEnded`, `scrollStarted`, `scrollUpdated`, `scrollEnded`, `holdStarted`, `dragStarted`, `dragUpdated`, `dragEnded`, `swipeDetected`에 대한 파생된 의미 이벤트
- `nativeGestureDetected`를 통한 네이티브 제스처 전달 정규화,
- `gestureSequence` 및 `lastGesture`를 통한 안정적인 최종 이벤트 검사.

`GestureEvents`는 ****를 소유하지 않습니다.

- 원시 애플리케이션 이벤트 필터링(`RuntimeEvents`가 이를 소유함),
- 최근 이벤트 버퍼링/데몬 상태 원격 측정(`RuntimeEvents`가 이것을 소유함),
- 백엔드 미러링/지속성(`Backend`가 이것을 소유함),
- 기본 접촉 경로를 넘어서는 다중 접촉 제스처 분류.

<a id="2-property-api"></a>

## 2. 속성 API

|속성|기본값|의미|
|---|---:|---|
| `runtimeAttached` | `false` |`RuntimeEvents` 소스가 바인딩된 경우 True입니다.|
| `holdThresholdMs` | `450` |`holdStarted`에 필요한 최소 정지 프레스 지속 시간.|
| `dragThresholdPx` | `12.0` |드래그 분류가 시작되기 전에 이동 거리가 필요합니다.|
| `scrollThresholdPx` | `12.0` |스크롤 분류가 시작되기 전에 주축 한 손가락 이동 거리가 필요합니다.|
| `swipeThresholdPx` | `48.0` |스와이프 감지에 필요한 최소 총 이동 거리.|
| `swipeMaxDurationMs` | `700` |스와이프는 이 기간 내에 완료되어야 합니다.|
| `axisDominanceRatio` | `1.35` |우세 방향을 분류하는 데 사용되는 축 비율(`x`, `y`, `diagonal`).|
| `gestureSequence` | `0` |인식된 제스처 페이로드에 대한 단조로운 시퀀스입니다.|
| `lastGesture` | `{}` |마지막으로 게시된 상위 수준 제스처 페이로드입니다.|

<a id="3-method-contract"></a>

## 3. 메소드 계약

### `attachRuntime(runtimeObject = null): bool`

- 명시적인 `RuntimeEvents` 개체를 허용하거나 싱글턴를 자동으로 확인합니다.
- `RuntimeEvents::eventRecorded`에 연결합니다.
- 바인딩이 성공하면 `true`를 반환합니다.

### `detachRuntime()`

- 바인딩된 런타임 소스에서 연결을 끊습니다.
- 활성 터치 세션 상태를 지웁니다.

### `resetState()`

- 활성 터치/끌기/홀드 세션 상태를 지웁니다.
- 진행 중인 타이머를 재설정합니다.
- 런타임 바인딩을 유지합니다.

<a id="4-signals"></a>

## 4. 신호

- `gestureRecognized(eventData)`
- `touchStarted(eventData)`
- `touchUpdated(eventData)`
- `touchEnded(eventData)`
- `touchCancelled(eventData)`
- `pressStarted(eventData)`
- `pressEnded(eventData)`
- `holdStarted(eventData)`
- `dragStarted(eventData)`
- `dragUpdated(eventData)`
- `dragEnded(eventData)`
- `scrollStarted(eventData)`
- `scrollUpdated(eventData)`
- `scrollEnded(eventData)`
- `swipeDetected(eventData)`
- `nativeGestureDetected(eventData)`

`gestureRecognized`는 위의 보다 구체적인 신호를 포함하여 게시된 모든 상위 수준 페이로드에 대해 방출됩니다.

<a id="5-payload-schema"></a>

## 5. 페이로드 스키마

<a id="51-common-gesture-fields"></a>

### 5.1 일반적인 제스처 필드

게시된 모든 페이로드에는 다음이 포함됩니다.

- `sequence`
- `gestureType`
- `interactionKind`
- `classification`
- `source`
- `timestampEpochMs`
- `x`, `y`, `globalX`, `globalY`
- `ui`

`ui` 및 `originUi`는 강화된 `RuntimeEvents.hitTestUiAt()` 스키마를 사용합니다.

- 논리적 대상 ID: `objectName`, `className`, `componentName`, `qmlId`, `qmlBaseUrl`, `path`
- 레이어 및 계층: `layerKind`, `depth`, `hierarchy`, 루트/창 필드
- 원시 가장 깊은 잎: `hitObjectName`, `hitClassName`, `hitPath`, `hitComponentName`, `hitQmlId`, `hitLocalX`, `hitLocalY`, `hitDepth`

<a id="52-touch-derived-gesture-fields"></a>

### 5.2 터치 기반 제스처 필드

`touch*`, `press*`, `scroll*`, `holdStarted`, `drag*` 및 `swipeDetected`에는 다음이 추가로 포함됩니다.

- `sessionId`
- `previousX`, `previousY`
- `startX`, `startY`, `startGlobalX`, `startGlobalY`
- `deltaX`, `deltaY`
- `totalDeltaX`, `totalDeltaY`
- `absoluteDeltaX`, `absoluteDeltaY`
- `distance`
- `durationMs`
- `pressDurationMs`
- `directionX` (`positive|negative|none`)
- `directionY` (`positive|negative|none`)
- `dominantAxis` (`x|y|diagonal|none`)
- `holdActive`, `dragActive`, `scrollActive`
- `holdThresholdMs`, `dragThresholdPx`, `scrollThresholdPx`, `swipeThresholdPx`, `swipeMaxDurationMs`
- `phase`
- `pointCount`, `fingerCount`, `activeFingerCount`, `maximumFingerCount`
- `pressedFingerCount`, `updatedFingerCount`, `stationaryFingerCount`, `releasedFingerCount`
- `primaryPointId`, `multiTouch`, `released`, `cancelled`, `releaseEpochMs`
- `points`
- `buttons`
- `pressedMouseButtons`
- `modifiers`
- `mouseButtonPressed`
- `originUi`

`points`의 각 항목은 `RuntimeEvents`의 네이티브 `QEventPoint` 세부 정보를 그대로 반영한다. 여기에는 상태 이름, 타임스탬프/pressTimestamp/timeHeld, 압력/회전/타원, 속도 및 위치 계열(`position*`, `pressPosition*`, `lastPosition*`, `scene*`, `global*`)이 포함된다.

<a id="53-scroll-specific-fields"></a>

### 5.3 스크롤 특정 필드

`scrollStarted`, `scrollUpdated` 및 `scrollEnded` 추가:

- `scrollAxis` (`x|y`)
- `scrollDirection`
- `scrollDeltaX`
- `scrollDeltaY`

<a id="54-swipe-specific-fields"></a>

### 5.4 스와이프 관련 필드

`swipeDetected`는 다음을 추가합니다.

- `swipeDirection`
- `velocityX`
- `velocityY`
- `speed`

`swipeDirection`는 총 델타에서 파생되며 다음과 같을 수 있습니다.

- `leftToRight`
- `rightToLeft`
- `topToBottom`
- `bottomToTop`
- 4 대각선 토큰 중 하나

<a id="55-native-gesture-fields"></a>

### 5.5 네이티브 제스처 필드

`nativeGestureDetected`는 다음을 추가합니다.

- `nativeGestureType`
- `fingerCount`
- `value`
- `deltaX`
- `deltaY`
- `buttons`
- `pressedMouseButtons`
- `modifiers`

<a id="6-recognition-rules"></a>

## 6. 인식 규칙

<a id="press"></a>

### 언론

- `TouchBegin`부터 시작됩니다.
- 네이티브 손가락 수 및 포인트 메타데이터를 사용하여 `pressStarted`를 즉시 내보냅니다.
- `pressDurationMs`, `releaseEpochMs`, `released`, `cancelled` 및 `finalInteractionKind`를 사용하여 릴리스/취소 시 `pressEnded`를 방출합니다.

<a id="scroll"></a>

### 스크롤

- 하나의 활성 터치 접촉이 주요 `x` 또는 `y` 축에서 `scrollThresholdPx`를 넘어 이동한 후에 시작됩니다.
- `scrollStarted`를 한 번 방출하고, 이후 인식된 움직임 시 `scrollUpdated`를 방출하고, 해제/취소 시 `scrollEnded`를 방출합니다.
- 스크롤 분류는 연속 동작 지향적입니다. 빠른 릴리스도 여전히 `swipeDetected` 자격을 얻을 수 있습니다.

<a id="hold"></a>

### 보류

- `TouchBegin`부터 시작됩니다.
- `holdThresholdMs`가 경과할 때까지 포인터가 `dragThresholdPx` 내에 머무는 경우에만 실행됩니다.

<a id="drag"></a>

### 드래그

- 터치 원점으로부터의 총 거리가 `dragThresholdPx`에 도달하면 시작됩니다.
- `dragStarted`를 한 번 내보낸 다음 이후 업데이트에서 `dragUpdated`를 내보낸 다음 릴리스/취소 시 `dragEnded`를 내보냅니다.

<a id="swipe"></a>

### 스와이프

- 터치 엔드에서 평가됩니다.
- `distance >= swipeThresholdPx`가 필요합니다.
- `durationMs <= swipeMaxDurationMs`가 필요합니다.
- 방향은 `axisDominanceRatio`에서 파생됩니다.

<a id="native-gesture"></a>

### 네이티브 제스처

- `RuntimeEvents`에서 이미 캡처한 원시 `native-gesture` 레코드를 사용합니다.
- Qt가 보고하는 플랫폼 제스처 종류인 `fingerCount`, 스칼라 `value` 및 `deltaX/Y`를 유지합니다.

<a id="7-integration-patterns"></a>

## 7. 통합 패턴

<a id="71-direct-singleton-usage"></a>

### 7.1 직접 싱글턴 사용법

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.start()
    LV.RuntimeEvents.attachWindow(rootWindow)
    LV.GestureEvents.attachRuntime(LV.RuntimeEvents)
}

Connections {
    target: LV.GestureEvents
    function onSwipeDetected(eventData) {
        console.log(eventData.swipeDirection, eventData.totalDeltaX, eventData.totalDeltaY)
    }
}
```

<a id="72-preferred-qml-consumption-through-eventlistener"></a>

### 7.2 `EventListener`를 통한 기본 QML 소비

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "swipeDetected"
    action: function(eventData) {
        console.log(eventData.swipeDirection)
    }
}
```

`EventListener`는 제스처 트리거가 사용될 때 `RuntimeEvents` 및 `GestureEvents`를 자동으로 연결합니다.

<a id="8-current-recognition-boundary"></a>

## 8. 현재 인식 경계

- 인식은 기본 접촉 중심입니다.
- 멀티터치 포인트 배열, 손가락 카운트 및 최대 세션 손가락 카운트가 전달됩니다. 방향 스크롤/드래그/스와이프 분류는 플랫폼이 네이티브 제스처를 발생시키지 않는 한 기본 접촉 경로를 계속 추적합니다.
- 원시 데스크탑 마우스 의미 체계는 `GestureEvents`가 아닌 `EventListener` 포인터/전역 트리거에 의해 처리됩니다.

<a id="9-troubleshooting-matrix"></a>

## 9. 문제 해결 매트릭스

|증상|예상 원인|검증|조치|
|---|---|---|---|
|제스처 콜백 없음|런타임 연결되지 않음|`runtimeAttached`|검사 `attachRuntime()` 호출 또는 `EventListener` 제스처 트리거 사용|
|홀드가 실행되지 않음|드래그 임계값이 너무 일찍 도달함|`distance` 대 `dragThresholdPx`|검사 `dragThresholdPx` 증가 또는 부수적 움직임 감소|
|스와이프가 감지되지 않음|지속 시간이 너무 길거나 거리가 너무 짧음|`durationMs` 검사, `distance`|조정 `swipeThresholdPx` / `swipeMaxDurationMs`|
|방향이 대각선처럼 보입니다|축 우세 비율이 너무 엄격합니다|`dominantAxis`|낮은 `axisDominanceRatio`|
|에는 원시 전체 제스처 기록이 필요합니다.|는 `lastGesture`만 사용합니다.|는 `gestureSequence` 변경 사항을 검사합니다.|는 `gestureRecognized`를 구독하고 자체 버퍼를 유지합니다.|

<a id="10-related-apis"></a>

## 10. 관련 API

- `RuntimeEvents`: 원시 이벤트 캡처 소스.
- `EventListener`: 제스처 콜백을 위한 QML 트리거 브리지입니다.
- `Backend`: 원시 런타임 이벤트용 백엔드 미러입니다.

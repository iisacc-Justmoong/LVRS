# EventListener

위치: `src/qml/components/control/util/EventListener.qml`

`EventListener`는 트리거 토큰을 콜백 이벤트에 매핑하는 LVRS 상호 작용 브리지 구성 요소입니다.   현재 정책은 사건 중심적입니다. 즉, 명시적으로 활성화하지 않는 한 지속적인 상태 수집을 방지합니다.

<a id="1-design-goal"></a>

## 1. 디자인 목표

- 실제 사건(`click`, `press`, `release`, `context request` 등)에 대한 이벤트 콜백을 전달합니다.
- 기본적으로 고비용 페이로드 강화를 피하세요.
- 불필요한 런타임 스냅샷/적중 테스트 풀로 인한 연쇄 반응을 방지합니다.

<a id="2-supported-triggers"></a>

## 2. 지원되는 트리거

- 포인터-로컬: `clicked`, `pressed`, `released`, `entered`, `exited`, `hoverChanged`
- 휠: `wheel`
- 키보드: `keyPressed`, `keyReleased`
- 글로벌 런타임: `globalPressed`, `globalContextRequested`
- 제스처 런타임: `touchStarted`, `touchUpdated`, `touchEnded`, `touchCancelled`, `pressStarted`, `pressEnded`, `holdStarted`, `longPressed`, `dragStarted`, `dragUpdated`, `dragEnded`, `scrollStarted`, `scrollUpdated`, `scrollEnded`, `swipeDetected`, `nativeGestureDetected`, `gestureRecognized`

<a id="3-core-api"></a>

## 3. 코어 API

|속성|기본값|의미|
|---|---|---|
| `trigger` | `"clicked"` |이벤트 소스 선택기.|
| `action` | `null` |트리거가 실행될 때 콜백이 호출됩니다.|
| `enabled` | `true` |리스너 활성화 플래그입니다.|
| `acceptedButtons` | `Qt.LeftButton` |로컬 포인터 경로에서 허용되는 마우스 버튼.|
| `macControlClickAsRight` | `true` |macOS Ctrl+왼쪽을 컨텍스트 제스처로 처리합니다.|
| `includeUiHit` | `false` |`ui` 적중 테스트 정보로 페이로드를 강화합니다(선택).|
| `includeInputState` | `false` |런타임/백엔드 `input` 스냅샷(선택)으로 페이로드를 강화합니다.|
| `preferBackendState` | `false` |`includeInputState=true`인 경우 백엔드 우선 입력 상태 확인을 선호합니다.|
| `includeBackendSummary` | `false` |페이로드에 백엔드 후크 요약 맵을 포함합니다(선택).|
| `globalPressDedupMs` | `24` |글로벌 언론 중복 제거 시간 창입니다.|
| `globalPressDedupTolerancePx` | `2.0` |전역 언론 중복 제거 공간 허용 범위입니다.|
| `contextDedupMs` | `180` |컨텍스트 중복 제거 시간 창입니다.|
| `contextDedupTolerancePx` | `2.0` |컨텍스트 중복 제거 공간 허용 오차입니다.|

<a id="4-incident-centric-payload-rules"></a>

## 4. 사고 중심 페이로드 규칙

<a id="41-default-mode-recommended"></a>

### 4.1 기본 모드(권장)

기본적으로:

- `input` 스냅샷을 가져오지 않았습니다.
- UI 적중 테스트 순회가 실행되지 않습니다.
- 콜백은 직접적인 사건 필드만 수신합니다.

이는 런타임 결합을 최소화하고 필요하지 않은 경우에 대한 보이지 않는 뷰 트리 추적을 방지합니다.

<a id="42-enriched-mode-explicit-opt-in"></a>

### 4.2 강화 모드(명시적 옵트인)

콜백 로직에 실제로 런타임 상태 일관성이 필요한 경우 `includeInputState: true`를 설정하세요.   결정에 UI 적중 메타데이터가 필요한 경우(예: 상황별 메뉴)에만 `includeUiHit: true`를 설정하십시오.

<a id="43-backend-summary-mode"></a>

### 4.3 백엔드 요약 모드

`includeBackendSummary: true`는 진단 지향적이며 고속 흐름에서는 비용이 많이 들 수 있습니다.   핫 경로 상호 작용 콜백이 아닌 도구/디버그 대시보드에만 사용하세요.

<a id="5-payload-schema"></a>

## 5. 페이로드 스키마

<a id="local-pointer-clickedpressedreleased"></a>

### 로컬 포인터(`clicked|pressed|released`)

항상:

- `x`, `y`, `globalX`, `globalY`
- `button`, `buttons`, `modifiers`
- `isGlobal=false`

선택사항:

- `includeUiHit=true`일 때 `ui`
- `includeInputState=true`일 때 `input`
- `includeBackendSummary=true`일 때 `src/backend`

<a id="global-pointer-globalpressedglobalcontextrequested"></a>

### 글로벌 포인터(`globalPressed|globalContextRequested`)

항상:

- `x`, `y`, `globalX`, `globalY`
- `buttons`, `modifiers`
- `isGlobal=true`

상황별 추가:

- `reason`
- `source` (`"mouse"` 또는 `"context"`)

선택적 강화는 로컬 포인터 페이로드와 동일한 토글을 따릅니다.

<a id="wheel--key-triggers"></a>

### 휠/키 트리거

- 휠/키는 Qt 이벤트 객체를 직접 전달하도록 트리거합니다.

### `hoverChanged`

- 페이로드: `{ containsMouse: bool }`

<a id="gesture-triggers"></a>

### 제스처 트리거

제스처 트리거는 `GestureEvents`에서 게시한 정규화된 페이로드를 수신합니다.

항상:

- `sequence`, `gestureType`, `interactionKind`, `source`
- `classification`
- `timestampEpochMs`
- `x`, `y`, `globalX`, `globalY`

터치 기반 제스처 트리거는 다음을 추가로 수신합니다.

- `sessionId`
- `previousX`, `previousY`
- `startX`, `startY`, `startGlobalX`, `startGlobalY`
- `deltaX`, `deltaY`
- `totalDeltaX`, `totalDeltaY`
- `absoluteDeltaX`, `absoluteDeltaY`
- `distance`, `durationMs`
- `pressDurationMs`
- `directionX`, `directionY`, `dominantAxis`
- `holdActive`, `dragActive`, `scrollActive`
- `phase`, `pointCount`, `fingerCount`, `activeFingerCount`, `maximumFingerCount`, `points`
- `pressedFingerCount`, `updatedFingerCount`, `stationaryFingerCount`, `releasedFingerCount`
- `primaryPointId`, `multiTouch`, `released`, `cancelled`, `releaseEpochMs`
- `buttons`, `pressedMouseButtons`, `modifiers`, `mouseButtonPressed`
- `ui`, `originUi`

`ui` / `originUi`에는 구성 요소 식별 메타데이터가 포함됩니다.

- 논리적 대상: `objectName`, `className`, `componentName`, `qmlId`, `qmlBaseUrl`, `path`
- 계층구조: `layerKind`, `depth`, `hierarchy`
- 원시 가장 깊은 항목: `hitObjectName`, `hitClassName`, `hitPath`, `hitComponentName`, `hitQmlId`

스와이프 관련 추가 사항:

- `swipeDirection`
- `velocityX`
- `velocityY`
- `speed`

스크롤 관련 추가 사항:

- `scrollAxis`
- `scrollDirection`
- `scrollDeltaX`
- `scrollDeltaY`

네이티브 제스처 추가:

- `nativeGestureType`
- `fingerCount`
- `value`
- `deltaX`, `deltaY`

<a id="6-dedup-behavior"></a>

## 6. 중복 제거 동작

- 글로벌 언론 중복 제거는 짧은 시간 내에 중복된 언론 사고를 억제합니다.
- 컨텍스트 중복 제거는 자체 창에서 중복된 컨텍스트 사건을 억제합니다.
- 중복 제거는 콜백 방출에만 영향을 미칩니다. 외부 런타임 데몬 카운터를 변경하지 않습니다.

<a id="7-usage-patterns"></a>

## 7. 사용 패턴

<a id="71-minimal-incident-listener-preferred"></a>

### 7.1 최소 사고 수신기(선호)

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalPressed"
    action: function(eventData) {
        // 외부 클릭으로 닫는 로직에는 좌표와 버튼만 사용한다.
    }
}
```

<a id="72-context-listener-with-ui-hit-metadata"></a>

### 7.2 UI 히트 메타데이터가 포함된 컨텍스트 리스너

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalContextRequested"
    includeUiHit: true
    action: function(eventData) {
        console.log(eventData.ui ? eventData.ui.path : "unknown")
    }
}
```

<a id="73-explicit-input-state-opt-in"></a>

### 7.3 명시적 입력 상태 옵트인

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalContextRequested"
    includeInputState: true
    preferBackendState: true
    action: function(eventData) {
        const input = eventData.input || ({})
        console.log(input.activeModifierNames)
    }
}
```

<a id="74-gesture-listener"></a>

### 7.4 제스처 리스너

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "swipeDetected"
    action: function(eventData) {
        console.log(eventData.swipeDirection, eventData.totalDeltaX, eventData.totalDeltaY)
    }
}
```

<a id="75-mobile-pressscroll-listeners"></a>

### 7.5 모바일 프레스/스크롤 리스너

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "pressEnded"
    action: function(eventData) {
        console.log(eventData.pressDurationMs, eventData.fingerCount, eventData.released)
    }
}

LV.EventListener {
    trigger: "scrollStarted"
    action: function(eventData) {
        console.log(eventData.scrollAxis, eventData.scrollDirection)
    }
}
```

<a id="8-common-pitfalls"></a>

## 8. 일반적인 함정

- 습관적으로 어디서나 `includeInputState`를 활성화합니다.
- 고주파 상호 작용 경로에 대해 `includeUiHit` 및 `includeBackendSummary`를 모두 활성화합니다.
- 보도 자료 중심이어야 하는 비즈니스 로직에 `hoverChanged`를 사용합니다.
- 전역 좌표 확인 없이 전역 트리거를 로컬 형상 전용 이벤트로 처리합니다.
- `longPressed`가 별도의 페이로드 유형이 될 것으로 예상됩니다. `holdStarted`의 별칭입니다.
- `EventListener`에서 전체 멀티터치 제스처 분류 체계를 기대합니다; 이는 손가락 카운트와 네이티브 포인트 배열을 전달하며, 방향성 스크롤/드래그/스와이프 분류는 Qt가 `nativeGestureDetected`를 방출하지 않는 한 기본 접촉을 따릅니다.

<a id="9-troubleshooting-matrix"></a>

## 9. 문제 해결 매트릭스

|증상|예상 원인|검증|조치|
|---|---|---|---|
|콜백이 실행되지 않음|잘못된 트리거 토큰 또는 비활성화된 리스너|`trigger` 검사, `enabled`|토큰/상태 수정|
|누락 `ui` 페이로드| `includeUiHit=false` |리스너 소품 검사|필요한 경우에만 활성화|
|누락 `input` 페이로드| `includeInputState=false` |리스너 소품 검사|해당 리스너에 대해 명시적으로 활성화|
|중복 컨텍스트 콜백|소스에 대한 중복 제거 창이 너무 느슨함|중복 제거 구성 검사|`contextDedup*` 값 조정|
|swipe/drag 발화 안됨|런타임 연결되지 않거나 임계값이 너무 엄격함|트리거 타입 및 이동 페이로드 검사|제스처 트리거 경로 사용 및 `GestureEvents` 를 통해 임계값 조정|
|헤비 콜백 체인|전역적으로 불필요한 강화 활성화|핫 경로의 소품 검사|사고 페이로드를 최소화|

<a id="10-codex-oriented-playbook"></a>

## 10. Codex 중심 플레이북

<a id="101-safe-codex-defaults-for-new-listeners"></a>

### 새로운 청취자를 위한 10.1 Safe Codex 기본값

1. `includeUiHit=false`를 유지하세요.
2. `includeInputState=false`를 유지하세요.
3. `includeBackendSummary=false`를 유지하세요.
4. 콜백 논리에 필요한 경우에만 강화를 활성화합니다.

<a id="102-codex-anti-patterns"></a>

### 10.2 Codex 안티 패턴

- 모든 리스너에 입력 상태 강화를 자동 주입하지 마세요.
- 프레임별 추적 의미 체계에 전역 수신기를 사용하지 마세요.
- 공유 루트 리스너에 부작용이 많은 콜백 본문을 추가하지 마세요.

<a id="103-codex-regression-checklist"></a>

### 10.3 코덱스 회귀 체크리스트

수정 후:

1. 글로벌 언론/컨텍스트 청취자는 여전히 사건당 정확히 한 번만 실행됩니다.
2. 외부 해제 흐름은 최소한의 페이로드로 계속 작동합니다.
3. 제스처는 런타임 자동 연결을 트리거하고 인식된 누르기/스크롤/제스처 페이로드를 제공합니다.
4. 강화된 리스너는 활성화된 경우 요청된 선택적 필드를 계속 수신합니다.

<a id="11-related-apis"></a>

## 11. 관련 API

- `RuntimeEvents`: 글로벌 트리거용 런타임 데몬 소스입니다.
- `GestureEvents`: 제스처 트리거에서 사용되는 상위 수준 제스처 소스입니다.
- `Backend`: 선택적 백엔드 우선 입력 상태 소스.
- `ApplicationWindow`: 앱 전체 동작을 위해 루트 수준 전역 수신기를 설치합니다.

<a id="shared-motion"></a>

## 공유 모션

리스너는 이벤트를 전달합니다. 아래의 실제 컨트롤은 눈에 보이는 동작을 제공합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

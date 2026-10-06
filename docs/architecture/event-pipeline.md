<a id="event-pipeline"></a>

# 이벤트 파이프라인

이 문서에서는 OS/Qt 이벤트부터 LVRS의 상위 수준 QML 동작까지의 엔드투엔드 이벤트 경로를 설명합니다.

<a id="pipeline-stages"></a>

## 파이프라인 단계

1. 캡처 단계: `RuntimeEvents`
- 이벤트 필터를 설치하고 키보드/포인터/컨텍스트/터치/태블릿/제스처/UI 수명 주기 이벤트를 기록합니다.
- 카운터, 최근 이벤트 링 버퍼 및 입력 스냅샷(`inputState()`)을 유지합니다.
- `eventRecorded(eventData)`를 표준 런타임 스트림으로 내보냅니다.

2. 인식 단계: `GestureEvents`
- `RuntimeEvents::eventRecorded`를 구독합니다.
- 원시 `touch-event` 및 `native-gesture` 항목을 해석합니다.
- 정규화된 상위 수준 제스처 페이로드(`touch*`, `press*`, `scroll*`, `holdStarted`, `drag*`, `swipeDetected`, `nativeGestureDetected`)를 방출합니다.

3. 후크 스테이지: `Backend`
- `hookUserEvents()`는 `RuntimeEvents::eventRecorded`를 구독합니다.
- 이벤트를 한계가 설정된 백엔드 캐시(`hookedUserEvents`)로 미러링합니다.
- 백엔드 우선 읽기에 대한 유형별 카운터와 마지막 입력 스냅샷을 유지합니다.

4. 소비단계: `EventListener`
- `RuntimeEvents` 및 `GestureEvents` 모두에서 트리거 이름을 구체적인 소스 구독으로 변환합니다.
- 사건 우선 페이로드(좌표/버튼/수정자 코어)를 구축합니다.
- 명시적으로 활성화된 경우에만 `input`/`ui` 강화를 추가합니다.
- 글로벌 프레스/컨텍스트 시퀀스에 대한 중복 제거 창을 지원합니다.

5. 파견 단계: `ApplicationWindow`
- 앱 수준 누름/컨텍스트 신호에 대한 상시 글로벌 리스너를 호스팅합니다.
- 정규화된 페이로드를 `globalPressedEvent` 및 `globalContextEvent`로 다시 내보냅니다.

6. 기능 단계
- `ContextMenu`: 외부 해제 및 조치 파견.
- 편집자/계층: `WheelScrollGuard`를 통한 중첩 휠 격리.
- 런타임 콘솔/디버그 도구: 이벤트 스트림 시각화.

<a id="canonical-payload-shape"></a>

## 표준 페이로드 형태

전역 포인터/컨텍스트 흐름은 항상 다음을 전달합니다.

- 위치: `x`, `y`, `globalX`, `globalY`
- 입력 마스크: `buttons`, `modifiers`

선택적 강화:

- `input`: 정규화된 입력 상태 스냅샷(`includeInputState=true`)
- `ui`: 적중 테스트 메타데이터(`includeUiHit=true`)
- `src/backend`: 요청 시 선택적 백엔드 요약

이 모양은 기능 구성 요소가 하나의 스키마를 사용할 수 있도록 의도적으로 공유됩니다.

터치/제스처 흐름에는 다음이 추가로 포함됩니다.

- 제스처 ID: `gestureType`, `interactionKind`, `sequence`, `sessionId`
- 기하학: `previous*`, `start*`, `delta*`, `totalDelta*`, `distance`
- 타이밍: `timestampEpochMs`, `durationMs`
- 프레스 수명주기: `pressDurationMs`, `released`, `cancelled`, `releaseEpochMs`
- 방향: `directionX`, `directionY`, `dominantAxis`
- 연락처: `fingerCount`, `activeFingerCount`, `maximumFingerCount`, `multiTouch`, 네이티브 `points[]`
- `scrollAxis`, `scrollDirection`, `swipeDirection`, `velocityX`, `velocityY`와 같은 선택적 의미 확장

<a id="why-backend-first-exists-opt-in"></a>

## 백엔드 우선이 존재하는 이유(선택)

많은 QML 핸들러에서 런타임 싱글턴 상태를 직접 읽으면 버스트 입력 시 일시적인 왜곡이 발생할 수 있습니다. 백엔드 우선 모드는 안정적인 미러링 캐시에서 읽어 왜곡을 줄이지만 핫 경로 오버헤드를 피하기 위해 의도적으로 선택되었습니다.

제스처 수신기는 의도적으로 `Backend`를 통해 미러링되지 않습니다. `GestureEvents`에서 직접 인식된 스트림을 사용합니다.

<a id="context-dismiss-flow-reference"></a>

## 컨텍스트 닫기 흐름(참조)

1. 글로벌 언론/컨텍스트 이벤트는 글로벌 좌표와 함께 도착합니다.
2. 대상 구성 요소는 전역 좌표를 오버레이 로컬 공간에 매핑합니다.
3. 포인트가 팝업/대화 상자 범위를 벗어나면 구성 요소를 닫습니다.
4. 중복 제거 기간은 소스 경로가 겹쳐서 생성된 중복 컨텍스트 이벤트를 억제합니다.

<a id="operational-checks"></a>

## 운영 점검

이벤트 동작을 검증할 때 다음을 확인하세요.

- `RuntimeEvents.running == true`
- 직접 싱글턴 소비자를 위한 `GestureEvents.runtimeAttached == true`
- src/backend/input 강화를 선택한 청취자에게만 `Backend.userEventHooked == true`
- 예상되는 트리거는 중복 제거 기간 내에 정확히 한 번만 실행됩니다.
- 페이로드는 활성화된 경우에만 예상되는 선택적 `ui`/`input` 필드를 전달합니다.

<a id="extended-example-global-context-menu-dispatch"></a>

## 확장된 예: 전역 컨텍스트 메뉴 디스패치

복잡한 페이지의 안정적인 컨텍스트 메뉴 전달 흐름은 일반적으로 다음을 사용합니다.

1. `EventListener(trigger: "globalContextRequested")`
2. 페이로드 UI 적중 테스트 검사(`eventData.ui.path`)
3. 대상 경로/클래스별 메뉴 모델 선택
4. `eventData.globalX/globalY`에서 메뉴 열기

이 흐름은 로컬 이벤트 경계에 대한 종속성을 방지합니다.

<a id="observability-probes"></a>

## 관찰 가능성 프로브

문제 해결 중에 최소한 다음 프로브를 기록하십시오.

- 런타임 시퀀스(`RuntimeEvents.eventSequence`)
- 제스처 시퀀스(`GestureEvents.gestureSequence`)
- 백엔드 미러 수(`Backend.hookedEventCount`)
- 리스너 페이로드 처리의 중복 제거 타임스탬프
- 메뉴/대화상자 외부-형상 검사 해제

<a id="failure-analysis-playbook"></a>

## 실패 분석 플레이북

글로벌 상호 작용이 일관성이 없다고 느껴지는 경우:

1. 런타임 데몬 실행 상태를 확인하세요.
2. 백엔드 우선 리스너가 활성화되면 백엔드 후크 상태를 확인합니다.
3. 중복 제거 임계값이 지나치게 공격적이지 않은지 확인합니다.
4. 상위 오버레이에 대한 좌표 매핑을 확인합니다.

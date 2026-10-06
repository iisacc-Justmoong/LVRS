<a id="overview"></a>

# 개요

LVRS는 결정적 UI 동작, 명시적인 런타임 관찰 가능성 및 보기, 탐색 및 모델 소유권 간의 엄격한 통합 경계를 중심으로 하는 Qt 6.5+ QML 프레임워크입니다.

<a id="design-goals"></a>

## 디자인 목표

- 컨트롤, 레이아웃, 탐색 및 표면 전반에 걸쳐 안정적인 구성 요소 계약이 이루어집니다.
- 백엔드 우선 런타임 관측성은 QML 친화적인 API로 공개됩니다.
- 명시적인 플랫폼/백엔드 정책을 통한 결정적 렌더링 부트스트랩.
- 우발적인 공유 상태 경합을 방지하기 위해 안전한 MVVM 쓰기 소유권.

<a id="runtime-architecture"></a>

## 런타임 아키텍처

LVRS 런타임는 3개의 협력 레이어로 분할됩니다.

1. 이벤트 캡처 데몬 ( `RuntimeEvents` ) 키보드, 포인터, 컨텍스트, 터치/태블레터/ 네이티브 제스처, UI 라이프사이클 및 프로세스 텔레메트리를 캡처합니다.
2. 고수준 제스처 인식기(`GestureEvents`)는 원시 터치/ 네이티브 스트림을 `press`, `scroll`, `hold`, `drag`, `swipe` 및 정규화된 터치 세션 페이로드로드로 분류합니다.
3. 백엔드 캐시/브리지(`Backend`)는 원시 런타임 이벤트 스트림을 한계가 설정된 캐시로 미러링하고 QML 사용자에게 안정적인 스냅샷을 제공합니다.

이 아키텍처는 UI 계층 논리가 두 가지 책임을 하나의 변경 가능한 스트림으로 축소하지 않고도 낮은 수준의 런타임 원격 측정 또는 높은 수준의 제스처 의미 체계를 사용할 수 있도록 존재합니다.

<a id="ui-architecture"></a>

## UI 아키텍처

QML 모듈은 관심사별로 그룹화되어 있습니다.

- `control`: 입력, 디스플레이 컨트롤, 선택 컨트롤 및 동작 가드.
- `layout`: 스택 기본 요소 및 앱 헤더.
- `navigation`: 라우터, 글로벌 탐색기, 계층 구조, 상황에 맞는 메뉴.
- `surfaces`: 카드/대화 상자 스타일 컨테이너.
- `app`: 적응형 탐색 브리지가 있는 루트 창 셸.

<a id="startup-sequence"></a>

## 시동 순서

프로덕션 시작 경로는 이 순서를 따라야 합니다.

1. `QGuiApplication` 이전의 `lvrs::preApplicationBootstrap(options)`.
2. `QGuiApplication`를 구축하세요.
3. `lvrs::postApplicationBootstrap(app, options)`.
4. QML 루트(`LV.ApplicationWindow`)를 로드합니다.
5. 선택적으로 런타임 브리지를 활성화합니다.
   - `RuntimeEvents.start() + attachWindow(window)`
   - `Backend.hookUserEvents()`

<a id="key-runtime-guarantees"></a>

## 키 런타임 보증

- 경로 전환은 `PageRouter.path`, 현재 경로 상태 및 선택적 `ViewStateTracker` 동기화를 업데이트합니다.
- 대화형 페이지 전환은 완료/취소할 때까지 경로 커밋을 연기하면서 앞으로/뒤로 이동을 미리 볼 수 있습니다.
- `ApplicationWindow` 이벤트 브리지를 통해 앱 루트에서 전역 컨텍스트/클릭 신호를 사용할 수 있습니다.
- 중첩된 휠 동작은 `WheelScrollGuard`로 격리할 수 있습니다.
- IME 구성 무결성은 `InputMethodGuard`에 의해 시행될 수 있습니다.
- 모바일 누르기/스크롤 수명주기 및 손가락 수를 포함한 높은 수준의 터치/제스처 의미는 `GestureEvents` 또는 `EventListener` 제스처 트리거를 통해 사용할 수 있습니다.

<a id="recommended-entry-documents-by-use-case"></a>

## 유스케이스별 추천 엔트리 문서

- 런타임 이벤트 통합: `docs/backend/RuntimeEvents.md`, `docs/backend/GestureEvents.md`, `docs/components/control/EventListener.md`.
- 탐색 및 경로 모델 바인딩: `docs/components/navigation/PageRouter.md`, `docs/mvvm.md`.
- 대화형 내비게이션 운전: `docs/components/navigation/PageRouter.md`, `docs/components/navigation/PageTransitionController.md`.
- 플랫폼/백엔드 동작: `docs/architecture/rendering-backend.md`, `docs/backend/Platform.md`.
- 로깅 및 진단: `docs/backend/Debug.md`, `docs/backend/DebugOutput.md`.

<a id="end-to-end-integration-scenario"></a>

## 엔드투엔드 통합 시나리오

일반적인 생산 흐름에서는 모든 런타임 레이어를 순서대로 사용합니다.

1. 부트스트랩 앱 및 렌더링 정책(`AppBootstrap`).
2. 런타임 데몬(`RuntimeEvents`)을 시작하고 연결합니다.
3. 기능에 높은 수준의 터치 의미가 필요한 경우 `GestureEvents`를 연결하세요.
4. 후크 백엔드 이벤트 미러(`Backend.hookUserEvents()`).
5. `PageRouter` 경로를 사용하여 `ApplicationWindow`를 마운트합니다.
6. 경로 메타데이터를 `ViewModels` 소유권에 바인딩합니다.
7. `EventListener`를 통해 전역 컨텍스트/눌림/제스처 이벤트를 처리합니다.

이 체인은 시작부터 UI 상호 작용을 통해 결정적인 동작을 제공합니다.

<a id="common-integration-mistakes"></a>

## 일반적인 통합 실수

- 창을 연결하지 않고 `RuntimeEvents`를 시작한 다음 UI 적중 테스트 데이터를 예상합니다.
- 원시 `RuntimeEvents` 터치 레코드가 높은 수준의 누르기/스크롤/홀드/드래그/스와이프 의미 체계를 소유할 것으로 예상됩니다. 분류에는 `GestureEvents`를 사용합니다.
- `ViewModels`에서 소유권을 주장하지 않은 뷰에서 모델 속성을 작성합니다.
- `WheelScrollGuard` 없이 중첩된 `Flickable` 표면을 사용합니다.
- 전역 좌표 대신 로컬 클릭 핸들러에서만 컨텍스트 메뉴 닫기 로직을 구축합니다.

<a id="verification-checklist"></a>

## 검증 체크리스트

새 앱에 LVRS를 통합한 후 다음을 확인하세요.

- `RuntimeEvents.running == true`
- `GestureEvents.runtimeAttached == true` 제스처 트리거가 `EventListener` 외부에서 사용되는 경우
- 백엔드 우선 리스너가 사용되는 경우 `Backend.userEventHooked == true`
- 경로 전환은 예상대로 `PageRouter.currentPath` 및 `ViewStateTracker.snapshot()`를 업데이트합니다.
- 디버그 출력에는 통제되지 않은 플러드 없이 예상되는 이벤트 도메인이 포함됩니다.

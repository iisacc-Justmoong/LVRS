<a id="event-policy"></a>

# 이벤트 정책

목표: 통합된 런타임 이벤트 계약을 통해 상위 수준 반응을 라우팅하여 결정론적 상호 작용 동작을 보존합니다.

<a id="rule-1-prefer-eventlistener-over-ad-hoc-handlers"></a>

## 규칙 1: 임시 핸들러보다 `EventListener`를 선호합니다.

구성 요소 간 상호 작용 논리는 각 구성 요소에 원시 `MouseArea`/`Keys` 배관을 복제하는 대신 `EventListener` 트리거 의미 체계를 사용해야 합니다.

이유:
- 중앙 페이로드 모양,
- 일관된 src/backend/runtime 대체 경로,
- 공유 중복 제거 동작.

<a id="rule-2-use-runtimegesture-triggers-for-cross-surface-behavior"></a>

## 규칙 2: 교차 표면 동작에는 런타임/제스처 트리거를 사용하세요.

오버레이 해제, 전역 컨텍스트 메뉴 제어, 앱 수준 상호 작용 후크 및 중첩된 보기 경계에서 살아남아야 하는 모바일 터치 의미 체계의 경우 다음을 사용합니다.

- `globalPressed`
- `globalContextRequested`
- `touchStarted`, `pressStarted`, `pressEnded`, `holdStarted`, `dragStarted`, `dragUpdated`, `dragEnded`
- `scrollStarted`, `scrollUpdated`, `scrollEnded`
- `swipeDetected`, `nativeGestureDetected`

이러한 트리거는 중첩된 로컬 이벤트 경계에 탄력적이며 프레임워크 관리 페이로드 계약을 공유합니다.

<a id="rule-3-incident-payload-is-default-srcbackendinput-enrichment-is-opt-in"></a>

## 규칙 3: 사고 페이로드가 기본값입니다. src/backend/input 강화는 선택 사항입니다.

`EventListener`는 기본적으로 사고 중심을 유지해야 합니다. 콜백에 실제로 일관된 입력 스냅샷이 필요한 경우에만 `includeInputState` / `preferBackendState`를 활성화하세요.

<a id="rule-4-outside-dismiss-must-be-coordinate-based"></a>

## 규칙 4: 외부 해제는 좌표 기반이어야 합니다.

팝업/대화상자 해제는 로컬 전용 클릭 가정이 아닌 전역-로컬 좌표 매핑 및 형상 확인을 사용해야 합니다.

<a id="rule-5-nested-wheel-isolation-is-mandatory"></a>

## 규칙 5: 중첩된 휠 격리는 필수입니다.

더 큰 스크롤 가능 페이지 내에 내부 스크롤 뷰포트를 소유한 모든 구성 요소는 이중 스크롤을 방지하기 위해 휠 격리(`WheelScrollGuard`)를 설치해야 합니다.

<a id="rule-6-text-composition-safety-is-mandatory"></a>

## 규칙 6: 텍스트 작성 안전은 필수입니다.

모든 텍스트 입력 표면은 `InputMethodGuard` 를 포함해야 하므로 IME 작성이 가시성/초점 전환 시 안전하게 커밋됩니다. 텍스트 입력 표면은 네이티브 `MouseArea`, `TextInput`, 또는 `TextEdit` 위에 전체 덮개 프레스/탭 핸들러(예: `TextArea`)를 배치해서는 안 되며, 선택, IME, 키보드, 더블 클릭, 트리플 클릭 및 플랫폼 텍스트 제스처는 네이티브 텍스트 항목에 직접 도달해야 합니다. 네이티브 텍스트 상호작용 모드는 네이티브 텍스트 항목보다 먼저 키보드 단축키를 소비해서는 안 됩니다. 시각 상태에 대한 비 잡기 호버 관찰은 네이티브 제스처가 활성화되지 않은 경우에만 허용됩니다.

<a id="rule-7-dedup-windows-must-remain-explicit"></a>

## 규칙 7: 중복 제거 기간은 명시적으로 유지되어야 합니다.

글로벌 프레스/컨텍스트 중복 제거 임계값(시간 및 거리)은 계약 매개변수입니다. 모든 변경 사항은 문서화하고 상황에 맞는 메뉴 동작에 대해 테스트해야 합니다.

<a id="concrete-implementation-pattern"></a>

## 구체적인 구현 패턴

오버레이/팝업의 경우 선호되는 패턴은 다음과 같습니다.

1. 열기를 위한 로컬 작업 트리거,
2. 외부 해제를 위한 전역 트리거,
3. 좌표 기반 기하학 검사,
4. 중첩된 스크롤 영역에 대한 명시적인 이벤트 소비 전략입니다.

<a id="review-checklist"></a>

## 체크리스트 검토

코드 검토 중에 다음과 같은 변경 사항을 거부하세요.

- `EventListener` 없이 페이지당 중복 글로벌 이벤트 배관,
- 전역 해제 동작에 로컬 전용 좌표를 사용합니다.
- 중첩된 `Flickable` 표면에서 `WheelScrollGuard`를 제거하고,
- 편집 가능한 텍스트 컨트롤에서 IME 컴포지션 가드를 비활성화합니다.
- 편집 가능한 텍스트 컨트롤 위에 전체 커버 누르기/탭 핸들러를 배치합니다.
- 네이티브 상호 작용 모드에서 네이티브 텍스트 항목 이전에 편집 가능한 텍스트 키 이벤트를 사용합니다.

<a id="enforcement-note"></a>

## 시행 참고 사항

이벤트 동작은 고주파수 입력에서 결정성을 유지해야 합니다. 클릭 속도 또는 스크롤 버스트에 따라 동작이 변경되면 파이프라인 설계가 불완전합니다.

<a id="audit-checklist"></a>

## 감사 체크리스트

- 전역 동작은 로컬 클릭 해킹이 아닌 전역 트리거를 사용합니다.
- 중첩된 스크롤 표면에는 명시적인 휠 격리가 포함됩니다.
- 텍스트 입력 구성 요소에는 IME 가드 통합이 포함됩니다.
- 편집 가능한 텍스트 컨트롤은 포인터 및 제스처 입력을 직접 수신합니다.
- 텍스트 입력 테스트는 직접 네이티브 항목 소유권, 클릭을 통한 포커스, 키보드 입력, IME 사전 편집/커밋, 두 번 클릭 선택, 세 번 클릭 선택 상태 전달, 드래그 선택, 바로가기 통과 및 네이티브 텍스트 모드 스크롤 경쟁을 다룹니다.

<a id="component-instance-states"></a>

# 구성요소 인스턴스 상태

LVRS Figma 라이브러리와 런타임 는 각 완전한 컴포넌트 인스턴스가 소유한 상태를 사용합니다. 레이블, 아이콘, 단축키, 공개 및 보조기는 모든 입력 단계를 통해 해당 인스턴스 내부에 남아있습니다. 호버, 누르기, 릴리스 또는 키보드 초점을 나타내기 위해 컴포넌트 위에 두 번째 상호작용 컴포넌트가 배치되지 않습니다.

<a id="figma-contract--2026-09-30"></a>

## Figma 계약 — 2026-09-30

[LVRS 라이브러리](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System) 는 `Interaction state=default/hover/press/release/focus` 를 사용합니다. 행 가족은 선택/비활성 상태와 무관하게 추가로 `Input=pointer/keyboard` 를 가집니다. 키보드 포커스 장식이 행에 속하며 누름과 해제 동안 계속 표시됩니다. 루트 필드는 바인딩된 변수를 유지합니다. 프로토타입 `CHANGE_TO` 동작은 같은 컴포넌트 세트의 변형을 타겟합니다.

|제품군|부품 세트|전체 변형|검토 프레임|
| --- | ---: | ---: | --- |
| PushButton |2(레이블/아이콘)| 50 |[버튼](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-728)|
| DropdownButton | 1 | 30 |버튼 페이지|
| ListItem |17(유형당 1개)| 510 | [ListItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-6737) |
| MenuItem |2 (축소/확장)| 60 | [MenuItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-7886) |
| ContextMenuItem | 1 | 30 | [ContextMenuItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-8219) |
| HierarchyItem |2 (접이식/비접이식)| 60 | [HierarchyItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-9422) |

각 세트는 최대 30 개의 변형을 가집니다. ListItem 타입 또는 라벨/아이콘 버튼 계열을 컴포넌트 세트를 교체하여 변경하고, 해당 인스턴스의 `Interaction state` 를 변경하여 입력을 미리 봅니다. 원본 컴포넌트 ID 는 세트에 그대로 남아 있습니다. 기존 539 소비자 인스턴스는 위상 및 사용자 정의 속성을 함께 마이그레이션되었습니다. 텍스트/가시성 속성 오버라이드가 모든 740 변형에서 실행되었습니다. 계열을 분리한 후 다른 타입의 사용하지 않는 속성이 제거되었으며, 각 세트는 실제 콘텐츠 속성과 상태 축만 노출합니다. 이 정리 후 모든 740 인스턴스 오버라이드가 다시 확인되었습니다.

참조가 0에 도달한 후 버튼, 목록, 메뉴, ContextMenu 및 계층 구조 상호작용 헬퍼 세트가 제거되었습니다. 23 컴포넌트 페이지에 대한 감사 결과 해당 헬퍼에 대한 남은 참조는 없습니다. 알림, 카드, ColorPicker 및 탭은 변환된 버튼 인스턴스를 상속합니다. 팝업/모달 오버레이와 가져온 플랫폼 컴포넌트의 내부 상태 레이어 프레임은 서로 다른 목적을 수행하며 유지됩니다. 나머지 7 썸네일, 구분선, 아바타, 색상, 아이콘 및 텍스트 페이지도 검사되었으며 상호작용 보조 컴포넌트를 포함하지 않습니다. 아바타는 비어 있습니다.

## Runtime API

`AbstractButton`는 인스턴스당 하나의 비시각적 `InteractionState` 개체를 생성합니다. PushButton, DropdownButton, 명명된 레이블/아이콘 사전 설정, MenuItem, ContextMenuItem, ListItem 및 HierarchyItem는 ​​이 소유권을 상속합니다.

|속성|의미|
| --- | --- |
| `interaction` |인스턴스의 상태 개체입니다. 공유되지 않음 싱글턴|
| `interactionPhase` |`disabled`, `press`, `release`, `focus`, `hover` 또는 `default`, 우선 순위|
| `interactionInput` |`keyboard` 네이티브 시각적 초점이 활성화된 동안; 그렇지 않으면 `pointer`(터치 포함)|
| `interaction.focusVisible` |누르기/손 떼기와 관계없이 키보드 포커스 활성화됨|
| `interaction.surfacePhase` |기존 표면 색상 정책 `disabled`, `press`, `hover` 또는 `default`|

활성화된 게이트는 입력보다 우선합니다. HierarchyItem 또한 활성화될 수 없거나 드래그 미리보기를 표시할 때 상호작용을 게이트합니다. 기존 드래그 및 선택 API 는 독립적으로 유지됩니다. MenuItem 의 정수 `state`, ListItem 의 `selected` / `type` /액세서리 값 및 HierarchyItem 의 `uxState` /확장은 입력 단계에 의해 대체되지 않습니다.

소유된 페이즈는 컴포넌트의 기존 배경 및 내부 포커스 장식을 구동합니다. 포커스와 릴리스는 새로운 영속성 필을 도입하지 않습니다. `surfacePhase` 은 호버 필을 유지 가능하게 유지하면서 `interactionPhase` 에서 포커스 또는 릴리스가 우선순위를 가집니다. 체크된 버튼은 기존 체크된 필을 유지하며, 내장된 ListItem 토글은 포함된 행을 색칠하지 않습니다. 콘텐츠 클리핑은 콘텐츠에 범위가 지정되어 있어 컴포넌트의 외부 포커스 윤곽선이 변경 없이 히트 또는 레이아웃 범위를 유지하며 가시화됩니다.

`InteractionMotion.releasing` 는 실행 중인 릴리스 애니메이션을 보고합니다. 진정한 포인터, 터치, 스페이스 또는 엔터 릴리스는 작성된 가족에 기존 180ms OutBack 반환을 제공합니다. 취소는 릴리스를 건너뜁니다. 재가압은 이를 중단하며, 운동 비활성화는 즉시 정지시키고 논리적 가압/포커스는 계속 작동합니다. 타이밍 및 접근성 설정에 대한 [운동 정책](motion.md) 을 참조하십시오.

```qml
import LVRS 1.0 as LV

LV.ListItem {
    label: "Document"
    selected: true
    onInteractionPhaseChanged: console.log(interactionPhase, interactionInput)
}
```

<a id="verification"></a>

## 검증

2026-10-03 Menus 수정에서 일반 `MenuItem`의 Selected 배경은 Press와 동일한 `Fill/pressed` / `Theme.accentMuted`(기본 `#25324D`)로 통일하였다. 축소·확장 두 세트의 Selected 변형 20개와 페이지 내 기존 인스턴스 21개를 확인하였다. `LVRSTests_instance_state::menu_selected_uses_press_fill`은 축소·확장 및 기본·사용자 지정 Primary에서 실제 포인터·키보드의 기본/호버/누름/해제/포커스 단계와 렌더링 픽셀을 검증한다. 별도 컴팩트 `ContextMenuItem`의 기존 Primary 선택색도 함께 검증한다.

이 색상 수정의 전체 빌드와 관련 CTest 다섯 개(`import_api`, `primary_color`, `motion`, `instance_state`, `figma_parity`)가 통과하였다. 새 테스트의 Tab 진입 절차를 수정한 뒤 `instance_state`를 재검증하였고, macOS 네이티브 Cocoa 창에서도 축소·확장 및 기본·사용자 지정 Primary의 네 시나리오가 렌더링 픽셀 확인을 포함해 통과하였다. Figma 변경 기록은 `build/menu-selected/figma-audit.json`, 실행 기록은 `build/menu-selected-ctest.log`, `build/menu-selected-instance-ctest.log`, `build/menu-selected-native.txt`에 있다.

`LVRSTests_instance_state` 는 Qt 를 사용하여 실제 Qt 와 빠른 창을 통해 포인터 및 키보드 단계를 연습하고, 소유권을 분리하며, 모든 17 ListItem 타입, 릴리스 시 활성화, 취소, 재 누름, 감소된 동작, 비활성화 게이트, 초점 유지 및 고정 타격 기하학을 실행합니다. 그는 `tests/fixtures/figma-instance-state.json` 에서 기록된 Figma 계약을 확인하며, 픽스처 는 날짜가 있는 디자인 감사이며, Figma 쿼리가 아닙니다. `ctest --test-dir build -R instance_state --output-on-failure` 를 실행합니다. 기존 동작, Figma 동등성, 합성 목록 및 계층 구조 수트는 시각적 값, 액세서리 라우팅 및 모델 동작을 다룹니다. 생성 출력은 설치된 LVRS 패키지에 독립적입니다; 재설치는 별도의 소비자 작업입니다.

네이티브 렌더링된 초점 증명에 대해 `build/` 하위의 디렉토리에 `LVRS_STATE_CAPTURE_DIR` 를 설정하고, 네이티브 플랫폼과 이 빌드의 라이브러리/수입 경로를 사용하여 `build/tests/LVRSTests_instance_state native_focus_capture` 를 실행합니다. 캡처는 각 6 컴포넌트 계열 외부의 실제 기본 색상 픽셀을 확인하고 초점 프레임을 저장합니다. 이 확인은 일반적인 오프스크린 수트에서 옵트-인입니다.

macOS 에서 Qt 6.8.3 로 검증: 전체 빌드가 통과했습니다. 모든 56 CTest 타겟이 실행되었으며, 초기 실행은 53를 통과했고, 3 실패 (예시/플랫폼 통합 및 애니메이션 시작 타이밍 주장을 포함한 카탈로그 카운트) 는 수정 및 타겟 재실행 후 통과했습니다. 인스턴스 상태 수트와 네이티브 렌더링된 초점 확인이 통과했습니다. `build/figma-instance-state/verification.json` 는 이러한 별도 실행, Figma 감사 및 네이티브 캡처를 기록합니다. 설치는 사용자의 요청한 재설치 단계에 맡겨둡니다.

# MenuItem

위치: `src/qml/components/navigation/MenuItem.qml`

`MenuItem`는 `ContextMenu` 대리인이 사용하는 상황에 맞는 메뉴 행 구성 요소입니다.

<a id="purpose"></a>

## 목적

- 선택/비활성 상태로 아이콘/레이블/키 행을 렌더링합니다.
- 키 가시성과 쉐브론 가시성을 독립적으로 제어합니다.
- 방향이 `auto`인 경우 접기/확장 상태에서 쉐브론 방향을 구동합니다.

<a id="core-api"></a>

## 코어 API

상태 상수 및 속성:

- `defaultState`, `selectedState`, `inactiveState`
- `state`

방향 상수 및 속성:

- `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection`(`int` 또는 문자열: `auto|right|left|up|down`)

내용:

- 표준 입력: 아이콘 문자열, 라벨 문자열, 키 가시성 부울, 키 문자열
- `label`
- `key` / `shortcut`(별칭, 기본값 `"key"`)
- `keyVisible`(부울, 기본값 `true`)
- `keyPlaceholder`(기본값 `""`)
- `iconName`(기본값 `"procedure"`), `iconSource`
- `showIconSlot`(부울, 기본값 `true`)
- `showChevron`(기본값 `true`)
- `hasChildItems`(기본값 `true`)
- `expanded`
- `effectiveShowChevron`(읽기 전용: `showChevron && hasChildItems`)

레이아웃 및 시각적 요소:

- `itemWidth`, `itemHeight`
- `iconSize`, `chevronSize`
- `iconPlaceholderColor`, `chevronColor`
- 해결됨: `effectiveShowIconSlot`, `resolvedIconSlotWidth`, `resolvedIconLabelGap`, `resolvedIconSource`, `resolvedShortcutText`, `resolvedSelectionDirection`, `resolvedChevronRotation`, `resolvedBackgroundColor`

<a id="behavior-contract"></a>

## 행동 계약

- Figma 소스: 노드 `107:498`(`MenuItem` 구성 요소 세트), 기본 변형은 노드 `107:495`에 있습니다.
- 독립 실행형 기본값은 측정된 Figma 인스턴스(`label: "Label"`, `key: "key"`, `keyVisible: true`, `iconName: "procedure"`) 및 표시되는 오른쪽 갈매기형입니다. `ContextMenu` 모델 대리인은 여전히 ​​정규화된 항목 값을 명시적으로 제공합니다.
- 데스크톱과 모바일은 동일한 `161 x 24` 프레임과 `13px` 본문 타이포그래피를 사용합니다.
- 일반 `MenuItem`의 `selected` 배경은 Press와 동일한 `Theme.accentMuted`이다. 기본 Primary에서는 짙은 파란색 `#25324D`이며, 사용자 지정 Primary에서는 Press와 함께 갱신된다. `inactive` 배경은 기존 토큰을 유지한다.
- 아이콘 프레임은 데스크톱에서는 `18 x 18`, 모바일에서는 `36 x 36`입니다. 쉐브론 프레임은 데스크톱에서는 `16 x 16`이고 모바일에서는 `32 x 32`입니다.
- 행은 데스크톱 및 모바일에서 `4px` 가로 및 `3px` 세로 패딩을 사용합니다.
- Figma 패리티 픽스처는 `procedure.svg`(아이콘 `x=4, y=3, 18 x 18`, 레이블 `x=30, y=5.5, 33 x 13`, 바로 가기 `x=112, y=5.5, 21 x 13` 및 `x=141, y=4, 16 x 16`의 `generalchevronRight.svg`)를 사용합니다.
- 레이블과 바로가기는 모두 고정된 본문 계약(Pretendard 중간 `13px`, `13px` 줄 높이 및 문자 간격 0)을 사용합니다. `labelMetricCompensation`는 호환성을 위해 계속 사용 가능하지만 `0`로 확인됩니다.
- `implicitWidth`는 아이콘/레이블/키/쉐브론 콘텐츠에 더 많은 공간이 필요한 경우 `itemWidth` 이상으로 확장됩니다.
- 자연 레이블 및 바로 가기 너비는 표시된 생략된 텍스트와 독립적으로 측정되므로 제한된 렌더링은 `implicitWidth`로 피드백되지 않습니다.
- 행 레이아웃은 제한된 너비에서 반응합니다. 레이블 및 바로가기 텍스트는 행 내에서 사라지고, 후행 메타데이터는 항목 프레임 내부에 유지되며, 유연한 스페이서는 절대 음수 너비로 확인되지 않습니다.
- `showIconSlot: false`는 아이콘 이미지뿐만 아니라 왼쪽 아이콘 슬롯 자체도 제거합니다. 행은 아이콘 너비나 아이콘-레이블 간격을 예약하지 않으며 레이블은 행 원점에서 시작됩니다.
- 레이블과 바로가기는 모두 `body` 스타일 토큰을 사용합니다.
- 아이콘 소스를 확인할 수 없는 경우 작은 원형 강조 자리 표시자가 표시됩니다.
- `keyVisible`가 `false`이면 키 텍스트가 숨겨집니다.
- `keyVisible`가 `true`이고 `key`가 비어 있으면 `keyPlaceholder`가 렌더링됩니다.
- 쉐브론은 `effectiveShowChevron`가 `true`인 경우에만 표시됩니다.
- `selectionDirection: "auto"`는 `expanded=false`를 오른쪽으로, `expanded=true`를 아래쪽으로 매핑합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.MenuItem {
    label: "Open Recent"
    showIconSlot: false
    key: "Cmd+O"
    keyVisible: true
    hasChildItems: true
    showChevron: true
    expanded: false
    selectionDirection: "auto"
    state: selectedState
}
```

<a id="shared-motion"></a>

## 공유 모션

`interaction`, `interactionPhase` 및 `interactionInput`는 인스턴스별로 소유됩니다. 이는 기존 정수 `state`(기본값/선택됨/비활성화)와는 별개입니다. Figma의 축소/확장 세트는 완전한 입력 변형을 보유합니다. 참조
[구성 요소 인스턴스 상태](../../instance-states.md).

Figma 상태는 `state` 선택 모델과 독립적입니다: 기본값은 작성된 투명 채우기를 사용하며, 호버는 `Theme.surfaceAlt` 를, 누름은 `Theme.accentMuted` 를, 릴리스는 현재 호버/기본 채우기를 180ms OutBack 변형만 함께 복원합니다. 일반 `MenuItem`의 선택된 행은 축소·확장, 포인터·키보드 및 모든 입력 단계에서 Press와 동일한 `Theme.accentMuted`를 유지한다. Figma Menus의 두 세트에 있는 Selected 변형 20개는 `Fill/pressed` 토큰에 연결되어 있다. 별도 컴팩트 디자인인 `ContextMenuItem`은 기존 `Theme.contextMenuItemSelectedBackground`(`Theme.primary`)를 유지한다. 비활성 행은 작성된 채우기와 타이포그래피를 유지하지만, 활성화하거나 호버하거나 변형되거나 Tab 초점을 받을 수 없습니다. 확장과 기존 레이블/아이콘/단축키 속성은 독립적으로 유지됩니다.

키보드 Tab/Shift+Tab은 포커스를 이동한다. Space와 Enter는 즉시 누름 상태로 만들고 키를 놓을 때 활성화한다. Enter를 누른 상태에서 포커스를 잃으면 취소한다. 1.5px Primary 포커스 링은 고정된 행 경계 바깥 3px에 위치하며 누름/해제 중에도 보인다. `motionEnabled`·전역 모션 감소·로컬 색상 덮어쓰기를 계속 지원한다.

채우기 혼합, 행 리바운드 및 공개 회전은 확장을 따릅니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

`compact` 기본값은 false 입니다. ContextMenuItem 실제 컴팩트 Figma 계열을 선택하며: 18px 행, 반지름 0, Inter 일반 12 레이블 및 Pretendard SemiBold 12 단축키입니다. 일반 기본값은 24px 에 Pretendard 중간 13를 유지합니다. 너비 측정은 렌더링된 레이블과 단축키와 동일한 글꼴을 사용합니다.

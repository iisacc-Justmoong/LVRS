<a id="hierarchy"></a>

# 계층 구조

위치: `src/qml/components/navigation/Hierarchy.qml`

`Hierarchy`는 도구 모음 + 스크롤 가능한 깊이 인식 계층 목록으로 구성된 계층 패널 표면입니다.

<a id="purpose"></a>

## 목적

- 명시적인 확장/축소 컨트롤을 사용하여 깊이 배열/목록/모델 계층 구조 데이터를 렌더링합니다.
- 호스트 기능에 대한 활성화 및 확장 콜백을 제공합니다.
- 중첩된 스크롤 컨테이너 내에서 트리 탐색을 사용할 수 있도록 유지합니다.

## API

필수 입력:

- 도구 모음 선언: `toolbarItems`(배열 모델) 또는 `toolbarButtons`(수동 `ToolbarButton` 하위)
- 목록 모델 바인딩: `model` / `treeModel`

모델 및 선택 별칭:

- `model` / `treeModel`
- `modelColumn`
- `activeListItem`
- `activeListItemId`
- `activeListItemKey`

모델 역할 별칭:

- `itemIdRole`
- `itemKeyRole`
- `labelRole`
- `iconNameRole`
- `iconSourceRole`
- `iconGlyphRole`
- `depthRole`
- `countRole`
- `enabledRole`
- `expandedRole`
- `selectedRole`
- `activatableRole`
- `draggableRole`
- `showChevronRole`

도구 모음 별칭:

- `toolbarButtons`
- `toolbarItems`
- `activeToolbarButton`
- `activeToolbarButtonId`
- `activeToolbarIndex`

동작 별칭:

- `keyboardListNavigationEnabled`
- `editable`
- 목록 스크롤 물리학: `listOvershootEnabled`, `listFlickDeceleration`, `listMaximumFlickVelocity`, `listReboundDuration`
- 읽기 전용 목록 뷰포트 정책: `listBoundsBehavior`, `listBoundsMovement`

선택적 바닥글:

- `footerVisible`
- `footerInteractive`
- `footerButton1`
- `footerButton2`
- `footerButton3`

방법:

- `expandAll()`
- `collapseAll(keepRootExpanded)`
- `activateListItemById(itemId)`
- `activateListItemByKey(itemKey)`
- `triggerFooterButton(index)`

신호:

- `toolbarActivated(button, buttonId, index)`
- `toolbarButtonTriggered(button, buttonId, index, item)`
- `toolbarEventTriggered(eventName, payload, index, item, buttonId)`
- `listItemActivated(item, itemId, index)`
- `listItemExpanded(item, itemId, index, expanded)`
- `listItemMoved(item, itemId, itemKey, fromIndex, toIndex, depth)`
- `footerButtonTriggered(index, config)`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Hierarchy {
    countRole: "counter"
    toolbarItems: [
        { id: "structure", iconName: "projectStructure", eventName: "hierarchy.structure" },
        { id: "layers", iconName: "projectStructure", events: ["hierarchy.layers", "analytics.layers"] }
    ]
    model: [
        { key: "root", depth: 0, label: "Root", expanded: true, counter: 2 },
        { key: "child", depth: 1, label: "Child", counter: 7 }
    ]
    footerVisible: true
    footerButton1: ({ type: "icon", iconName: "projectStructure" })
    footerButton2: ({ type: "icon", iconName: "delete" })
    footerButton3: ({ type: "menu", iconName: "viewMoreSymbolicDefault" })
}
```

```qml
import LVRS 1.0 as LV

LV.Hierarchy {
    model: backendHierarchyModel
    itemKeyRole: "key"
    labelRole: "name"
    depthRole: "depth"
    countRole: "counter"
}
```

<a id="how-it-works"></a>

## 동작 원리

- Figma 패널(`180:1012`)은 `Theme.panelBackground05`를 사용하여 데스크톱 및 모바일에서 `200x530`입니다.
- 해당 도구 모음은 상단 `26px`(`52px` 모바일)을 차지합니다. 목록은 바로 아래에서 시작하고 16개 행 참조 콘텐츠가 `320px`(`640px` 모바일)를 차지하므로 나머지 패널 높이를 스크롤 뷰포트에 사용할 수 있습니다.
- 도구 모음과 목록은 명시적인 신호와 전달된 별칭을 통해 통신합니다.
- `model`, `modelColumn` 및 역할 별칭은 내부 `HierarchyList`로 직접 전달되므로 어레이, QML `ListModel` 및 C++ `QAbstractItemModel` 소스를 패널 수준에서 삽입할 수 있습니다.
- `ensureListItemVisible`는 목록이 가시성을 요청할 때 깜박일 수 있는 뷰포트를 조정합니다.
- 중첩된 스크롤 블리드를 방지하기 위해 `WheelScrollGuard`가 설치되었습니다.
- 옵션 `ListFooter`는 왼쪽 하단에 고정되어 있습니다. 표시되면 목록 뷰포트가 바닥글 상단에서 끝납니다.
- `editable`는 생성된 `HierarchyItem` 행에서 항목 소유 드래그/드롭을 활성화합니다. 기본 모델은 객체 배열, QML `ListModel`/목록 유사 모델 또는 쓰기 가능한 깊이 역할 및 행 이동이 있는 C++ `QAbstractItemModel` 등 깊이 상태가 있는 변경 가능한 객체 행을 노출해야 합니다.
- 모바일 대상 실행에서 편집 가능한 행 드래그에는 `HierarchyItem`가 드래그/드롭 모드로 들어가기 전에 `1000ms`를 길게 눌러야 하며, 이는 패널 `Flickable` 내에서 터치 스크롤 우선 순위를 유지합니다.
- 모바일 대상 실행에서 행 활성화는 누르는 대신 놓기/클릭할 때도 커밋되므로 활성 행이 변경되기 전에 목록 스크롤 경로에 의해 수직 드래그가 계속 요구될 수 있습니다.
- 이제 모바일 대상 목록 스크롤을 사용하면 가장자리에서 iOS와 유사한 관성 느낌을 위해 오버슈트 + 리바운드(`DragAndOvershootBounds`/`FollowBoundsBehavior`) 및 조정된 플릭 모멘텀(`listFlickDeceleration`, `listMaximumFlickVelocity`)이 가능합니다.
- `listItemActivated`는 이미 활성화된 행에서 의도적인 반복 활성화 제스처를 미러링하므로 패널 호스트는 선택 항목을 변경하지 않고도 두 번째 탭/클릭을 작업 트리거로 처리할 수 있습니다.

<a id="advanced-usage-programmatic-activation"></a>

## 고급 사용법: 프로그래밍 방식 활성화

```qml
import LVRS 1.0 as LV

LV.Hierarchy {
    id: tree
}

function focusNodeByKey(key) {
    tree.activateListItemByKey(key)
}
```

<a id="operational-notes"></a>

## 운영 참고 사항

- 안정적인 프로그래밍 방식 활성화를 위해 항목 ID/키를 안정적으로 유지하세요.
- 온보딩 흐름에서 `expandAll()`와 `activate*()`를 결합하여 결정론적으로 딥 노드를 표시합니다.

<a id="failure-pattern"></a>

## 실패 패턴

형제 노드에 고유하지 않은 키를 사용하면 프로그래밍 방식 활성화 및 확장 추적이 중단됩니다. 각 논리 노드에 안정적인 고유 식별자를 할당합니다.

<a id="shared-motion"></a>

## 공유 모션

행이 리바운드되고, 공개 화살표가 회전하고, 오버스크롤된 콘텐츠가 공유된 타이밍으로 반환됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

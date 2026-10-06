# HierarchyList

위치: `src/qml/components/navigation/HierarchyList.qml`

`HierarchyList`는 수동 자식, 플랫 JavaScript/list 모델 또는 명시적인 깊이 값이 있는 직접 `QAbstractItemModel`의 `HierarchyItem` 행을 렌더링하는 깊이 인식 뷰 목록입니다.

<a id="purpose"></a>

## 목적

- 깊이 배열/목록 모델을 눈에 보이는 계층 구조 행으로 렌더링합니다.
- QML `ListModel` 및 C++ `QAbstractItemModel` 소스를 직접 읽으세요.
- 활성화, 가시성 및 확장 상태를 효율적으로 유지합니다.
- 키보드 탐색 및 상위 자동 확장 동작을 제공합니다.

<a id="core-api"></a>

## 코어 API

모델 및 역할:

- `model`(기본 트리 입력), `treeModel`(호환 별칭)
- `modelColumn`(`QAbstractItemModel`를 읽을 때 사용되는 열, 기본값 `0`)
- `itemIdRole`, `itemKeyRole`, `labelRole`, `iconNameRole`, `iconSourceRole`, `iconGlyphRole`, `countRole`
- `enabledRole`, `expandedRole`, `selectedRole`, `activatableRole`, `draggableRole`, `showChevronRole`
- `depthRole`(기본값 `depth`)

생성된 행 기본값:

- `generatedIndentStep`(기본값 `8`), `generatedRowHeight`(기본값 `20`), `generatedItemWidth`(기본값 `200`)
- `generatedIconSize`(데스크탑 및 모바일의 경우 기본 `Theme.iconSm`: `18`)
- `generatedChevronSize`(데스크톱 `16`, 모바일 `32`)
- `itemDelegate`: 선택적 생성 행 구성 요소 위임. 기본 대리인은 `HierarchyItem`입니다.

상태:

- `activeItem`, `activeItemId`, `activeItemKey`
- `itemCount`, `visibleItemCount`(읽기 전용)
- `keyboardNavigationEnabled`
- `autoExpandAncestorsOnActivate`
- `editable`(배열 지원 객체 깊이 모델에 대한 드래그 기반 깊이 편집 가능)

구성:

- `default property alias items`(수동 행 모드)

신호:

- `activeChanged(item, itemId, index)`
- `expansionChanged(item, expanded, index)`
- `ensureVisibleRequested(y, height)`
- `itemMoved(item, itemId, itemKey, fromIndex, toIndex, depth)`

기본 방법:

- 활성화: `requestActivate(item)`, `activateById(itemId)`, `activateByKey(itemKey)`
- 확장: `expandAll()`, `collapseAll(keepRootExpanded)`
- 탐색: `navigateLeft()`, `navigateRight()`, `activateRelativeVisible(step)`
- 조회 도우미: `resolveById(...)`, `resolveByKey(...)`, `indexOfItem(...)`, `isItemVisible(...)`
- 트리 관계: `parentItem(item)`, `firstChildItem(item)`

<a id="behavior-contract"></a>

## 행동 계약

- `model`가 있습니다. 목록이 관리형 `HierarchyItem` 인스턴스를 생성합니다.
- `model`가 비어 있음: 수동으로 슬롯된 `items`를 관리되는 행으로 사용합니다.
- 모델 입력은 플랫 JavaScript 배열, 기본 배열, QML `ListModel`/목록형 객체 또는 C++ `QAbstractItemModel`일 수 있습니다.
- 모델 행은 배열, 목록형 객체 및 `QAbstractItemModel` 소스에 대해 `ModelSource`를 사용하는 C++ `HierarchyModel`에 의해 투영됩니다.
- 가시성, 조회 맵, 상위/경로 메타데이터, 형제/하위/하위 항목 수, 하위 범위, 드래그 대상 계산, 편집 가능한 설명자 이동 및 직접 모델 쓰기 저장은 초기 빌드 및 소스 형태 변경을 위해 `HierarchyModel`에 의해 투영됩니다.
- 모델 행은 최상위 기본 요소가 아닌 이상 명시적인 깊이 데이터(먼저 `indentLevel`, 그 다음 `depthRole`)가 필요합니다.
- 객체 행의 레이블 대체 경로 순서는 `labelRole`, `text`, `title`, `name`, `display`, `edit`입니다.
- `QAbstractItemModel` 변경(`rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged`, `dataChanged`)은 C++ 모델 투영을 무효화하고 생성된 행을 다시 빌드합니다.
- 생성된 모델 행은 소스 설명자당 한 번씩 `itemDelegate`를 인스턴스화합니다.
- `itemDelegate`는 주입된 `modelData` 및 `index`와 더불어 기본 행이 사용하는 동일한 `HierarchyItem` 호환 속성(ID, 레이블/아이콘/개수, 깊이, 확장, 선택, 활성화, 활성화, 드래그 어포던스 및 생성된 지오메트리 기본값)을 받습니다.
- 사용자 정의 계층 대리자는 `HierarchyItem`를 직접 사용하거나 `__isHierarchyItem`를 포함한 동일한 행 계약을 노출해야 합니다. 왜냐하면 `HierarchyList`는 여전히 라이브 행 항목을 통해 활성화, 확장, 가시성 및 편집 가능한 끌기 상태를 관리하기 때문입니다.
- 하위 존재 여부는 들여쓰기/순서로 추론되며 각 행 `hasChildItems`에 동기화됩니다.
- 생성된 행 기본값은 목록에서 명시적으로 재정의되지 않는 한 `HierarchyItem` 기본값을 미러링합니다.
- Figma 목록(`180:995`)은 `200x320`입니다. 데스크톱 및 모바일에서 `rowSpacing = 0`가 포함된 16개의 `200x20` 행입니다.
- 생성된 행은 `countRole`(기본값 `count`)에서 후행 숫자 카운터를 읽고 이를 `HierarchyItem.count`로 전달합니다.
- 새로고침 시 관리된 행에는 `parentItemKey`, `parentLabel`, `parentPathLabel`, `pathLabel`, `ancestorItemKeys`, `ancestorLabels`, `pathItemKeys`, `pathItemLabels`, `childCount`, `visibleChildCount`, `descendantCount`, `visibleDescendantCount`, `childItemKeys`, `childItemLabels`, `flatIndex`, `visibleIndex`, `siblingIndex`, `visibleSiblingIndex`, `siblingCount`, `visibleSiblingCount` 항목 메타데이터가 부가됩니다.
- 가시성은 조상 확장 상태로부터 계산되며 점진적으로 캐시됩니다. 초기 투영, 모델 무효화, 역할 변경, 대리자 변경 및 구조적 재순서는 전체 `HierarchyModel` 투영을 사용하며, 단일 행 확장/축소 새로고침은 전환된 행의 후손 범위, 영향받는 가시성 인덱스 꼬리 및 조상들의 가시성 후손 개수를 사용합니다.
- 활성화하면 상위 항목을 자동으로 확장하고 `ensureVisibleRequested`를 통해 뷰포트 정렬을 요청할 수 있습니다.
- 사용자 상호 작용은 이미 활성화된 행에 대해 `activeChanged`를 다시 내보낼 수 있으므로 호스트 동작은 선택을 변경하지 않고도 고의적인 반복 탭/클릭에 작업을 바인딩할 수 있습니다.
- 생성된 행은 `activatableRole`(기본값 `activatable`, `selectable` 대체 경로 포함)를 통해 노드별 활성화 어포던스를 사용할 수 있으며, 활성화할 수 없는 행은 활성화 정규화 및 키보드 활성화 대상에서 제외됩니다.
- 생성된 행은 `draggableRole`(기본값 `draggable`, `dragAllowed` 대체 경로 포함)를 통해 노드별 드래그 어포던스를 사용할 수 있으므로 편집 가능한 목록은 드래그 시작에 대해 특정 노드를 잠그는 동안 선택한 행을 대화형으로 유지할 수 있습니다.
- `editable`는 객체 깊이 배열, 변경 가능한 QML `ListModel`/목록 유사 모델 및 변경 가능한 C++ `QAbstractItemModel` 소스를 지원합니다. 기본형 배열은 깊이 상태를 받을 행 개체가 없기 때문에 편집할 수 없습니다.
- `editable`는 목록 자체에 API 드래그를 노출하지 않습니다. 생성된 `HierarchyItem` 행에서만 항목 수준 끌어서 놓기 계약을 활성화합니다.
- 생성된 편집 가능한 행은 데스크탑 드래그를 즉시 유지하지만 모바일 대상 포인터 드래그는 `1000ms`를 길게 누른 후에만 시작되므로 터치 스크롤은 홀드 게이트에 도달할 때까지 주변 `Flickable`와 함께 유지됩니다.
- 모바일 대상 행 활성화는 누르기보다는 놓기/클릭 시 커밋되므로 목록 스크롤은 `activeItem`가 변경되기 전에 제스처를 요구할 수 있습니다.
- 깊이 재정렬 작업은 지원 소스를 업데이트합니다. 배열은 제자리에 다시 작성됩니다. `QAbstractItemModel` 소스는 깊이 및 상위 키 역할에 대해 `moveRows`와 함께 `setData`를 받습니다. QML `ListModel`/목록 유사 모델은 `move(...)`와 `setProperty(...)`를 수신합니다.

<a id="backend-model"></a>

## 백엔드 모델

`HierarchyModel` 는 이제 `HierarchyList` 뒤의 상태ful 모델 계산을 소유합니다: 소스 설명자, 상호작용 메타데이터, 가시성 투영, 편집 가능 모델 자격, 드래그 타겟 정규화, 설명자 재순서 및 모델 기반 편집을 위한 직접 소스 변형. `HierarchyList.qml` 는 라이브 `HierarchyItem` 객체를 수집하여 해당 객체에 투영된 메타데이터를 적용하고 행을 렌더링하며 사용자 입력을 전달하고, 해당 모델이 QML 의 동적 객체 경로만 노출될 때 QML `ListModel` 작성 방법을 호출합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.HierarchyList {
    model: [
        { key: "root", depth: 0, label: "Root", expanded: true, count: 2 },
        { key: "child", depth: 1, label: "Child", count: 7 }
    ]
}
```

```qml
import LVRS 1.0 as LV

LV.HierarchyList {
    model: backendHierarchyModel
    itemKeyRole: "key"
    labelRole: "name"
    depthRole: "depth"
    countRole: "counter"
    expandedRole: "expanded"
}
```

```qml
Component {
    id: customTreeRow

    LV.HierarchyItem {
        id: row
        countView: Component {
            LV.Label {
                text: row.count >= 0 ? row.count : ""
            }
        }
    }
}

LV.HierarchyList {
    model: [
        { key: "root", label: "Root", depth: 0, expanded: true },
        { key: "child", label: "Child", depth: 1 }
    ]
    itemDelegate: customTreeRow
}
```

<a id="shared-motion"></a>

## 공유 모션

행 상태 및 공개 동작은 로컬입니다. 드래그 형상과 모델 변경 사항은 그대로 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

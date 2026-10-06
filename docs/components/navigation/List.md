<a id="list"></a>

# 목록

위치: `src/qml/components/navigation/List.qml`

`List`는 직접 모델 또는 레거시 `items` 어레이에서 혼합 컴팩트 및 복합 행을 포함하여 모든 17 [ListItem 변형](ListItem.md)를 렌더링합니다. 미니 전용 목록에는 측정된 Figma `SmallList` 표면과 `ListFooter`가 유지됩니다.

<a id="purpose"></a>

## 목적

- 간단한 1열 탐색 행을 렌더링합니다.
- 호출자가 데이터를 하위 구성 요소로 변환하지 않고도 직접 모델 입력을 허용합니다.
- 기존 LVRS 탐색 화면과 호환되는 도구 모음/바닥글 어포던스를 유지하세요.

<a id="core-api"></a>

## 코어 API

모델 입력:

- `model`: 선호되는 직접 행 소스.
- `items`: `model`가 `null`/`undefined`인 경우에만 사용되는 레거시 행 소스입니다.
- `usingModel`(읽기 전용): `model`가 활성 소스인 경우 true입니다.
- `entryCount`(읽기 전용), `entryAt(index)`.
- `modelColumn`: `model`가 C++ `QAbstractItemModel`일 때 사용되는 열입니다.

역할 매핑:

- `labelRole`(기본값 `label`)
- `textRole`(기본값 `text`)
- `titleRole`(기본값 `title`)
- `iconRole`(기본값 `iconName`)
- `enabledRole`(기본값 `enabled`)
- `selectedRole`(기본값 `selected`)
- `typeRole`(기본값 `type`): ListItem 열거형 또는 정확한 Figma 유형 이름을 허용합니다. 레거시 `size`, 그 다음에는 `defaultItemType`(`Mini`)로 대체됩니다.
- `descriptionRole`(기본값 `description`)

레이아웃 및 상태:

- `selectedIndex`(기본값 `-1`), `interactive`
- `listWidth`(`170` 데스크톱), `minimumListHeight`(`140` 데스크톱), `itemHeight`(`22` 데스크톱), `itemLabelLeftPadding`(`4` 데스크톱)
- `defaultItemIconName`(기본값 `nodesfolder`)
- `expandToContent`(기본값 `false`): 실제 행 높이, 행 간격 및 도구 모음/바닥글의 합계로 커집니다.
- `scrollable`(기본값 `true`): 콘텐츠가 뷰포트를 초과하는 경우 세로 스크롤을 활성화합니다.
- `itemSpacing`(기본값 `0`); `itemHeight`는 고정 보폭이 아닌 행당 최소값입니다.
- `listWidth`는 기본적으로 가장 넓은 필수 사전 설정으로 설정됩니다. Mini의 경우 170, Detail의 경우 194, Navigation/Toggle/Checkable의 경우 280, 합성 유형의 경우 400입니다. 명시적 너비 재정의는 계속 지원됩니다.
- `backgroundColor`, `selectedRowColor`, `separatorColor`, `separatorOpacity`
- `itemDelegate`: 선택적 행별 구성 요소 위임. 기본값은 모델 유형과 함께 `ListItem`를 사용하고 `Mini`로 대체됩니다.

도구 모음/바닥글:

- `toolbarVisible`, `toolbarIcon1`, `toolbarIcon2`, `toolbarIcon3`
- `footerVisible`, `footerButton1`, `footerButton2`, `footerButton3`
- 기본 바닥글 슬롯은 `addFile`, `generaldelete` 및 `settings` 메뉴 버튼입니다.
- 스톡 바닥글 버튼은 설정 아이콘과 갈매기 모양 사이에 `Theme.iconSm` 아이콘 프레임(`18 x 18` 데스크탑, `36 x 36` 모바일), `Theme.gap2` 삽입 및 `ListFooter.stockMenuButtonSpacing`(`-2` 데스크탑, `-4` 모바일)을 사용합니다.

신호:

- `itemTriggered(index, item)`
- `itemEdited(index, item, field, value)`
- `itemActionTriggered(index, item, action, payload)`
- `toolbarIconTriggered(index, source)`
- `footerButtonTriggered(index, config)`

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 소스: `203:5161`(`SmallList`), `209:9199`(`ListFooter`) 및 `241:9253`(`ListItem` 구성 요소 세트).
- 데스크탑 `SmallList`는 `Theme.panelBackground03`가 포함된 `170 x 140`입니다. 해당 항목 뷰포트는 ​​`170 x 114`입니다. 6개의 `22px` 행이 `132px`를 차지하므로 마지막 행은 `26px` 바닥글 위에서 의도적으로 잘립니다.
- 기본 행은 `Mini` 변형입니다. 가로/세로 패딩 `4/2`, `nodesfolder` 아이콘 `18 x 18`, `1px` 아이콘-레이블 간격 및 본문 `13px Medium / 13px` 텍스트입니다. 데스크톱 및 모바일에서 본문은 `13px`에 고정되어 있습니다.
- `ListFooter`는 `86 x 26`입니다. 외부 패딩 `2`, 버튼 프레임 `22 x 22`, 슬롯 너비 `22`, `22` 및 `38`입니다. 정확한 기존 `addFile`, `generaldelete`, `settings` 및 `generalchevronDownBorderless` SVG 자산이 재사용됩니다.
- 데스크톱과 모바일은 동일한 형상을 공유합니다. 목록 `170 x 140`, 미니 행 `170 x 22`, 세부 정보 행 `194 x 106` 및 바닥글 `86 x 26`. 본문 텍스트는 `13px`로 유지됩니다.

<a id="behavior-contract"></a>

## 행동 계약

- `model`는 `items`보다 우선합니다.
- 지원되는 직접 모델 입력:
  - JavaScript 배열 및 기본 배열,
  - 객체 배열,
  - QML `ListModel`/`count` 및 `get(index)`가 있는 목록 유사 객체,
  - C++ `QAbstractItemModel` 인스턴스는 QML에 노출됩니다.
- 직접 모델 입력은 C++ `ModelSource` 유형을 통해 읽습니다.
- C++ 아이템 모델 변경(`rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged`, `dataChanged`)이 `ModelSource.revision`를 무효화합니다. `entryCount` 및 행 새로 고침.
- 기본 행은 `String(value)`로 렌더링되고 `defaultItemIconName`를 사용합니다. 해당 레이블은 아이콘 이름으로 해석되지 않습니다.
- 객체/모델 행은 `labelRole`, `textRole`, `titleRole`, `display`, `edit`의 레이블 텍스트를 확인합니다.
- `enabledRole` 및 `selectedRole`는 선택 사항입니다. `enabledRole`가 누락되면 활성화되었음을 의미합니다. `selectedRole`가 누락되어 `selectedIndex`로 대체됩니다.
- `itemTriggered`는 `entryAt(index)`에서 반환된 확인된 행 항목을 내보냅니다.
- 렌더링된 모든 행은 `itemDelegate`를 인스턴스화하고 하나의 `modelData` 개체를 주입합니다. 설명자에는 `index`, `entry`, `label`, `iconName`, `enabled`, `selected`, `type`, `description`, `properties`(원래 개체/역할 맵) 및 `trigger()`.
- 기본 위임은 행 항목에서 ListItem 콘텐츠, 상태, 구성 개체 및 가시성 속성을 전달합니다. 지원되는 이름은 [ListItem](ListItem.md)를 참조하세요.
- 모델 새로 고침은 기존 대리인의 `modelData`를 업데이트합니다. 단순히 텍스트나 선택 항목이 변경되었다는 이유만으로 편집기를 삭제/재생성하지 않습니다. `itemDelegate`를 변경하면 대리인이 교체됩니다.
- 사용자 변경 사항은 `itemEdited`를 내보냅니다. 애플리케이션은 모델 지속성을 소유합니다. 배열을 업데이트하거나, QML `ListModel.setProperty`를 호출하거나, 해당 핸들러에서 C++ 모델의 setter를 호출하세요. 임의의 애플리케이션 모델은 자동으로 변경되지 않습니다. 새로 고친 모델 데이터는 신뢰할 수 있는 상태로 유지됩니다.
- 기둥은 각 대리자의 실제 높이를 사용하여 대리자를 배치합니다. 복합 행은 후속 행과 겹치지 않습니다. Flickable은 바닥글이 스크롤 뷰포트 외부에 남아 있는 동안 오버플로를 노출합니다.
- 사용자 지정 대리자는 목록에서 정규화된 `itemTriggered` 신호를 내보내려는 경우 `property var modelData`를 선언하고 `modelData.trigger()`를 호출해야 합니다.

<a id="legacy-listitem-variants"></a>

## 레거시 ListItem 변형

- `size: ListItem.Mini`는 데스크탑의 `170 x 22`이며 기존 `inputable`, `inputResult`, `inputEdited` 및 `inputSubmitted` API를 유지합니다. 인라인 편집기는 데스크톱과 모바일에서 고정된 Body `13 / 13` 라인 상자를 사용하며 둘 모두에서 동일한 행 형상을 사용합니다.
- 데스크탑에서는 `size: ListItem.Detail`가 `194 x 106`입니다. `12px SemiBold / 12px` 제목 및 날짜 텍스트, `11px Regular / 11px` 메타데이터, `12/8` 가로/세로 패딩 및 `8px` 섹션 간격을 사용합니다.
- 세부 데이터는 `detail`, `dateText`, `folderLabel1`, `folderLabel2`, `tagLabel1` 및 `tagLabel2`를 통해 노출됩니다. 해당 아이콘 이름은 기본적으로 `bookmarksbookmark`, `folder@14x14` 및 `vcscurrentBranch`입니다.
- Figma 변형에는 구분 기호가 포함되어 있지 않으므로 `separatorVisible`의 기본값은 `false`입니다. 기존 구분 기호 색상/크기 속성은 명시적 선택에 계속 사용할 수 있습니다.

나머지 15 사전 설정과 해당 대화형 구성은 [ListItem](ListItem.md)에 문서화되어 있습니다. 시각적 카탈로그 목록 섹션은 모든 17 유형을 보여주고 해당 예제 모델에 대한 편집 내용을 다시 유지합니다.

<a id="usage"></a>

## 사용법

```qml
LV.List {
    id: mixedList
    expandToContent: true
    footerVisible: false
    items: [
        { type: "Navigation", label: "Library", value: "24" },
        { type: "Form", label: "Metadata", inputText1: "Project" }
    ]
    onItemEdited: function(index, item, field, value) {
        const next = items.slice()
        next[index] = Object.assign({}, item)
        next[index][field] = value
        items = next
    }
    onItemActionTriggered: function(index, item, action, payload) {
        console.log(index, action, payload)
    }
}
```

```qml
import LVRS 1.0 as LV

LV.List {
    model: [
        { label: "Overview", iconName: "nodesfolder" },
        { label: "Settings", enabled: false }
    ]
}
```

```qml
LV.ListItem {
    size: LV.ListItem.Detail
    detail: "Two-line note title"
    dateText: "2026-08-16"
}
```

```qml
Component {
    id: customRow

    LV.AbstractButton {
        property var modelData: ({})
        text: modelData.label || ""
        enabled: modelData.enabled === true
        onClicked: modelData.trigger()
    }
}

LV.List {
    model: [{ label: "Overview" }, { label: "Settings" }]
    itemDelegate: customRow
}
```

# TableHeader

위치: `src/qml/components/control/display/TableHeader.qml`

`TableHeader`는 `Table`의 머리글 행을 렌더링합니다.

<a id="purpose"></a>

## 목적

- 균일하거나 호출자가 제공한 열 너비로 열 레이블을 렌더링합니다.
- 행 본문과 별개로 구분 기호 스타일을 제공합니다.

## API

- `cellItems`(선호됨, TableCellItem와 유사한 항목의 배열/목록 모델)
- `columns`
- `rowHeight`
- `cellHorizontalPadding`
- `columnWidths`
- `fallbackCellWidth`
- `minColumnWidth`
- `textColor`
- `separatorHeight`
- `separatorColor`
- `cellDelegate`: 선택적 헤더별 셀 구성 요소 위임. 기본 대리자는 기존 텍스트 레이블을 렌더링합니다.
- `interactive`(기본값 `false`)
- `sortColumn`(기본값 `-1`), `sortOrder`

신호:

- `columnClicked(columnIndex, columnData)`

도우미 방법:

- `columnAt(index)`
- `columnText(index)`
- `columnType(index)`
- `normalizeColumnType(value)`
- `inferredColumnType(value)`
- `columnPadding(index)`
- `columnWidth(index)`
- `columnX(index)`

계산됨:

- `resolvedColumnSource`
- `resolvedColumnCount`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    columnWidths: [160, 80, 120]
    cellItems: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" },
        { label: "Enabled", type: "bool" }
    ]
}
```

<a id="how-it-works"></a>

## 동작 원리

- `TableHeader`는 소스 해상도, 입력 및 기하학을 C++ `TableHeaderModel`에 위임합니다.
- `cellItems`가 기본 계약입니다. 레거시 `columns`는 대체 경로로 유지됩니다.
- 열 텍스트는 기본 항목 또는 개체 항목(`label/text/title/value` 대체 경로)을 허용합니다.
- 열 유형은 객체 키 `type`, `valueType`, `cellType` 또는 `dataType`를 허용합니다.
- 기본 헤더 항목은 유형을 추론합니다: `string -> string`, 정수 -> `int`, 정수가 아닌 숫자 -> `float`, 부울 -> `bool`.
- 선택적 열별 `contentSpacing`/`horizontalPadding` 재정의가 지원됩니다.
- 리피터 대리자는 제공된 경우 `columnWidths`를 사용하고, 그렇지 않으면 `fallbackCellWidth`를 사용하고, 그렇지 않으면 동일한 자동 너비를 사용합니다.
- 렌더링된 모든 헤더 셀은 `cellDelegate`를 인스턴스화하고 하나의 `modelData` 개체를 주입합니다. 설명자에는 `index`, `descriptor`, `cellData`, `text`, `valueType`, `x`, `width`, `height`가 포함됩니다. `padding`.
- `cellData` 및 `columnData`에는 원래 열 항목이 포함되어 있습니다. `sorted` 및 `sortOrder`를 사용하면 사용자 정의 대리자가 자체 정렬 표시기를 그릴 수 있습니다.
- 사용자 지정 대리인은 `property var modelData`를 선언해야 합니다. 해당 루트 항목의 크기는 확인된 헤더 셀 경계에 맞게 조정됩니다.
- 하단 구분 기호는 전용 직사각형으로 렌더링됩니다(`panelBackground10` 기본값).

<a id="backend-model"></a>

## 백엔드 모델

`TableHeaderModel`는 `resolvedColumnSource`, 열 설명자, 유형 정규화, 유형 추론, 패딩, 열 너비 및 x 오프셋 계산을 소유합니다. `TableHeader.qml`는 모델 설명자를 렌더링하고 레거시 도우미 메서드를 통과 API로 유지합니다.

<a id="typed-header-contract"></a>

## 형식화된 헤더 계약

`columnType(index)`는 `string`, `int`, `float` 또는 `bool` 중 하나를 반환합니다. 의도적으로 헤더 전용 메타데이터입니다. `Table`는 본문 셀 편집 내용을 검증하기 위해 동일한 계약을 사용합니다.

<a id="practical-tip"></a>

## 실용적인 팁

조밀한 12px 세미 볼드 텍스트는 컴팩트 메타데이터에 최적화되어 있으므로 헤더 레이블을 짧고 의미 있는 방식(필드 유형/범주)으로 유지하세요.

<a id="extended-example-object-based-header-definition"></a>

## 확장된 예: 객체 기반 헤더 정의

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    cellItems: [
        { label: "Service", type: "string" },
        { text: "State", type: "bool", contentSpacing: LV.Theme.gap8 },
        { title: "Latency", type: "float" }
    ]
}
```

<a id="delegate-example"></a>

## 대리인 예

```qml
Component {
    id: compactHeaderCell

    Item {
        property var modelData: ({})

        LV.Label {
            anchors.left: parent.left
            anchors.leftMargin: modelData.padding || 0
            anchors.verticalCenter: parent.verticalCenter
            text: modelData.text
        }
    }
}

LV.TableHeader {
    interactive: true
    cellItems: [{ label: "Name" }, { label: "Count", type: "int" }]
    cellDelegate: compactHeaderCell
    onColumnClicked: function(columnIndex, columnData) {
        console.log("sort", columnIndex, columnData.label)
    }
}
```

## FAQ

Q. `TableHeader`에서 열별 정렬을 직접 정의할 수 있나요?   A. 현재 계약은 균일한 왼쪽 정렬 헤더 텍스트를 유지합니다. 열별 레이아웃에는 사용자 정의 파생 구성 요소가 필요합니다.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 헤더 개수가 예상 열 개수와 일치합니다.
- `columnType(index)`는 선언되거나 추론된 필드 유형과 일치합니다.
- 분리막 두께/색상은 테마 계약을 준수하며,
- 객체 기반 열 레이블은 대체 경로 키를 통해 확인됩니다.

<a id="figma-contract"></a>

## Figma 계약

노드 `203:3647` 는 `717 × 25` 입니다: `24` 헤더 행과 `1` 하단 구분선입니다. 3 기본 열은 `239` 넓이이며, 각 레이블은 `8` 가로 패딩을 가지며, 텍스트는 12/12 설명 스타일을 사용합니다. 모바일은 동일한 `717 × 25` 기하학, `8` 패딩, `1` 구분선, 및 `12px` 설명 타이포그래피를 사용합니다.

<a id="shared-motion"></a>

## 공유 모션

헤더 셀은 해당 대리자로부터 텍스트 입력 포커스와 선택 피드백을 상속합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

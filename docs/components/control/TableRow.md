# TableRow

위치: `src/qml/components/control/display/TableRow.qml`

`TableRow`는 `TableCellItem` 대리자를 반복하여 하나의 데이터 행을 렌더링합니다.

<a id="purpose"></a>

## 목적

- 행 입력을 시각적 셀 대리자로 변환합니다.
- 컴팩트한 Figma 스타일 행 형상을 유지합니다.

## API

- `cellItems`(선호됨, TableCellItem와 유사한 항목의 배열/목록 모델)
- `cells`
- `cellWidth`
- `cellHeight`
- `contentSpacing`
- `dividerColor`
- `textColor`
- `inputable`(기본값 `false`, 셀의 행 수준 편집 가능 기본값)

신호:

- `cellInputEdited(columnIndex, text)`
- `cellInputSubmitted(columnIndex, text)`

도우미 방법:

- `cellAt(index)`
- `cellText(index)`
- `cellInputable(index)`

계산됨:

- `resolvedCellSource`
- `resolvedCellCount`
- `resolvedSpacing`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cellItems: [
        { text: "A" },
        { text: "B" },
        { text: "C" }
    ]
}
```

<a id="how-it-works"></a>

## 동작 원리

- `cellItems`가 기본 계약입니다. 레거시 `cells`는 대체 경로로 유지됩니다.
- 각 항목은 `TableCellItem.itemData`로 전달됩니다.
- 셀 텍스트는 기본 값 또는 객체 대체 경로 키(`label/text/title`)에서 확인됩니다.
- 셀 입력 가능성은 먼저 `entry.inputable` 재정의로 해결된 다음 행 수준 `inputable`로 해결됩니다.
- 행 간격은 사용 가능한 너비와 고정 셀 너비로 계산됩니다.
- 간격은 절대 음수가 되지 않습니다(`Math.max(0, computed)`).
- 기본 선행 구분자는 `panelBackground10`를 사용하여 측정된 행 내보내기를 일치시킵니다.

<a id="practical-tip"></a>

## 실용적인 팁

정확한 Figma 일치를 위해 `cellWidth: 234`, `cellHeight: 24` 및 행 너비 `717`를 유지합니다.

<a id="extended-example-mixed-primitiveobject-cells"></a>

## 확장된 예: 혼합 기본/객체 셀

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cellItems: [
        "Renderer",
        { text: "Active", dividerColor: LV.Theme.panelBackground10 },
        { title: "Core", textColor: LV.Theme.bodyColor }
    ]
}
```

## FAQ

Q. `cells`는 여전히 작동합니까?   A. 네. `cells`는 호환성을 위해 유지되지만 `cellItems`가 기본 API입니다.

<a id="figma-contract"></a>

## Figma 계약

노드 `203:3648` 는 `717 × 24` 입니다: 3 `234 × 24` 셀로 `7.5` 간격으로 분리됩니다. 바디 텍스트는 13/13로 고정됩니다. 모바일은 동일한 `717 × 24` 행, `234` 셀 너비, 및 `8` 콘텐츠 간격을 사용하며, 바디는 `13px` 로 유지됩니다.

<a id="shared-motion"></a>

## 공유 모션

각 셀은 자체 포커스와 상태 애니메이션을 처리합니다. 행은 열 기하학을 유지합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

# TableCellItem

위치: `src/qml/components/control/display/TableCellItem.qml`

`TableCellItem`는 가장 작은 테이블 기본 형식(선행 구분선 + 단일 텍스트 본문)입니다.

<a id="purpose"></a>

## 목적

- Figma 행 기하학과 일치하는 하나의 컴팩트 테이블 셀을 렌더링합니다.
- 테이블 셀 데이터 개체에서 직접 텍스트 및 시각적 재정의를 해결합니다.

## API

- `itemData`(객체와 유사한 셀 설명자, 선택 사항)
- `text`
- `cellHeight`
- `contentSpacing`
- `dividerColor`
- `textColor`
- `showDivider`
- `clipContent`
- `textStyle`(레이블 스타일 열거형 값)
- `inputable`(기본값 `false`, 텍스트 경계에서 인라인 입력 오버레이 전환)
- `selected`, `current`
- `selectionColor`, `currentBorderColor`, `currentBorderWidth`
- `inputResult`(최신 편집 가능한 문자열 값)
- `valueType` (`string`, `int`, `float`, `bool`; 주입된 검증인에 의해 사용됨)
- `valueValidator`(상위 테이블에 의해 주입된 선택적 함수)
- `typedValue`(최신에 허용된 입력 값)
- `inputAccepted` (마지막 검증 결과)

입력 이벤트:

- `inputEdited(text)`
- `inputSubmitted(text)`
- `inputRejected(text, valueType)`
- `applyInputResult(value)`는 정규화된 `string`를 반환합니다.

해결된 읽기 전용 값:

- `resolvedText`
- `resolvedCellHeight`
- `resolvedContentSpacing`
- `resolvedDividerColor`
- `resolvedTextColor`
- `resolvedShowDivider`
- `resolvedClipContent`
- `resolvedTextStyle`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    itemData: ({
        text: "Renderer",
        dividerColor: LV.Theme.panelBackground03
    })
}
```

<a id="how-it-works"></a>

## 동작 원리

- `itemData`는 `label`, `text`, `title`와 같은 객체 키를 지원합니다.
- 레이블/텍스트/제목 키가 없는 경우에도 `itemData.value`가 허용됩니다.
- 객체 키가 누락된 경우 구성 요소 수준 대체 경로 소품이 사용됩니다.
- 구분선과 텍스트 렌더링은 조밀하게 유지됩니다(`24px` 셀 높이, 줄임표 텍스트).
- `selected`는 셀 선택 레이어를 채웁니다. `current`는 현재 셀 테두리를 그립니다. `Table`는 두 상태를 기본 대리자에게 자동으로 제공합니다.
- `inputable`가 활성화되면 `InputField`는 텍스트 영역을 오버레이하고 원래 라벨 슬롯에 맞춰 정렬된 형상을 유지합니다. 내장된 라인 박스와 네이티브 입력은 데스크톱과 모바일의 Body `13 / 13`에 고정되어 이전에 잘린 `16 / 32` 라인 박스를 피합니다.
- `valueValidator`는 부모가 주입한 API입니다. `true/false` 또는 `{ accepted, text, value }`를 반환할 수 있습니다. 거부된 편집 내용은 `inputResult`를 변경하지 않고 그대로 두고 `inputRejected`를 내보냅니다.

<a id="practical-tip"></a>

## 실용적인 팁

모델 기반 테이블에는 `itemData`를 사용하고 static/one-off 케이스에는 기본 `text` 할당을 유지합니다.

<a id="extended-example-header-like-cell-styling"></a>

## 확장된 예: 헤더와 같은 셀 스타일 지정

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    itemData: ({ label: "Column" })
    showDivider: false
    textStyle: description
    textColor: LV.Theme.descriptionColor
}
```

<a id="extended-example-typed-edit-guard"></a>

## 확장된 예: 입력된 편집 가드

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    valueType: "int"
    valueValidator: function(value) {
        const text = String(value).trim()
        if (!/^-?\d+$/.test(text))
            return { accepted: false, text: text, value: value }
        const intValue = Number(text)
        return { accepted: true, text: String(intValue), value: intValue }
    }
}
```

## FAQ

Q. `itemData`에 여러 레이블 필드가 포함된 경우 어떤 텍스트 키가 사용됩니까?   A. 해결 순서는 `label -> text -> title -> value`이며 구성 요소 `text`로 대체됩니다.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 구분선 가시성이 설계 요구 사항과 일치합니다.
- 텍스트 제거는 좁은 너비에서 작동합니다.
- 컬러 토큰 매치 테이블 디자인(`panelBackground03` 독립형 구분선, 본문 텍스트).

<a id="figma-contract"></a>

## Figma 계약

노드 `203:3863`는 `234 × 24`입니다. 주요 분배기는 `1 × 24`입니다. 콘텐츠 라인 상자는 고정된 13/13 본문 타이포그래피를 사용하는 `(x: 9, y: 5.5, width: 225, height: 13)`입니다. 모바일은 동일한 `234 × 24` 기하학, `8` 콘텐츠 간격 및 `13px` 본문 글꼴을 사용합니다.

<a id="shared-motion"></a>

## 공유 모션

선택 채우기가 해제되고 편집기에 탄력적인 포커스 피드백이 표시됩니다. 검증은 즉시 이루어집니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

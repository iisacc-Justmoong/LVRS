<a id="hstack"></a>

# H스택

위치: `src/qml/components/layout/HStack.qml`

`HStack`는 `RowLayout`로 구현된 SwiftUI 스타일 수평 스택입니다.

<a id="purpose"></a>

## 목적

- 선택적 이름 기반 정렬 의미 체계로 간단한 축 레이아웃을 제공합니다.
- 공백 동작에 대해 관리되는 하위 항목에 자동 주석을 답니다(`stackAxis`).

## API

- `spacing`(`-1`는 기본 간격을 의미함)
- `defaultSpacing`
- `alignment`
- `alignmentName` (`top | center | bottom`)
- 기본 `content` 슬롯

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.HStack {
    alignmentName: "bottom"
    spacing: 12
    LV.LabelButton { text: "Cancel" }
    LV.LabelButton { text: "Save"; tone: LV.AbstractButton.Primary }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 이름 기반 정렬은 원시 `alignment` 플래그보다 우선순위가 높습니다.
- 정렬 업데이트는 레이아웃 정렬이 변경되지 않은 관리되는 하위 항목에만 적용됩니다.
- `stackAxis`를 노출하는 어린이는 `Spacer`와 협력하기 위해 `"horizontal"`를 받습니다.

<a id="advanced-example-mixed-managedcustom-alignment"></a>

## 고급 예: 혼합 관리형/사용자 지정 정렬

```qml
import QtQuick
import LVRS 1.0 as LV

LV.HStack {
    alignmentName: "center"
    Rectangle { width: 32; height: 32 }
    Rectangle {
        width: 32; height: 48
        Layout.alignment: Qt.AlignTop
    }
}
```

이 경우 명시적으로 할당된 하위 정렬은 유지되고 자동 관리되는 하위 항목은 스택 정렬을 따릅니다.

## FAQ

Q. 한 아이는 왜 `alignmentName`를 무시하나요?   A. 하위 항목에는 명시적인 `Layout.alignment` 세트가 있을 수 있으며 더 이상 스택에 의해 자동 관리되지 않습니다.

Q. 음수 띄어쓰기가 허용되나요?   A. 계약은 모든 정수를 허용하지만 음수 공백은 중복을 유발하므로 의도적으로 사용해야 합니다.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 정렬 토큰은 예상되는 수직 배치로 확인됩니다.
- 간격 계약은 응답 중단점 전체에서 일관됩니다.
- 명시적/자동 하위 정렬이 혼합되어 의도한 대로 작동합니다.

<a id="shared-motion"></a>

## 공유 모션

RowLayout 간격은 수직 스택과 동일한 타이밍으로 고정됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

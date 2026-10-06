<a id="vstack"></a>

# V스택

위치: `src/qml/components/layout/VStack.qml`

`VStack`는 `ColumnLayout`로 구현된 SwiftUI 스타일 수직 스택입니다.

<a id="purpose"></a>

## 목적

- 정렬 이름 약칭을 사용하여 예측 가능한 수직 구성을 제공합니다.
- 스택 인식 하위 항목에 축 메타데이터를 전파합니다.

## API

- `spacing`(`-1`는 기본 간격을 의미함)
- `defaultSpacing`
- `alignment`
- `alignmentName` (`leading | center | trailing`)
- 기본 `content` 슬롯

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.VStack {
    alignmentName: "leading"
    spacing: 8
    LV.Label { text: "Title"; style: title2 }
    LV.Label { text: "Description"; style: description }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 이름 토큰은 가로 레이아웃 정렬 플래그에 매핑됩니다.
- 관리되는 하위 맞춤 업데이트는 하위 맞춤이 자동 관리되는 경우에만 적용됩니다.
- `stackAxis`를 노출하는 어린이는 `Spacer` 협력에 대한 대가로 `"vertical"`를 받습니다.

<a id="advanced-example-trailing-alignment-with-spacer"></a>

## 고급 예: 스페이서를 사용한 후행 정렬

```qml
import LVRS 1.0 as LV

LV.VStack {
    alignmentName: "trailing"
    LV.Label { text: "Latency"; style: body }
    LV.Spacer { minLength: 16 }
    LV.Label { text: "12 ms"; style: header2 }
}
```

<a id="practical-tip"></a>

## 실용적인 팁

디자인 토큰 중심의 수직 리듬을 원할 경우 `spacing = -1`와 `defaultSpacing`를 사용하세요.

## FAQ

Q. `alignmentName`가 `alignment`보다 우선합니까?   A. 네. 명명된 정렬 토큰이 있는 경우 우선순위를 갖습니다.

Q. `Spacer`가 예상대로 푸시되지 않는 이유는 무엇입니까?   A. 하위가 별도로 고정된 하위 트리가 아닌 스택 관리 레이아웃 트리 내부에 있는지 확인하세요.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 정렬 토큰은 예상되는 수평 배치로 확인됩니다.
- 간격 대체 경로는 `spacing == -1`일 때 작동합니다.
- 스택 축 전파는 스페이서 확장을 가능하게 합니다.

<a id="shared-motion"></a>

## 공유 모션

ColumnLayout 간격은 일반적인 리바운드로 해결됩니다. 하위 크기는 레이아웃 소유로 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

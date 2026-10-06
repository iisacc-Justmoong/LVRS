<a id="zstack"></a>

# Z스택

위치: `src/qml/components/layout/ZStack.qml`

`ZStack`는 2차원 앵커 공간에서 하위 항목을 정렬하는 SwiftUI 스타일 오버레이 스택입니다.

<a id="purpose"></a>

## 목적

- 단일 정렬 계약으로 여러 하위 항목을 오버레이합니다.
- 이미 외부 앵커로 관리되는 하위 항목에 대한 우발적인 재정의를 방지하세요.

## API

- `alignment`
- `alignmentName`
- 기본 `content` 슬롯

지원되는 `alignmentName` 토큰:

- `topLeading`, `top`, `topTrailing`
- `leading`, `center`, `trailing`
- `bottomLeading`, `bottom`, `bottomTrailing`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.ZStack {
    alignmentName: "topTrailing"
    Rectangle { anchors.fill: parent }
    Rectangle { width: 14; height: 14; radius: 7 }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 관리되지 않는 하위 항목의 경우 기존 앵커가 지워지고 확인된 정렬에서 다시 작성됩니다.
- 내부적으로 관리되는 것으로 이미 태그가 지정되지 않은 한 외부 앵커가 있는 하위 요소는 건너뜁니다.
- 동적 삽입 후 정렬을 일관되게 유지하기 위해 하위 업데이트가 `childrenChanged`에서 실행됩니다.

<a id="advanced-example-explicit-anchor-preservation"></a>

## 고급 예: 명시적 앵커 보존

명시적인 외부 앵커가 있는 하위 항목은 이미 내부적으로 관리되지 않는 한 자동 정렬 논리에 의해 의도적으로 건너뜁니다. 이는 합성 오버레이에서 실수로 앵커가 재설정되는 것을 방지합니다.

<a id="troubleshooting"></a>

## 문제 해결

아이가 잘못 정렬된 것처럼 보이는 경우:

1. 수동 앵커가 있는지 확인하고,
2. `alignmentName` 토큰 정확성을 확인하고,
3. 하위 항목이 스택 콘텐츠 레이어 외부에서 다시 부모로 지정되지 않았는지 확인하세요.

## FAQ

Q. 수동 앵커가 재설정되는 이유는 무엇입니까?   A. `ZStack`의 자동 관리 하위 항목에는 정렬 계약으로 다시 작성된 앵커가 있습니다.

Q. 수동 앵커 소유권을 유지하는 방법은 무엇입니까?   A. 스택 자동 관리를 적용하기 전에 하위 항목에 대한 명시적 앵커를 유지하거나 전용 래퍼 항목을 사용하십시오.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 하위 정렬은 `alignmentName` 토큰 기대치를 따릅니다.
- 외부에 고정된 어린이는 의도한 대로 영향을 받지 않습니다.
- 동적 하위 삽입은 예상되는 스택 순서를 유지합니다.

<a id="shared-motion"></a>

## 공유 모션

정렬이 앵커에 의해 계속 관리되는 동안 레이어 불투명도가 해소됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

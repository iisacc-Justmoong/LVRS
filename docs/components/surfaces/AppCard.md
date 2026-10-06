# AppCard

위치: `src/qml/components/surfaces/AppCard.qml`

`AppCard`는 헤더, 구분 기호 및 유연한 콘텐츠 슬롯이 포함된 재사용 가능한 제목 표면입니다.

<a id="purpose"></a>

## 목적

- 양식, 요약, 대시보드 모듈에 일관된 카드 컨테이너를 제공합니다.
- 헤더/콘텐츠 간격 및 반경 정책을 중앙 집중화합니다.

<a id="core-api"></a>

## 코어 API

헤더:

- `title`
- `subtitle`

모양과 간격:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`, `resolvedCornerRadius`
- `cardPadding`(읽기 전용)
- `sectionSpacing`(읽기 전용)

내용:

- 기본 `content` 슬롯(`contentSlot.data`)

<a id="behavior-contract"></a>

## 행동 계약

- 암시적 너비/높이는 헤더/콘텐츠 크기 + 카드 패딩으로 계산됩니다.
- `subtitle`가 비어 있으면 자막 행이 숨겨집니다.
- 콘텐츠 영역 높이는 슬롯 하위 `childrenRect`에 의해 결정됩니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.AppCard {
    title: "System Health"
    subtitle: "Last 15 minutes"

    LV.Label { text: "No incidents" }
}
```

<a id="shared-motion"></a>

## 공유 모션

카드 색상은 각 어린이가 상호 작용을 소유하는 동안 혼합됩니다. 이 컨테이너에는 암시적인 클릭 동작이 없습니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

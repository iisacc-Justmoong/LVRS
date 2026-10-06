# AppHeader

위치: `src/qml/components/layout/AppHeader.qml`

`AppHeader`는 옵션 메뉴 트리거, 제목/부제 블록 및 후행 작업 슬롯이 있는 상단 도구 모음 구성 요소입니다.

<a id="purpose"></a>

## 목적

- 일관된 간격/글꼴로 재사용 가능한 페이지 헤더를 제공하세요.
- 패딩과 간격을 줄여 컴팩트 모드를 지원합니다.

## API

텍스트 및 동작:

- `title`
- `subtitle`
- `menuVisible`
- `menuClicked()` 신호

밀도/레이아웃 제어:

- `compact`
- `contentHorizontalPadding`
- `contentVerticalPadding`
- `rowSpacing`
- `actionSpacing`
- `menuButtonPadding`

슬롯:

- 기본 `actions` 슬롯(`actionRow.data`)

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.AppHeader {
    title: "Runs"
    subtitle: "Production"
    menuVisible: true
    onMenuClicked: openDrawer()

    LV.IconButton { iconName: "add" }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 암시적 높이는 행 암시적 높이와 최소 헤더 정책을 통해 계산됩니다.
- 메뉴 버튼은 `menuVisible == true`인 경우에만 나타납니다.
- 제목 블록은 채우기 너비를 사용합니다. 후행 작업 행이 오른쪽으로 정렬됩니다.

<a id="extended-example-compact-header-variant"></a>

## 확장된 예: 컴팩트 헤더 변형

```qml
import LVRS 1.0 as LV

LV.AppHeader {
    compact: true
    title: "Inspector"
    subtitle: "Selection Details"

    LV.LabelButton {
        text: "Apply"
        tone: LV.AbstractButton.Primary
    }
}
```

<a id="implementation-notes"></a>

## 구현 노트

- 컴팩트 모드는 별도의 스타일 사전 설정을 전환하는 대신 내부 간격 값을 조정합니다.
- 작업 슬롯은 기본 속성 별칭이므로 하위 컨트롤을 구성 요소 내에서 직접 선언할 수 있습니다.

<a id="shared-motion"></a>

## 공유 모션

각 헤더 작업은 AbstractButton 기반을 통해 압축된 후 리바운드됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

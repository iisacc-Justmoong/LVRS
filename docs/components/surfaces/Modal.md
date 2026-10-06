<a id="modal"></a>

# 모달

위치: `src/qml/components/surfaces/Modal.qml`

`Modal`는 왼쪽 아이콘, 제목/설명 및 최대 3개의 작업 버튼이 있는 Apple 스타일의 대화 상자 표면입니다.

<a id="purpose"></a>

## 목적

- 아이콘 + 텍스트 + 작업 컨트롤이 포함된 중앙 상단 이동 모달 프레임을 제공합니다.
- 모달이 열려 있는 동안 흐리게 오버레이를 표시합니다.
- 사용자가 모달 프레임 외부를 클릭하면 취소됩니다.

<a id="core-api"></a>

## 코어 API

상태 및 동작:

- `open`
- `dismissOnBackground`
- `useOverlayLayer`
- `cancel()`
- `handleBackdropClick(localX, localY)`
- `containsFramePoint(localX, localY)`
- `actionVisible(index)`
- `triggerAction(index)`

크기 및 레이아웃:

- `minWidth`, `maxWidth`, `preferredWidth`
- `sidePadding`
- `frameMinHeight`
- `verticalOffset`(기본 상단 이동)

내용:

- `title`
- `description`, `message`, `desc`(`description`의 별칭)
- `iconName`, `iconSource`, `showIcon`
- `iconSize`, `iconCornerRadius`
- `contentComponent`: 열려 있는 동안 헤더와 작업 사이에 로드되는 선택적 `Component`입니다.
- `contentItem`: 읽기 전용 로드된 항목 또는 닫혀 있는 동안 `null`.
- 해결됨: `resolvedIconSource`, `resolvedDescription`

작업:

- `buttonCount` (`0=auto`, `1~3=explicit`)
- `primaryText`, `secondaryText`, `tertiaryText`
- `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`
- 해결됨: `resolvedButtonCount`, `hasPrimaryAction`, `hasSecondaryAction`, `hasTertiaryAction`

프레임 스타일:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `frameCornerRadius`, `resolvedFrameCornerRadius`
- `frameColor`
- `backdropColor`

신호:

- `canceled()`
- `primaryClicked()`
- `secondaryClicked()`
- `tertiaryClicked()`

<a id="behavior-contract"></a>

## 행동 계약

- `open`가 `true`이면 `Modal`가 표시되고 활성화됩니다.
- 프레임은 수평 중앙에 위치하며 수직 중앙보다 약간 위에 있습니다.
- 배경화면 외부 클릭은 `dismissOnBackground == true`일 때 `cancel()`를 호출합니다.
- 프레임 내부의 배경화면 클릭은 취소되지 않습니다.
- 열리면 구성요소가 사용 가능한 경우 `Controls.Overlay.overlay`로 다시 상위화될 수 있습니다.
- 제공된 컨텐츠 뷰는 프레임의 컨텐츠 너비를 수신하고 해당 `implicitHeight`에 기여합니다. 닫으면 언로드됩니다. 다시 열면 새로운 인스턴스가 생성됩니다. 구성 요소가 제공되지 않으면 기본 아이콘/텍스트/작업 레이아웃이 변경되지 않습니다.
- 자체 작업이 있는 보기의 경우 `showIcon: false` 및 `primaryText: ""`를 설정합니다. 전체 호스트 예는 [ColorPicker](../control/ColorPicker.md)를 참조하세요.
- `buttonCount`는 최대 3로 고정됩니다.
- 자동 모드(`buttonCount=0`)에서 3차 텍스트는 3 작업을 활성화하고, 보조 텍스트는 2작업을 활성화하며, 그렇지 않으면 기본만 활성화합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Modal {
    open: true
    iconName: "projectStructure"
    title: "Unlock iPhone 15 Pro Max to Continue"
    description: "Xcode cannot launch because the device is locked."
    buttonCount: 2
    primaryText: "Cancel Running"
    secondaryText: "Later"
}
```

<a id="shared-motion"></a>

## 공유 모션

프레임과 배경이 함께 들어갑니다. 닫는 프레임은 페이딩되는 동안 계속 표시됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

# AbstractButton

위치: `src/qml/components/control/buttons/AbstractButton.qml`

`AbstractButton`는 LVRS 버튼 제품군 구성 요소의 공유 베이스입니다.

구체적인 버튼 계열은 Figma 버튼 페이지를 따르며, 레이블/아이콘 PushButton 세트와 DropdownButton 세트 ( 레이블/아이콘 메뉴 트리거용) 를 사용합니다. 각 완성된 컴포넌트는 자체 입력 상태 변형을 가집니다. 현재 노드 ID 와 런타임 런타임 매핑에 대한 내용은 [컴포넌트 인스턴스 상태](../../instance-states.md) 를 참조하세요.

<a id="purpose"></a>

## 목적

- 톤 기반 색상 정책을 통합합니다(`Primary`, `Default`, `Borderless`, `Destructive`, `Disabled`).
- 상호 작용 게이팅(`effectiveEnabled`)을 중앙 집중화하고 동작에 집중합니다.
- 공유 패딩, 반경 정책 및 암시적 크기 기준을 제공합니다.

<a id="core-api"></a>

## 코어 API

톤과 모양:

- `tone` (`AbstractButton.ButtonTone`)
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`
- `resolvedCornerRadius`(읽기 전용)

상호작용:

- `effectiveEnabled`(읽기 전용, `enabled && tone !== Disabled`)
- `hoverEnabled`/`focusPolicy`는 `effectiveEnabled`에서 파생됩니다.
- `releaseOnSignal`: 기본적으로 false입니다. PushButton / DropdownButton는 명시적인 짧은 릴리스 리바운드를 활성화합니다.
- `interaction`: 구성 요소 인스턴스당 하나의 소유된 넌비주얼 `InteractionState`
- `interactionPhase` / `interactionInput`(읽기 전용): 현재 입력 위상 및 포인터/키보드 양식
- `interaction.focusVisible`: 누르기/놓기와 관계없이 활성화된 키보드 포커스

주입된 방법:

- `method`: 하나의 호출 가능 항목이 버튼에 직접 삽입됩니다.
- `methods`: 콜러블 또는 명령 객체의 배열
- `hasInjectedMethods`(읽기 전용)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

색상:

- `textColor`, `textColorDisabled`
- `backgroundColor`, `backgroundColorHover`, `backgroundColorPressed`, `backgroundColorDisabled`
- 톤 파생 읽기 전용 색상: `toneTextColor`, `toneBackgroundColor*`

Figma 종류 매핑:

- `accent` -> `AbstractButton.Primary` / `Theme.primary`
- `default` -> `AbstractButton.Default` / `Theme.panelBackground12`
- `borderless` -> `AbstractButton.Borderless` / 투명한 배경 및 `Theme.primary` 텍스트 또는 표시기
- `destructive` -> `AbstractButton.Destructive` / `Theme.danger`
- `disabled` -> `AbstractButton.Disabled` / `Theme.panelBackground04` 및 `Theme.disabledColor`

`PushButton` 및 `DropdownButton` 제품군과 4개의 명명된 사전 설정은 기본적으로 `Primary`로 설정되어 구성 요소 세트의 기본 `Kind=accent`와 일치합니다. `AbstractButton` 자체는 중립 공유 베이스로 유지되며 자체 `Default` 톤을 유지합니다.

레이아웃:

- `horizontalPadding`, `verticalPadding`
- 콘텐츠 + 패딩의 `implicitHeight`/`implicitWidth`

<a id="behavior-contract"></a>

## 행동 계약

- `tone: Disabled`는 `enabled: true`인 경우에도 상호 작용을 비활성화합니다.
- `Borderless` 톤은 투명한 기본 채우기를 유지하고 표면 호버/눌린 색상을 사용합니다.
- 클릭연결을 방지하기 위해 `effectiveEnabled == false` 시 차단 `MouseArea`가 설치됩니다.
- 포커스가 있는 동안 비활성화되면 구성요소가 포커스를 지웁니다.
- `clicked()`에서는 주입된 `method`와 `methods`가 순서대로 실행됩니다. `method`는 `methods`의 항목보다 먼저 실행됩니다.
- `methods` 항목은 JavaScript 함수이거나 `invoke(eventData)`/`trigger(eventData)`를 노출하는 개체일 수 있습니다.
- 수동 명령 파견을 위해 `invokeMethods()`를 직접 호출할 수도 있습니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.AbstractButton {
    text: "Action"
    tone: LV.AbstractButton.Primary
    method: function(eventData) {
        saveModel(eventData.source)
    }
    methods: [
        function(eventData) { audit("clicked", eventData.trigger) }
    ]
}
```

<a id="button-families"></a>

## 버튼 계열

`PushButton` 와 `DropdownButton` 는 `AbstractButton` 의 독립적인 자식입니다. 전자는 평범한 레이블/아이콘 액션을 소유하고, 후자는 꼬리 화살표가 있는 레이블/아이콘 메뉴 트리거를 소유합니다. 그들의 컴팩트한 8px 반경과 측정된 패딩/간격은 해당 계열에 범위가 지정되므로, `AlertButton` , `Stepper` , 및 기타 직접적인 `AbstractButton` 소비자는 자신의 계약을 유지합니다.

<a id="shared-motion"></a>

## 공유 모션

누르기 변형은 탭 탐색을 위한 포커스 링과 함께 모든 버튼 하위 클래스에서 공유됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

`showFocusRing`의 기본값은 true입니다. Figma가 작성한 포커스 테두리가 있는 구성 요소는 키보드 포커스와 공유 동작을 유지하면서 이를 false로 설정하고 해당 테두리를 렌더링할 수 있습니다(ColorPickerButton가 이를 수행함).

`focusRingOutset` 은 기본값으로 `0` 이며, `focusRingRadius` 는 기본값으로 `resolvedCornerRadius` 입니다. 작성된 PushButton / DropdownButton 와 메뉴/목록/계층 구조 행 계열은 3px 바깥쪽을 사용하며, 콤팩트 버튼은 링 반지름을 11px로 증가시킵니다. 이는 초점 윤곽선을 시각적 표면 바깥으로 유지하면서 입력 또는 레이아웃 경계를 변경하지 않습니다. `enterKeyActivation` 는 기본값으로 false 이며, 이 작성된 계열에 의해 활성화되며, 관련 없는 컨트롤은 기존 키보드 핸들러를 유지합니다. Enter/Return 은 즉시 하단 상태 피드백을 제공하며, 반복된 키 누름을 무시하고, 키 릴리스 시 네이티브 `click()` 경로를 호출 (선택 가능/액션 의미 보존) 하며, 초점이 손실될 때 취소됩니다.

콤팩트 버튼 계열은 기존 기본, 호버, 눌림 및 키보드 초점 정책을 유지합니다. 릴리스는 콘텐츠와 표면의 180ms 탄성 복귀일 뿐이며, `released()` 로 트리거되며, 별도의 릴리스 채움 또는 테두리가 적용되지 않습니다. 빠른 탭은 반환되기 전에 최소 가시 압축을 받습니다. 취소되면 다시 활성화되지 않거나 작동하지 않습니다. 인터럽트를 다시 누르면 애니메이션, 축소된 동작, 로컬 동작 옵트아웃 또는 비활성화된 상호작용이 즉시 변환을 해제합니다. 레이아웃과 타격 기하학은 고정되어 있으며, 콜백은 애니메이션을 기다리지 않고 활성화 시점에 실행됩니다.

배경과 내부 포커스 장식이 인스턴스의 상태 객체를 소비합니다. `interactionPhase` 는 비활성화, 누름, 해제, 포커스, 호버 및 기본값을 우선순위로 합니다. 해제는 `InteractionMotion.releasing` 를 따르며, 동작 비활성화는 해당 일시적 단계를 종료하지만 논리적 누름/포커스는 여전히 작동합니다. 선택, 체크된 값 및 서브클래스 모델 상태 API 는 독립적으로 유지됩니다. 외부 상태 오버레이는 필요 없습니다.

입력 완료/취소는 `down`를 `undefined`로 재설정하여 이후 포인터 또는 공백 입력에 대한 Qt의 네이티브 누름 상태 추적을 복원합니다. 명시적으로 거짓 값을 남겨두면 이후의 언론 상태가 억제됩니다. [Qt 다운 계약](https://doc.qt.io/qt-6/qml-qtquick-controls-abstractbutton.html#down-prop)를 참조하세요.

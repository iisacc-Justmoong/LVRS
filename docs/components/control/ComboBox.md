# ComboBox

위치: `src/qml/components/control/buttons/ComboBox.qml`

`ComboBox`는 Figma 계약(`97x20`)을 따르고 `Stepper`를 후행 표시기로 사용하는 간단한 컨텍스트 메뉴 트리거 행입니다.

<a id="purpose"></a>

## 목적

- 메뉴와 같은 팝업을 열고 닫을 수 있는 경량 선택기 트리거를 제공합니다.
- 속성 API 를 2 개의 Figma -정의된 속성으로만 최소화하세요.

<a id="core-api"></a>

## 코어 API

- `text`(기본값: `"Label"`)
- `tone`(기본값: `ComboBox.Primary`)
  - `ComboBox.Primary`
  - `ComboBox.Borderless`
- `arrow`(기본값: `Stepper.UpDown`)
  - `Stepper.UpDown`
  - `Stepper.Up`
  - `Stepper.Down`

신호:

- `clicked()`
- `pressed()`
- `released()`
- `canceled()`

주입된 방법:

- `method`: 하나의 호출 가능 항목이 콤보 트리거에 직접 삽입됩니다.
- `methods`: 콜러블 또는 명령 객체의 배열
- `hasInjectedMethods`(읽기 전용)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

<a id="visual-contract"></a>

## 시각적 계약

- 데스크탑 논리 프레임: `97 x 20`
- 모바일 프레임이 데스크탑과 일치함: `97 x 20`
- 컨테이너 패딩: 왼쪽 `8`, 오른쪽 `1`, 상단/하단 `1`
- 반경: `Theme.radiusControl` (`5`)
- 컨테이너 베이스/호버/누름 색상:
  - `Theme.panelBackground10`
  - `Theme.panelBackground11`
  - `Theme.panelBackground12`
- 레이블 텍스트의 기본값은 `"Label"`이고 고정된 본문 `13px / 13px` 타이포그래피를 흰색으로 렌더링하며 콤보 프레임 내에서 삭제됩니다.
- 후행 표시기는 항상 `Stepper`이며 콤보 톤에서 톤이 매핑됩니다.
- 라벨 영역과 표시기 슬롯은 명시적으로 배치됩니다.
  - 라벨 슬롯: `x=8`, `y=3.5`, `width=70`, `height=13`
  - Figma의 기본 `"Label"` 텍스트 노드는 `x=8`, `y=3.5`, `width=33`, `height=13`를 차지합니다. 더 넓은 프로덕션 슬롯으로 동적 텍스트의 제거가 유지됩니다.
  - 스테퍼 프레임: `x=78`, `y=1`, `size=18`
- 소스 구성요소 세트: Figma 노드 `254:889`; 6개의 `Tone × Arrow` 변형은 모두 동일한 프레임 형상을 공유합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.ComboBox {
    text: "Control"
    tone: LV.ComboBox.Borderless
    arrow: LV.Stepper.Down
    method: function(eventData) { menu.open() }
}
```

<a id="practical-notes"></a>

## 실용적인 참고 사항

- `tone`는 `Stepper` 모양에만 영향을 미칩니다(`Primary` 파란색 / `Borderless` 투명).
- `arrow`는 열린 방향 상태(`Up`, `Down`, `UpDown`)를 표현합니다.
- `clicked()`에서 주입된 `method` 및 `methods`는 신호 발송의 일부로 순서대로 실행됩니다.
- 구성 요소는 표시기 슬롯에 대해 `RowLayout`를 사용하지 않으므로 다른 레이아웃 컨테이너 내부에서 사용될 때 스테퍼가 늘어나거나 접힐 수 없습니다.
- `ContextMenu`와 페어링되면 팝업 크기 조정은 고정 `ComboBox` 프레임과 독립적으로 유지됩니다. 콘텐츠 또는 명시적 팝업 너비가 필요한 경우 메뉴가 트리거 너비 이상으로 확장될 수 있으며, 좁은 트리거 너비 바인딩은 더 이상 팝업을 암시적 콘텐츠 너비 아래로 고정하지 않습니다.

<a id="shared-motion"></a>

## 공유 모션

컴팩트한 합성는 하나의 장치로 변환됩니다. 클릭된 신호는 즉시 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

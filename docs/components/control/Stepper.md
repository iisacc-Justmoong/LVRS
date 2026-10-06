<a id="stepper"></a>

# 스테퍼

위치: `src/qml/components/control/buttons/Stepper.qml`

`Stepper`는 정사각형 `Theme.iconSm` 프레임(데스크톱의 경우 `18 x 18`, 모바일의 경우 `36 x 36`)과 `Up`, `Down` 및 `UpDown` 화살표 모드를 갖춘 소형 독립형 방향 제어 장치입니다.

<a id="purpose"></a>

## 목적

- 조밀한 제어 행에 대해 최소 증가/감소 트리거 모양을 제공합니다.
- `Primary` 및 `Borderless` 톤 전체에서 상호 작용 상태를 결정적으로 유지합니다.

<a id="core-api"></a>

## 코어 API

- `tone`(기본값: `AbstractButton.Primary`)
- `arrow`(기본값: `Stepper.UpDown`)
  - `Stepper.UpDown`
  - `Stepper.Up`
  - `Stepper.Down`
- `stepped(direction)`는 포인터 클릭 후 증가의 경우 `1`를 내보내고 감소의 경우 `-1`를 내보냅니다. `UpDown`는 상/하반을 사용합니다. `Up` 및 `Down`는 항상 표시된 방향을 방출합니다. 비활성화된 컨트롤은 `clicked` 또는 `stepped`를 방출하지 않습니다.

계산된 속성:

- `iconWidth`, `iconHeight`(모드 종속 시각적 계약)
- `iconBounds`(정사각형 버튼 내부의 실제 중심 아이콘 아트워크 경계)
- `renderedBackgroundColor`(렌더링된 프레임에 바인딩된 색상)
- `resolvedIconColor`(활성화 인식 화살표 색상)
- `resolvedIconName`(`tone` + `arrow` 변형에 대한 호환성 이름)
- `resolvedIconAssetName`(실제 공유 Figma SVG 자산)
- `iconRotation`(`Up`의 경우 `180`, 그렇지 않은 경우 `0`)

주입된 방법:

- `method`: 스테퍼에 직접 주입되는 호출 가능 항목 1개
- `methods`: 콜러블 또는 명령 객체의 배열
- `hasInjectedMethods`(읽기 전용)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

<a id="visual-contract"></a>

## 시각적 계약

- 고정 프레임: `Theme.iconSm`(데스크톱에서는 `18 x 18`, 모바일에서는 `36 x 36`)
- 코너 반경: `Theme.radiusSm`(데스크톱에서는 `4`, 모바일에서는 `8`)
- 기본 톤:
  - 배경: `Theme.primary`
  - 아이콘: `Theme.accentWhite`
- 경계 없는 톤:
  - 배경: 투명
  - 아이콘: `Theme.accentWhite`
  - 마우스를 올리거나 누른 배경은 테두리 없는 버튼 정책을 따릅니다.
- 데스크탑 기준선의 화살표 아트워크:
  - `Up` / `Down`: `10 x 6`
  - `UpDown`: `6.43604 x 11.1455`
- 데스크톱과 모바일은 동일한 비율과 `18 x 18` `Theme.iconSm` 프레임을 공유합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Stepper {
    tone: LV.AbstractButton.Borderless
    arrow: LV.Stepper.Up
    method: function(eventData) { increment() }
}
```

```qml
LV.Stepper {
    arrow: LV.Stepper.UpDown
    onStepped: function(direction) { quantity += direction }
}
```

`ListItem`는 이 이벤트를 범위 고정 수량 상태 및 키보드 Up/Down과 결합합니다. 방향 포인터 동작은 기존 Stepper 시각적 계약 외에도 `LVRSTests_list_composites`에서 다룹니다.

<a id="how-it-works"></a>

## 동작 원리

- `StepperChevron.svg` 및 `StepperUpDownChevron.svg`에서 정확한 Figma 내보내기를 해결합니다. `Up`는 공유 단일 갈매기형 자산을 `180`도만큼 회전합니다.
- `RenderQuality.effectiveSupersampleScaleValue` 및 `Screen.devicePixelRatio`에서 슈퍼샘플링된 `Image.sourceSize`를 요청하므로 정적 SVG가 HiDPI 대상에서 선명하게 유지됩니다.
- 정수 픽셀 스냅 없이 현재 Figma 노드 `254:506` 아트웍을 중앙에 배치합니다. 따라서 데스크톱 18px 기준선에서 `iconWidth`, `iconHeight` 및 `iconBounds`는 정확하게 내보낸 분수 형상을 유지합니다.
- 렌더링된 아트워크는 미리 추출된 SVG 리소스에서 가져오는 동안 가리키기/누르기/비활성화된 마우스 파이프라인을 구성 요소에 로컬로 유지합니다.
- `clicked()`가 생성될 때마다 공유 버튼 메서드 레지스트리를 통해 주입된 `method` 및 `methods`를 실행합니다.

<a id="practical-notes"></a>

## 실용적인 참고 사항

- 일반 스피너 어포던스에는 `Stepper.UpDown`를 사용하세요.
- 별도의 제어가 필요한 경우 `Stepper.Up`/`Stepper.Down`를 사용하세요.

<a id="shared-motion"></a>

## 공유 모션

각 프레스가 리바운드됩니다. stepped는 실제 클릭 하프에 따라 +1 또는 -1를 전달합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

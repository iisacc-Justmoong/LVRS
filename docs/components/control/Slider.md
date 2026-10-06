<a id="slider"></a>

# 슬라이더

`LV.Slider` 는 LVRS Figma 슬라이더 라이브러리에서 입력되는 수평 값입니다. Qt 퀵 템플릿의 슬라이더를 포인터, 터치, 키보드, 범위, 그리고 접근성 동작에 사용하며, LVRS 기하학, 색상, 아이콘, 및 타이포그래피를 사용합니다. 추가적인 의존성이 필요하지 않습니다.

```qml
import LVRS 1.0 as LV

LV.Slider {
    width: 320
    type: LV.Slider.CenterBiasedTicks
    size: LV.Slider.Regular
    from: -100
    to: 100
    value: 0
    stepSize: 10
    Accessible.name: "Exposure"
    onMoved: exposureModel.exposure = value
}
```

<a id="variants"></a>

## 변형

| `type` |모양|기본 끝점|기본 단계|
| --- | --- | --- | --- |
| `Slider.Default` |얇은 트랙과 원형 조절 손잡이|숨겨진|연속|
| `Slider.CenterBiased` |채우기가 범위 중간점에서 시작됩니다.|숨겨진|연속|
| `Slider.Ticks` |11 틱이 포함된 얇은 트랙|숨겨진|연속|
| `Slider.CenterBiasedTicks` |중간점 채우기, 중앙 마커, 11 틱|숨겨진|연속|
| `Slider.Filled` |캡슐 트랙, 선택적 선행 기호|숨겨진|연속|
| `Slider.MinMaxLabels` |끝점 레이블이 있는 캡슐 트랙|표시|연속|
| `Slider.Segmented` |캡슐 트랙, 5 정지 및 가시적인 스무지|표시|25% 의 범위|

`type` 및 `size`는 현재 값 및 상호 작용 상태와 무관합니다. 진드기는 시각적 가이드입니다. 연속 변형에는 스냅을 적용하지 않습니다.

| `size` |제어 높이|얇은 트랙|얇은 썸|캡슐 트랙|캡슐 썸|
| --- | ---: | ---: | ---: | ---: | ---: |
| `Slider.Mini` | 22 | 4 | 12 | 12 | 8 |
| `Slider.Small` | 24 | 6 | 14 | 16 | 8 |
| `Slider.Regular` | 32 | 6 | 18 | 28 | 20 |
| `Slider.Large` | 44 | 8 | 22 | 44 | 36 |

기본 크기는 Regular이고 암시적 너비는 320입니다. 너비는 자유롭게 변경할 수 있습니다. 끝점 텍스트는 트랙을 소비하기 전에 사라집니다. 측정항목은 데스크톱과 모바일에서 동일하게 유지됩니다. 이 Figma 디자인은 수평 컨트롤입니다.

<a id="values-and-input"></a>

## 값 및 입력

네이티브 `from`, `to`, `value`, `position`, `visualPosition`, `stepSize`, `snapMode`, `live`, `pressed`, `hovered`, `moved()`, `increase()`, `decrease()`, 및 `valueAt()` API 는 여전히 사용 가능합니다. 기본 범위는 0 – 1 이며 초기 값은 0.5입니다. 프로그래밍 값은 Qt 에 의해 범위로 제한되며, `moved()` 는 사용자 입력을 위해 사용되고, `valueChanged` 도 프로그래밍 업데이트를 관찰합니다.

마우스/터치 드래그 및 트랙 누르기는 값을 변경합니다. 화살표 키로 조정하고, Home/End가 범위 끝점을 선택하고, Tab이 컨트롤에 도달합니다. 마우스 휠 입력은 `wheelEnabled: true`에서 선택적으로 제공됩니다. 각 컨트롤에 대해 애플리케이션별 `Accessible.name`를 설정합니다.

세그먼트의 경우, `segmentCount` 는 5 로 기본값이 설정되며 최소 2로 제한됩니다. 기본 `stepSize` 는 범위와 개수를 따르고, `snapMode` 는 `Slider.SnapAlways` 입니다. 점과 스냅 위치를 정렬하기 위해 `segmentCount` 를 변경하는 것이 좋습니다. 네이티브 `stepSize` 또는 `snapMode` 를 명시적으로 오버라이드하는 것은 지원되며, 표시된 정지와 일관된 설정을 유지하세요. Qt 의 슬라이더와 마찬가지로 스냅핑은 사용자 입력을 지배하며, `value` 에 대한 직접 할당에는 적용되지 않습니다.

```qml
LV.Slider {
    type: LV.Slider.Segmented
    from: 1
    to: 5
    value: 3
    segmentCount: 5
    minimumLabel: "Low"
    maximumLabel: "High"
    Accessible.name: "Quality"
}
```

중앙 바이어스 유형은 0이 아닌 범위 또는 하강 범위를 포함하여 `(from + to) / 2`를 중립점으로 사용합니다. 채워진 유형은 둥근 시작 캡을 최소한으로 유지하고 전체 트랙을 최대로 채웁니다. 오른쪽에서 왼쪽 레이아웃은 트랙, 끝점, 채우기, 틱 및 기호를 함께 미러링합니다.

<a id="labels-icons-and-appearance"></a>

## 레이블, 아이콘 및 모양

|속성|기본/목적|
| --- | --- |
| `showMinMax` |MinMaxLabels 및 세그먼트의 경우 True입니다. 그렇지 않으면 false|
| `minimumLabel`, `maximumLabel` |`"0%"`, `"100%"`; 자동 형식화된 값이 아닌 편집 가능한 문자열|
| `showLabels` |True; LVRS Body 13 / Pretendard Medium 을 사용합니다.|
| `showEndpointIcons` |거짓|
| `minimumIconName`, `maximumIconName` |`"sun"`; 기존 LVRS 아이콘 이름|
| `minimumIconSource`, `maximumIconSource` |선택 사항인 URL 는 해당 아이콘 이름을 덮어씁니다.|
| `symbolName`, `symbolSource` |`"sun"` 및 옵션 URL 재정의|
| `showSymbol` |True; Filled Regular/Large 에서만 표시됩니다.|
| `showTicks`, `tickCount` |유형에 따른 가시성; 11 기본적으로 얇은 트랙 틱|
| `showThumb` |Thin 과 Segmented 타입에는 True, Filled/ MinMaxLabels 에는 false 입니다.|
| `showFocusRing` |거짓; 호버/누르기/키보드 포커스 외에 명시적 벨소리 추가|
| `active` |참; false는 입력을 활성화한 상태에서 비활성 회색 채우기를 사용합니다.|
| `trackColor`, `pressedTrackColor` |패널 12 및 surfaceSolid|
| `fillColor`, `inactiveFillColor` |악센트 및 비활성화된 텍스트 토큰|
| `thumbColor`, `labelColor`, `tickColor` |TitleHeader, 본문, 캡션|

`sun` 문자 모양은 18×18에서 렌더링된 기존 LVRS 리소스입니다. Figma 아이콘은 16에서 18까지 확장된 동일한 8개 경로입니다. 컨트롤 형상을 변경하지 않고도 아이콘 URL과 이름을 바꿀 수 있습니다.

사용할 수 없는 작업에는 `enabled: false` 를 사용하며, 이는 입력을 차단하고 전체 컨트롤 32% 불투명도를 제공합니다. 독립적으로 창 활성화에는 `active` 를 사용하며, 예를 들어 `active: applicationWindow.active` 입니다. 마우스 오버 및 누름은 캡슐 스마일과 1px/2px 강조 고리를 표시합니다. 키보드 초점 또한 스마일과 고리를 표시합니다.

`displayState` 는 `Slider.Automatic` 로 기본값입니다. `IdleState`, `HoverState`, `PressedState`, 및 `DisabledState` 는 결정론적 카탈로그/디자인 미리보기를 제공합니다. DisabledState 는 `enabled: false` 를 공급하며, 애플리케이션은 런타임 가용성을 위한 네이티브 `enabled` 속성을 사용해야 합니다. 명시적인 QML 할당은 `enabled` 에 이 기본 바인딩을 대체합니다. 미리보기 상태는 `value` 를 쓰거나 포인터 입력을 시뮬레이션하지 않습니다.

조절 손잡이의 높이 효과와 캡슐 내부 그림자는 InputField와 같은 캐시된 Canvas 기법을 사용하며 소프트웨어 및 RHI 렌더러를 지원한다. 비활성 제어는 레이어를 사용하여 32% 불투명도가 전체 합성에 적용되게 하고, 채우기 위의 반투명 조절 손잡이를 보존하며 [Qt의 레이어 불투명도 규칙](https://doc.qt.io/qt-6/qml-qtquick-item.html#layer-opacity-vs-item-opacity)을 따른다. Figma 캡슐의 칠은 불투명하므로 배경 흐림이 표시 결과를 바꾸지 않는다. 화면 밖 배경 캡처는 필요하지 않다.

<a id="figma-and-validation"></a>

## Figma 및 검증

- [Public Slider 디자인](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=859-4157): 7 패밀리 × 4 크기 × 4 상태입니다.
- [내부 값 기하학](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=859-4158): 연속 위치, 중앙 채우기, 캡슐 채우기 및 세그먼트.
- [Qt 슬라이더 API](https://doc.qt.io/qt-6/qml-qtquick-controls-slider.html): 상속된 입력/범위 의미 체계.

VisualCatalog → Control → Input → Slider 는 인터랙티브 예제와 완전한 112-variant 행렬을 포함합니다. `tests/tst_slider.cpp` 는 Figma 기하학, 상태 보존, 크기 조정, 범위 동작, 입력, 레이블/아이콘, 및 렌더링된 출력을 다룹니다. 설치된 소비자 테스트 또한 모든 타입을 인스턴스화하고 내장된 QML 를 소스에 대해 검증합니다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build -R 'LVRSTests_(slider|examples|import_api)' --output-on-failure
```

<a id="shared-motion"></a>

## 공유 모션

포인터 추적은 직접적입니다. 엄지 손가락은 유지 시 변형되며, 이산적인 위치 변경은 반동으로 안정화됩니다. 전역 속도, 감소된 동작, 로컬 오버라이드 및 컴포넌트별 VisualCatalog 레시피에 대한 글로벌 속도는 [동작 정책](../../motion.md) 을 참조하세요.

# InputField

위치: `src/qml/components/control/input/InputField.qml`

`InputField`는 LVRS의 공개 단일 라인 `TextInput` 래퍼입니다. Qt의 네이티브 편집, IME, 선택, 클립보드 또는 접근성 동작을 대체하지 않고 노드 `114:179`에서 Figma `TextField` 구성 요소를 구현합니다.

<a id="purpose"></a>

## 목적

- `Rounded`, `Cylinder` 및 `Inline` 스타일로 안정적인 한 줄 텍스트 항목을 제공합니다.
- 네이티브 `TextInput` 상태로 Figma의 기본, 비활성화, 커서 활성, 선택, 활성 및 검색 상태를 처리한다.
- 데스크탑과 모바일에서 동일한 필드 지오메트리와 Body `13px / 13px` 타이포그래피를 사용하세요.
- 측정된 검색 어포던스와 통합된 명확한 작업을 포함합니다.

<a id="core-api"></a>

## 코어 API

모드:

- `defaultMode`, `searchMode`, `mode`
- `search`(`mode == searchMode`용 간편 바인딩)
- `searchIconVisible`
- `clearButtonVisible`, `showClearButton`(읽기 전용)

스타일:

- `roundedStyle`, `cylinderStyle`, `inlineStyle`
- `filledStyle`(`roundedStyle`의 호환성 별칭)
- `style`, `resolvedStyle`(읽기 전용)

형상 및 자리 표시자:

- `fieldMinHeight`: 기본값은 `Theme.controlHeightSm`(`22` 논리 픽셀)입니다.
- `insetVertical`: 분수 패딩, 기본값 `(fieldMinHeight - centeredTextHeight) / 2`, 0으로 고정됩니다.
- `centeredTextHeight`: 본체 `13`; 상속된 `centeredTextY`는 절반 픽셀 위치를 유지합니다.
- `placeholderColor`, `placeholderColorDisabled`: 둘 다 비활성화된 토큰인 `Theme.disabledColor`가 기본값입니다.

재료:

- `glassEnabled`(기본 `true`), `glassBlurRadius`(데스크톱 `8`, 인라인 `6`)
- `backdropSource`: 기본값은 포함된 Controls/LVRS ApplicationWindow의 배경입니다.
- `resolvedBackdropSource`, `glassActive`(읽기 전용)
- 상속된 `backgroundColor` 및 상태 변형은 여전히 재질 색조를 재정의합니다.
- 상속된 `backgroundComponent`는 입력 또는 슬롯 형상을 변경하지 않고 장식을 대체합니다.

검색/지우기 영상:

- `searchIconSize`, `searchIconSource`
- `clearIconSize`, `clearIconMarkLength`, `clearIconMarkThickness`
- `clearIconBackgroundColor`, 호버/눌림/비활성화 변형
- `clearIconForegroundColor`

`AbstractInputBar`에서 상속된 텍스트/입력 API에는 `text`, `placeholderText`, `readOnly`, `validator`, `maximumLength`, `inputMethodHints`가 포함됩니다. `echoMode`, `renderType`, `inputItem`, 선택/커서 별칭, `accepted(text)` 및 `textEdited(text)`.

<a id="geometry-contract"></a>

## 기하학 계약

The 2026-09-10 LVRS 조정은 모든 3 스타일과 모든 상호작용 상태에 걸쳐 `22px` 필드 높이와 Disabled 토큰을 사용합니다. 이는 원래 Figma 참조의 `19px` 프레임을 업데이트합니다.

|미터법|데스크탑|모바일|
| --- | ---: | ---: |
|안정적인 필드 프레임| `206 × 22` | `206 × 22` |
|왼쪽 삽입; 지우기 버튼이 없는 오른쪽 삽입| `7` | `7` |
|지우기 버튼 오른쪽 삽입| `8` | `8` |
|텍스트/자리 표시자 위쪽 및 아래쪽 삽입| `4.5` | `4.5` |
|상단 및 하단 삽입 검색/삭제| `5` | `5` |
|검색/프레임 지우기| `12 × 12` | `12 × 12` |
|둥근 반경| `8` | `8` |
|실린더/인라인 반경| `11` | `11` |
|검색/텍스트/공백 지우기| `2` | `2` |
|본문 글꼴 및 라인 상자| `13 / 13` | `13 / 13` |

Figma 의 활성, 선택 및 검색 인스턴스는 `205px` 를 보고하며 기본 및 비활성 인스턴스는 `206px` 를 보고합니다; 그 차이는 변형별 로컬 자동 레이아웃 콘텐츠에서 비롯됩니다. 런타임 필드는 의도적으로 `206px` 로 유지되므로, 타이핑, 선택, 검색 및 클리어 버튼 표시가 항상 주변 레이아웃을 1 픽셀만큼 이동시키지 않습니다.

모바일에서 프레임, 인셋, 반지름, 및 아이콘 프레임은 데스크톱과 일치합니다. 본문은 `13px / 13px` 로 유지되며, `22px` 프레임 내에서 `y = 4.5` 에 중앙 정렬됩니다. 검색 및 클리어 아이콘은 `y = 5` 에 중앙 정렬됩니다. `AbstractInputBar.insetVertical`, `centeredTextY`, 및 `contentBoxHeight` 는 실제 값을 사용하므로 홀수 줄 높이는 짝수 필드 높이에서 정확히 중앙에 위치합니다.

명시적 `fieldMinHeight`, `height`, 및 인셋 오버라이드는 여전히 사용 가능합니다. 예를 들어 `fieldMinHeight` 를 `30` 로 증가시키면 `8.5px` 텍스트 패딩이 생성됩니다. 내장 TableCellItem 및 레거시 미니 ListItem 편집기는 명시적 라인 박스 및 0-인셋 오버라이드를 유지하며, 표준 InlineEdit 및 폼 입력 슬롯은 `22px` 기본값을 따릅니다.

클리어 버튼 인셋은 명시적인 2026-09-06 LVRS 조정으로 Figma 참조에 적용됩니다. 클리어 버튼이 가시화될 때, 그 오른쪽 가장자리가 필드의 오른쪽 가장자리로부터 정확히 `8` 논리 픽셀 떨어져 있으며, 너비 또는 `insetHorizontal` 가 변경되더라도 마찬가지입니다. `AbstractInputBar.insetRight` 는 기본적으로 `insetHorizontal` 입니다; `InputField` 는 클리어 버튼 표시 시 `8` 에 이를 바인딩합니다. 후방 앵커 및 텍스트 예약은 모두 해당 오른쪽 인셋을 사용하여 텍스트와 버튼 간 간격 및 왼쪽/검색 레이아웃을 보존합니다.

<a id="behavior-contract"></a>

## 행동 계약

- `style: roundedStyle` 는 `8px` 반지름을 가진 함몰 유리 재료를 사용하여 LVRS 버튼과 일치합니다. 기본 (`114:176`), 비활성화 (`114:174`), 커서 활성 (`114:175`), 선택 (`114:177`), 검색 (`323:9164`), 및 활성 (`114:178`)) 인스턴스인 Figma 라운드 모두 이 반지름을 사용합니다.
- `style: cylinderStyle`는 높이 반경이 절반인 동일한 재료를 사용합니다.
- `style: inlineStyle`는 동일한 삽입 및 어포던스로 더 가벼운 반투명 소재와 더 얕은 삽입 조명을 사용합니다.
- 비활성화된 텍스트는 `Theme.disabledColor`를 사용합니다. 기본/활성 텍스트는 `Theme.titleHeaderColor`를 사용합니다.
- 빈 필드 자리 표시자는 활성화 및 비활성화 상태에서 `Theme.disabledColor`를 사용합니다. 입력한 텍스트는 자체 텍스트-색상 계약을 유지합니다.
- 선택 항목은 `Theme.accent`를 사용하고 선택한 텍스트는 `Theme.titleHeaderColor`로 유지됩니다.
- 검색에서는 일반 `16 × 16` 아이콘을 확장하는 대신 Figma `12 × 12` 벡터에서 측정된 구성 요소별 `inputFieldSearch.svg`를 사용합니다.
- 클리어는 `clearButtonVisible && enabled && !readOnly && text.length > 0`인 경우에만 표시됩니다. 클릭하면 필드가 비워지고 입력 포커스가 복원됩니다.
- 선행 및 후행 고유 너비는 필드 또는 조상이 숨겨져 있는 동안에도 삽입 계약의 일부로 유지되어 뷰가 표시될 때 지연된 지오메트리 이동을 방지합니다.
- iOS 대상에서 네이티브 `TextInput`는 여전히 `NativeRendering`를 사용하고 플랫폼 기반 키보드, IME, 선택 및 포인터 동작을 유지합니다.

`LVRSTests_import_api::input_field_figma_contract_loads` 체크를 수행하여 `22px` 프레임, `4.5px` 텍스트 중앙 정렬, 및 `5px` 아이콘 중앙 정렬을 3 모든 스타일과 macOS , iOS , 및 Android 테마 설정의 검색 모드 전반에 걸쳐 확인합니다. 또한 사용자 정의 필드 높이/너비/내측 여백과 초점 복원 시 클릭 시 지우기 기능을 확인합니다. `input_field_material_rendering_contract` 는 18 스타일/상태 조합 전반에 대해 실제 플레이스홀더 색상, 가시성, 및 라인박스 정렬을 확인하며, 네이티브 입력, 선택, 및 마テリア렌더링과 함께 수행됩니다. 모바일 테마 확인은 호스트에서 실행되며 기기 테스트로 간주되지 않습니다.

<a id="textfield-material"></a>

## TextField 재질

2026-09-06의 [Figma TextField](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=114-179)는 visionOS 재질과 안쪽 조명을 LVRS의 기존 작은 치수에 맞춘다. 활성화된 모든 상호작용 상태는 같은 재질을 공유하며 hover와 focus가 불투명 채우기로 대체하지 않는다. 비활성 상태는 흐림, 내부 그림자와 테두리를 제거한다.

|데스크탑 재질 값|둥근/원통|비활성화 둥근/원통|인라인|
| --- | --- | --- | --- |
|`panelBackground10` 틴트 알파| 64% | 36% | 16% |
|흰색 반사 알파| 1.8% | 1.8% | 1.8% |
|세로 그라데이션: 상단 검정색 / 52% 투명 / 하단 흰색| 20% / 0% / 3.5% | 4.5% / 0% / 1.5% | 6% / 0% / 1.5% |
|배경 흐림 반경| 8 |꺼짐|6, 비활성화되면 꺼짐|
|검정색 내부 그림자: x / y / 흐림 / 알파| 0.5 / 1.2 / 2 / 30% |꺼짐| 0 / 0.65 / 1 / 12% |
|흰색 내부 그림자: x / y / 흐림 / 알파| 0 / -0.5 / 0.8 / 16% |꺼짐| 0 / -0.5 / 0.6 / 5.5% |
|내부 가장자리| 0.5 |없음|없음|

내측 가장자리는 28% → 1.5% → 17% 에서 0%, 50%, 및 100%에 수직 검은색에서 흰색으로 이어지는 그라디언트를 사용합니다. 효과 거리와 색상 알파 값은 데스크톱과 모바일에서 동일합니다. 인라인은 비활성화 시 채우기를 유지하지만 내측 그림자를 제거합니다. Figma 페인트 스택은 반사된 오목한 그라디언트에서 시작하여 틴트로 이어지고, 그 후 내측 조명과 가장자리로 이어집니다.

`InputField.qml`는 전용 인라인 `MaterialSurface` 구성 요소에 장식을 유지합니다. 이는 기존의
[Qt 퀉 MultiEffect](https://doc.qt.io/qt-6.8/qml-qtquick-effects-multieffect.html) 및 `ShaderEffectSource` 접근 방식은 알림에 사용되며, 새로운 패키지 의존성이 추가되지 않습니다. 캡처는 블러 패딩을 포함하며, 필드에 마스킹되고 가시화되고 활성화된 동안 라이브로 유지됩니다. 필드의 재배치 또는 스크롤은 캡처 영역을 업데이트합니다. 텍스트, 선택, 검색 및 클리어 컨트롤은 효과 바깥에 있습니다.

샘플링되려면 소스는 배경만 포함해야 합니다. 필드 뒤의 별도 패널 또는 이미지는 `backdropSource` 에 할당할 수 있으며, 필드, 하위 요소, 그리고 해당 필드를 포함하는 조상 요소는 캡처 피드백을 피하기 위해 거부됩니다. 평범한 `Window` 는 기본 소스가 없습니다. 소스가 없거나 `glassEnabled: false` 가 있거나 Qt 의 소프트웨어 렌더러에서는 틴트, 그라데이션, 내부 조명 및 내부 테두리가 가시화되며, 실제 배경 블러에는 Qt RHI 렌더러가 필요합니다.

하위 픽셀 내부 조명 및 가장자리는 캐시된 장치 픽셀 비율 인식을 사용합니다.
[Qt 캔버스](https://doc.qt.io/qt-6.8/qml-qtquick-context2d.html). 외형 또는 기하학적 변화만 다시 그립니다; 입력은 장식을 다시 래스터화하지 않습니다. `input_field_material_rendering_contract` 는 모든 18 스타일/상태 조합을 렌더링하며, 내측 대비와 외부 페인트 부재를 확인하고, 상세한 배경에 대한 실제 블러를 검증하며, 라이브 소스 변경, 이동된 조상, 안전하지 않은 소스, 숨김/비활성화 캡처, 틴트 오버라이드, 네이티브 입력, 선택 및 클리어 포커스를 처리합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.InputField {
    style: cylinderStyle
    search: true
    placeholderText: "Filter"
}
```

```qml
Item {
    Rectangle {
        id: panelBackdrop
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#31425b" }
            GradientStop { position: 1; color: "#252629" }
        }
    }
    LV.InputField {
        backdropSource: panelBackdrop
        placeholderText: "Filter"
    }
}
```

```qml
LV.InputField {
    style: inlineStyle
    text: "Editable value"
}
```

<a id="shared-motion"></a>

## 공유 모션

초점 윤곽이 다시 나타납니다. 상속된 액세서리는 자체 버튼 피드백을 사용합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

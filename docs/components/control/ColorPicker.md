# ColorPicker

위치: `src/qml/components/control/input/ColorPicker.qml`

`LV.ColorPicker`는 내장형 색상 편집 보기입니다. 해당 부모는 패널, 모달 또는 팝오버 표면, 위치 지정, 열기 및 해제를 소유합니다. 제목 표시줄, 배경, 그림자, 오버레이 또는 자동 부모 재지정 기능이 없습니다. 참조는 [Figma ColorPickerPanel, 885:44522](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=885-44522)입니다.

<a id="variants-and-layout"></a>

## 변형 및 레이아웃

| `type` |내용|기본 크기|
| --- | --- | --- |
| `LV.ColorPicker.Wheel` |H/S/B 채널, 색조 링 및 채도/밝기 삼각형| 320 × 461 |
| `LV.ColorPicker.HueSaturation` |색조/채도 평면, 밝기 레일 및 B 채널| 320 × 437 |
| `LV.ColorPicker.SaturationBrightness` |채도/밝기 평면, 색조 레일 및 H 채널| 320 × 437 |
| `LV.ColorPicker.Grayscale` |K 채널 및 스펙트럼| 320 × 361 |
| `LV.ColorPicker.RGB` |R/G/B 채널 및 스펙트럼| 320 × 413 |
| `LV.ColorPicker.CMYK` |C/M/Y/K 채널 및 스펙트럼| 320 × 439 |

투명한 콘텐츠는 12px 패딩/격차, LVRS 본문 13 및 캡션 11, 22px `LV.InputField` 및 미니 `LV.Slider`, 28px 비교/16 진/알파 행, 20px 최근 스왑치 및 22px 동작을 사용합니다. 320px 이상의 너비가 지원되며, 정사각형 평면은 너비와 함께 성장합니다. 호스트가 이미 인셋을 제공하는 경우 `padding` 를 변경할 수 있습니다. 최근 색상을 숨기면 높이가 32px 만큼 감소하며, 동작을 숨기면 또 34px만큼 감소합니다.

## API

|속성/시그널/방법|계약|
| --- | --- |
| `type` |6개의 열거형 중 하나입니다. 이를 변경하면 색상 상태가 유지됩니다.|
| `currentColor` |편집 가능한 `color` 속성이며 기본값은 `#7A5AF8`이다. 네이티브 편집은 외부 QML 바인딩을 유지한다.|
| `previousColor` |기준선과 비교 견본의 왼쪽 절반을 취소합니다. 기본값은 외부에서 제공한 초기 색상입니다. 실시간 문서 미리보기를 지원할 때 명시적으로 전달하세요.|
| `hex` |읽기 전용 대문자 RRGGBB 텍스트입니다.|
| `recentColors` |호출자가 소유한 색상 배열이며 처음 8개 항목을 표시한다. 기본값은 8개의 Figma 색상 견본이다. 전역 이력은 영속 저장하지 않는다.|
| `showRecentColors`, `showActions` |둘 다 기본값은 `true`입니다.|
| `applyText`, `cancelText` |번역 가능한 작업 텍스트입니다.|
| `colorEdited(color)` |최근 색상 선택 또는 취소 시 복원을 포함한 사용자 편집. 외부 속성 변경은 이를 내보내지 않습니다.|
| `accepted(color)`, `canceled()` |호스트가 지속성/해제를 처리합니다. 두 신호 모두 보기를 닫지 않습니다.|
| `eyedropperRequested()` |호스트는 문서/화면 샘플링을 시작하고 결과를 `setColor(color)`로 제공합니다.|
| `setColor(color)` |동일한 상태 모델을 통해 색상을 편집합니다. 유효하지 않은 색상을 무시합니다.|
| `setHex(text)` |성공 여부를 반환한다. 선택적인 `#`, 6자리 RRGGBB 또는 8자리 RRGGBBAA를 받는다. 6자리 입력은 알파를 유지한다.|
| `accept()` |보류 중인 입력을 커밋하고 비교 기준을 업데이트하며 `accepted`를 내보냅니다.|
| `cancel()` |비교 기준선을 복원하고 `canceled`를 내보냅니다. Escape는 이 메소드를 호출합니다.|

수정된 숫자 필드는 Enter 키 입력 또는 포커스 손실 시 커밋됩니다. 단순히 둥근 표시 값을 포커스하는 것만으로는 기본 색상을 양자화하지 않습니다. 값은 H 0 – 360, RGB 0 – 255 및 기타 채널 0 – 100 로 제한되며, 유효하지 않거나 무한이 아닌 입력은 버려집니다. 알파는 색상 채널 수정 후에도 유지됩니다. 최근 색상을 선택하면 해당 팔레트의 알파가 사용됩니다.

HSV 편집은 0 채도를 유지한 채 선택된 색조를 유지하며, 0 밝기를 유지한 채 채도를 유지합니다. CMYK  편집은 다른 모델에서 편집할 때까지 nonzero K 를 포함한 모든 4 입력된 채널을 유지합니다. 변환은 Qt 의 sRGB / CMYK 산술을 사용하며, ICC 인쇄 프로파일 변환이 아닙니다. 따라서 푸터는 작업 공간을 sRGB 로 식별합니다. 그레이스케일 K 채널은 `1 − qGray(rgb)/255` 를 사용하며 이를 편집하면 무채색이 생성됩니다. 그레이스케일로 전환하는 것만으로는 현재 색상을 삭제하지 않습니다.

스펙트럼에는 가로 방향으로 색상이 포함되고 세로 방향으로 흰색 → 채도 → 검은색이 포함됩니다. 2차원 영역 밖의 색상의 경우 커서는 채도/밝기에서 가장 가까운 색조/음영 분기로 투영됩니다. 정확한 채널, Hex 및 비교 견본은 여전히 ​​실제 색상을 표시합니다.

<a id="pass-the-view-to-a-host"></a>

## 뷰를 호스트에 전달

```qml
import QtQuick
import LVRS as LV

Item {
    id: editor
    property color documentColor: "#7A5AF8"
    property color originalColor: "#7A5AF8"

    Component {
        id: colorView
        LV.ColorPicker {
            type: LV.ColorPicker.SaturationBrightness
            currentColor: editor.documentColor
            previousColor: editor.originalColor
            onColorEdited: color => editor.documentColor = color
            onAccepted: color => {
                editor.originalColor = color
                dialog.open = false
            }
            onCanceled: dialog.open = false
        }
    }

    LV.Modal {
        id: dialog
        minWidth: 360
        maxWidth: 360
        frameMinHeight: 0
        showIcon: false
        primaryText: ""
        contentComponent: colorView
        onCanceled: editor.documentColor = editor.originalColor
    }

    LV.LabelButton {
        text: "Edit color"
        onClicked: {
            editor.originalColor = editor.documentColor
            dialog.open = true
        }
    }
}
```

같은 `Component`를 다른 패널/팝오버의 `Loader`에 제공할 수 있다. 직접 배치한 `LV.ColorPicker` 자식도 지원한다. 호스트는 일반 수명 주기에 따라 뷰를 제거하며 선택기는 스스로 다른 부모로 이동하지 않는다. 별도 인스턴스는 독립적인 편집기 상태를 가진다. 호스트가 표면을 직접 닫는 경우(예: Modal 배경 클릭) 저장된 문서 색상을 직접 복원한다. `Modal.canceled`를 내보낼 때는 콘텐츠가 이미 언로드되었기 때문이다.

<a id="input-and-rendering"></a>

## 입력 및 렌더링

색상 도메인은 포인터와 단일 손가락 드래깅을 지원합니다. 탭은 도메인, 슬라이더, 필드, 스와치 및 액션을 선택합니다. 화살표 키는 선택된 도메인을 조정합니다: HS 평면은 채도/포화도, SB 평면은 포화도/밝기, 휠은 채도/밝기 (Shift+Up/Down 은 휠 포화도 조정). Shift 는 수평 및 레일 변경을 가속합니다. 기존 LVRS 슬라이더는 네이티브 키보드 동작을 유지합니다.

색조 링은 내보낸 Figma 자산입니다. 대화형 삼각형 및 평면은 Qt `QColor` 및 캐시된 `QQuickPaintedItem` 색상 도메인을 사용합니다. 추가 렌더링 종속성은 도입되지 않습니다. `ColorPickerModel` 및 `ColorPickerSurface`는 별도의 카탈로그 구성 요소가 아닌 구현 도우미입니다.

QML 모듈은 `DEPENDENCIES QtQuick`를 선언하므로 컴파일러/린트 도구는 [Qt의 모듈 종속성 계약](https://doc.qt.io/qt-6/qt-add-qml-module.html)에서 요구하는 대로 네이티브 항목 상속 및 QColor 속성을 확인할 수 있습니다.

검증: `LVRSTests_colorpicker`는 변환, 편집, 기하학, 호스트 라이프사이클, 바인딩, 입력 및 렌더링을 다룹니다. `LVRSTests_examples`는 등록된 갤러리를 확인합니다. `ctest --test-dir build --output-on-failure`를 실행합니다. 참조 캡처를 저장하기 위해 집중 테스트를 직접 실행할 때 `LVRS_COLORPICKER_CAPTURE_DIR`를 설정하십시오.

<a id="shared-motion"></a>

## 공유 모션

슬라이더 썸, 입력 포커스 및 작업 버튼은 LVRS 동작을 공유합니다. 색상 샘플링은 즉시 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

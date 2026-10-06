<a id="alert"></a>

# 경고

위치: `src/qml/components/surfaces/Alert.qml`

`Alert`는 1개, 2개 또는 3개의 작업이 포함된 유리 오버레이입니다. 시각적 계약은 변형 `106:282` 및 `106:281`를 포함하는 [Figma Alert 658:229](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=658-229)에서 제공됩니다. 텍스트 크기 조정은 2026-09-11의 현재 자동 높이 및 제목/본문 스타일과 조정되었습니다.

## API

- 상태/내용: `open` , `imageSource` , `title` , `description` , `buttonCount` ( `0=auto` , `2` , `3` ).
- 동작 인수: `button1Text` , `button1Method` , `button2Text` , `button2Method` , `button3Text` , `button3Method`.
- 활성화된 상태: `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`.
- 동작: `dismissOnBackground`, `useOverlayLayer`, `secondaryDestructive`.
- 크기/모양: `minWidth` , `maxWidth` , `preferredWidth` , `cardCornerRadius` , `shapeStyle` , `resolvedCardCornerRadius`.
- 재료: `backdropColor` , `cardBackgroundColor` , `glassEnabled` , `glassBlurRadius` , `backdropSource` , 읽기 전용 `resolvedBackdropSource` 및 `glassActive`.
- 아이콘: `showIcon` , `appIconSource` , `appIconSize` , `iconFrameSize` , `appIconBackgroundColor` , `appIconFrameColor` , `appIconInnerColor` .
- 신호: `primaryClicked()`, `secondaryClicked()`, `tertiaryClicked()`, `dismissed()`.

기존 액션 신호와 자동 카운트 동작은 호환성을 유지합니다: 서드터리 텍스트는 3 액션을 선택하고, 이차 텍스트는 2를 선택하며, 이차/서드터리 텍스트가 없는 경우 하나를 선택합니다. 액션은 제공된 메서드를 실행하고 기존 신호를 방출하며, 호출자가 닫기를 제어합니다.

<a id="content-and-action-arguments"></a>

### 내용 및 작업 인수

|인수|유형|기존 속성/작업|
| --- | --- | --- |
| `imageSource` | URL |`appIconSource` 의 별칭; 중앙 이미지 프레임에서 렌더링됨|
| `title` |문자열|가운데 제목|
| `description` |문자열|`message`의 별칭; 중심 설명|
| `button1Text` |문자열|`primaryText` 의 별칭|
| `button1Method` |호출 가능, 기본값 `null`|주요 액션; `primaryClicked()` 는 여전히 방출됨|
| `button2Text` |문자열|`secondaryText` 의 별칭|
| `button2Method` |호출 가능, 기본값 `null`|이차 액션; `secondaryClicked()` 는 여전히 방출됨|
| `button3Text` |문자열|`tertiaryText` 의 별칭|
| `button3Method` |호출 가능, 기본값 `null`|서드터리 액션; `tertiaryClicked()` 는 여전히 방출됨|

별칭은 생성 후 업데이트를 포함하여 기존 속성과 동일한 값을 공유합니다. 버튼 번호는 액션 역할을 식별합니다: 2-버튼 레이아웃의 오른쪽과 3-버튼 레이아웃의 상단에 있는 1 번 버튼은 주요 액션입니다. 2 번 버튼은 이차이며, 3 번 버튼은 최종/취소 액션입니다. `button3Text: ""` 를 2 액션에 설정하고, `buttonCount` 가 자동일 때 하나의 액션에 `button2Text: ""` 도 설정합니다. 메서드를 제공하는 것이 카운트를 변경하지 않습니다.

각 메서드는 기존 `AlertButton.method` API 에 직접 전달됩니다. 공유 `ButtonMethodRegistry` 를 사용하여 JavaScript 함수 또는 `invoke(eventData)` 또는 `trigger(eventData)` 를 노출하는 객체를 받습니다. `eventData.source` 는 클릭된 버튼이고, `eventData.trigger` 는 `"clicked"` 입니다. 활성화 상태 필드와 톤 필드는 [AbstractButton](../control/AbstractButton.md)에 따릅니다. 메서드는 인수를 생략할 수 있습니다. 자신의 리시버가 필요한 애플리케이션 메서드를 감싸세요. 예를 들어 `function() { documentController.save() }` 입니다. 생성 후 `null` 로 메서드를 대체하거나 지울 수 있습니다. 생략된 메서드는 신호 전용 API 를 사용할 수 있게 합니다. 동일한 애플리케이션 작업을 두 번 실행하지 않기 위해 각 작업을 메서드 또는 신호 처리기 중 하나에 유지하세요.

<a id="figma-layout-and-typography"></a>

## Figma 레이아웃 및 타이포그래피

참고 단일 줄 제목과 2-줄 설명을 사용하면, 2/3-액션 대화상자는 데스크톱과 모바일에서 500 × 379 / 500 × 479 로직 픽셀입니다. 카드가 호스트의 사용 가능한 너비로 축소되며, `minWidth` 보다 좁은 호스트도 포함됩니다. 기본값은 36px 둥근 사각형입니다. 호환성을 위한 기존 옵트인 `shapeCylinder` 는 여전히 사용 가능하지만 기본값으로 사용되지 않습니다.

이것들은 로직 픽셀입니다. 디바이스 픽셀 비율 2 를 가진 레티나 창은 500px 카드를 1000 물리 픽셀로, 56px 버튼을 112 물리 픽셀로 포착하며, 추가 UI 스케일을 적용해서는 안 됩니다. 390px 호스트는 342px 카드를 생성하며, 각 측면에 24px 마진을 유지합니다. 아이콘이 없는 Society 공지사항은 제목/메시지 각각 한 줄씩, 하나의 액션을 포함하여 500 × 252 로직 픽셀입니다. 기하학적 구조는 2026-09-18의 디자인과 대조하여 검증되었습니다.

콘텐츠는 46px 상단과 36px 하단 패딩을 가집니다. 86px 아이콘 프레임이 28px만큼 복사 앞에 위치하며 28px 모서리 반경을 가집니다. 정확한 Figma SVG 내보내기들은 번들로 제공되며: 64px 조정 2 액션, 56px 파일 텍스트 3입니다. `showIcon: false` 는 프레임과 28px 간격을 제거합니다. 명시적 커스텀 아이콘 URL 는 여전히 작동하며, 빈 URL 는 구성 가능한 대체 경로 를 유지합니다.

타이포그래피는 기존 `Label.title` ( 26px 굵은) 와 `Label.body` ( 13px 중간) 토큰 및 프레임워크의 내장 Pretendard 글꼴을 사용합니다. 새로운 텍스트 스타일이 도입되지 않습니다. 제목과 메시지 높이는 26px 와 13px 줄당 렌더링된 줄 수에 따르며, 14px 간격과 중앙 정렬된 텍스트를 사용합니다. Figma 의 두 텍스트 노드는 자동 높이를 사용합니다. 빈 텍스트는 라인 박스와 인접 간격을 해제하며, 두 문자열이 모두 빈 경우 전체 복사 섹션과 아이콘-복사 간격이 축소됩니다. 줄 바꿈 또는 명시적인 줄 바꿈을 추가하면 대화 상자가 커지고, 텍스트를 줄이면 다시 축소됩니다.

Figma 스냅샷은 2026-09-11 에서 검색되어 34px/56px 텍스트 경계와 417px/517px 카드 경계를 보고하며, 현재 26px/13px 줄 높이 스타일에 불구하고 있습니다. 구현은 스냅샷 경계를 최소 높이로 보존하는 대신 해당 스타일과 자동 높이 동작을 따릅니다. 이 작업은 참조 복사에 대한 38px 의 사용하지 않는 높이를 제거하며, 한 줄 제목과 한 줄 설명은 366px/466px를 사용합니다. 폭, 아이콘 프레임, 섹션 패딩, 작업 기하학, 그리고 재료는 디자인에 지정된 값으로 유지됩니다.

- 2 동작: 왼쪽에 보조/취소, 오른쪽에 기본; 14px 간격, 24px 측면, 28px 위 버튼 및 아래에 32px.
- 3 동작: 기본, 보조/삭제, 취소; 12px 간격, 24px 측면, 18px 위 및 아래.
- 주요 버튼: 56px 높이 및 16px 반경. 최종 취소: 44px, 투명, 윤곽선 없음. 보조 버튼은 1px 윤곽이 있는 투명합니다.
- `secondaryDestructive` 는 3 동작에 대해 기본값으로 true 로 설정되어 기존 `Theme.danger` 빨간색을 Discard/No 텍스트에 적용합니다. 중립적인 보조 동작으로 오버라이드하거나, 2-액션 Cancel 은 기본적으로 중립적으로 유지됩니다.
- 경보별 표시가 `AlertButton.dialogStyle`를 통해 활성화됩니다. 독립형 `AlertButton` 및 `Modal`는 컴팩트한 기본을 유지합니다.

<a id="glass-material-and-rendering"></a>

## 유리 소재 및 렌더링

경고는 `Theme.alert*` 하위의 Figma 색상 토큰을 사용하며: 72% 불투명도의 #1D1F21 , 20% 흰색 테두리, #F4F5F7 제목/액션, #D6D9DF 설명, #027DFF 기본 버튼, #596168 윤곽선, #363B3F 구분선 및 기본 테마의 파란색 아이콘 프레임입니다. 사용자 지정 `ApplicationWindow.primaryColor` 는 작업 색상을 덮어쓰고 아이콘 프레임을 틴트합니다. Society 는 의도적으로 `#57965C` 기본 색상을 사용하며, Figma 레이아웃과 일치하는 것이 해당 애플리케이션 색상을 대체하지 않습니다.

소스 창은 Qt로 캡처됩니다.
[ShaderEffectSource](https://doc.qt.io/qt-6/qml-qtquick-shadereffectsource.html) 및 흐려짐
[MultiEffect](https://doc.qt.io/qt-6/qml-qtquick-effects-multieffect.html). 둥근 마스크, 28px 프로스트, 반투명 틴트, 32px 소프트 섀도우는 샤프한 콘텐츠와 별개입니다. 이는 LVRS에서 이미 사용하고 있는 기존 Qt 빠른 효과이므로 추가 패키지 종속성이 도입되지 않습니다.

LVRS 창은 `materialBackdropSource` 가 `Controls.Overlay` 로 이동할 때 자동으로 발견됩니다. 이는 다른 LVRS 재료와 동일한 콘텐츠 전용 캡처 경로를 사용합니다. 일반 Qt ApplicationWindows 는 `ApplicationWindow.contentItem` 로 되돌아갑니다. 사용자 정의/일반 창은 `backdropSource` 를 자매 배경 항목으로 설정할 수 있습니다. Alert 를 포함하는 조상들은 재귀적 캡처를 방지하기 위해 거부됩니다. 안전한 소스가 사용 불가능하거나 글래스가 비활성화되면 반투명한 틴트가 여전히 렌더링됩니다. 닫는 것은 캡처를 연결 해제하고 라이브 업데이트를 중지합니다.

Qt의 흐림 및 가장자리 처리는 Figma 소재와 유사합니다. Figma의 독점 굴절/광 시뮬레이션은 재현되지 않습니다. 셰이더 효과에는 Qt RHI 그래픽 백엔드가 필요합니다. 소프트웨어 렌더링은 색조를 유지하지만 성에를 제공할 수는 없습니다.

<a id="interaction"></a>

## 상호작용

열려 있는 동안 백그라운드 입력이 사용됩니다. `dismissOnBackground`가 true인 경우에만 배경화면 클릭이 해제됩니다. 빈 카드 공간을 클릭해도 절대 닫히지 않습니다. 비활성화된 작업은 메서드를 호출하거나 클릭 신호를 내보내지 않습니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Alert {
    id: saveAlert
    open: true
    buttonCount: 3
    imageSource: "qrc:/qt/qml/LVRS/resources/images/alert-file-text.svg"
    title: "Save changes?"
    description: "You have unsaved changes.\nSave them before closing?"
    button1Text: "Save changes"
    button1Method: function() {
        documentController.save()
        saveAlert.open = false
    }
    button2Text: "Discard changes"
    button2Method: function() {
        documentController.discard()
        saveAlert.open = false
    }
    button3Text: "Cancel"
    button3Method: function() { saveAlert.open = false }
}
```

`documentController`는 애플리케이션에서 제공됩니다. URL 이미지는 Qt 리소스, 로컬 파일 또는 Qt Quick `Image`에서 지원하는 원격 이미지를 가리킬 수 있습니다.

<a id="validation"></a>

## 검증

`LVRSTests_import_api` 는 Figma 기하학, 제목/본문 토큰, 작업 순서, 캡슐이 아닌 모서리, 아이콘 내보내기, 너비 포함, 아이콘 가시성, 복사 기반 성장 및 축소, 빈 복사 축소, 반응형 래핑, 격리된 버튼 기본값, 실제 배경 흐림, 빨간색 Discard 픽셀, 클릭 신호, 모달 입력 차단, 및 캡처 수명 주기를 확인합니다. 또한 콘텐츠 인자와 라이브 별칭, 이미지 로딩, 하나/2/3-버튼 레이아웃에 걸쳐 렌더링된 모든 6 작업 위치, 함수/명령 방법, 방법 대체, 비활성화된 작업, 및 신호 처리기와의 호환성을 확인합니다. 재료/입력 테스트는 Qt 와 LVRS ApplicationWindows 에 대해 실행됩니다. 네이티브 RHI 는 샘플 `QQuickWindow::grabWindow()` 를 확인하므로 흐림 주장은 제시된 창뿐만 아니라 중간 항목 렌더링만 포함하지 않습니다. `LVRSTests_primary_color` 는 Alert 작업/아이콘 프레임 및 일반 컨트롤을 포함한 Society 의 녹색을 포함한 라이브 앱 강조 업데이트를 확인합니다.

지원되는 그래픽 백엔드로 실행:

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

CTest는 소프트웨어 렌더링을 선택할 수 있는 오프스크린 플랫폼을 사용합니다. 이 경우 픽셀 테스트는 반투명 대체 경로를 검증합니다. 실제 GPU 서리를 확인하려면 네이티브 플랫폼에서도 집중 테스트를 실행하세요. macOS 빌드 트리의 경우:

```sh
DYLD_LIBRARY_PATH="$PWD/build" QT_QPA_PLATFORM=cocoa QSG_RHI_BACKEND=metal \
  QT_QUICK_CONTROLS_STYLE=Basic \
  LVRS_ALERT_CAPTURE_DIR="$PWD/build/alert-preview" \
  ./build/tests/LVRSTests_import_api alert_figma_variant_contract_loads \
  alert_action_button_padding_scopes_to_alert alert_glass_overlay_and_input_contract
```

명시적인 라이브러리 경로는 이전에 설치된 LVRS가 검증 중인 라이브러리를 섀도잉하는 것을 방지합니다. 다른 플랫폼은 네이티브 RHI 백엔드를 사용해야 합니다.

렌더링된 참조 이미지를 저장하려면 `alert_glass_overlay_and_input_contract`를 실행할 때 `LVRS_ALERT_CAPTURE_DIR`를 `build/` 아래의 폴더로 설정하세요. 아이콘 알림은 `resources/images/alert-icons-LICENSE.txt`에 번들로 제공됩니다.

<a id="shared-motion"></a>

## 공유 모션

중앙에 위치한 카드는 배경이 희미해지는 동안 리바운드를 통해 92%에서 성장합니다. 출구는 정착될 때까지 계속 보입니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

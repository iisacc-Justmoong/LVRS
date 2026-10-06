<a id="sheet"></a>

# 시트

`LV.Sheet`는 모바일 하단 시트와 중앙 데스크톱 모달을 갖춘 콘텐츠 호스트입니다. [LVRS 시트 디자인](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=921-14595)를 구현합니다. 제공된 뷰는 해당 데이터, 입력 및 작업을 소유합니다. 시트는 배치, 헤더, 스크롤 및 해제를 소유합니다.

<a id="device-corner-radius"></a>

## 장치 코너 반경

`cornerRadius` 는 Qt 퀵 논리 픽셀**에서의 **실수입니다. 분수 값을 포함합니다. 애플리케이션의 장치 메트릭 제공자에서 측정된 디스플레이 모서리 반경을 전달하십시오. 더 큰 측정은 OS 이름과 관계없이 더 둥근 시트를 생성합니다. 0 는 명시적으로 사각 모서리를 의미합니다. 유효하지 않거나 무한하거나 음수 값은 0 로 해결되며, 과도한 값은 현재 표면의 짧은 차원의 절반으로 제한됩니다.

```qml
import QtQuick
import LVRS as LV

LV.Sheet {
    id: exportSheet
    title: "Export image"
    // deviceMetrics는 LVRS 싱글턴이 아니라 애플리케이션이 제공한다.
    // 측정값이 이미 논리 픽셀 단위라면 해당 값을 직접 바인딩한다.
    cornerRadius: mobilePresentation
        ? deviceMetrics.displayCornerRadiusPixels / deviceMetrics.devicePixelRatio
        : 16
    contentComponent: exportView
}
```

픽셀 비율은 측정이 수행된 디스플레이를 설명해야 합니다. 논리 측정값을 다시 곱하거나 나누지 마십시오. Qt 는 창 장치 픽셀 비율에서 논리 차원을 렌더링하며, [Qt 높음 DPI 좌표](https://doc.qt.io/qt-6/highdpi.html)를 참조하십시오. 예를 들어, 84 물리 픽셀과 DPR 3 및 165 물리 픽셀과 DPR 3 는 반경 28 와 55를 생성합니다. 이것은 픽스처, **에 대한 비교이며 Android 또는 iPhone 모델**에 대한 사양이 아닙니다.

기본 반경 (모바일 24, 데스크톱 16) 은 Figma 대체 경로 이며 감지된 하드웨어가 아닙니다. 시트는 개인 iOS API 를 쿼리하지 않으며, 안전 영역 내측에서 반경을 추론하거나 기기 이름 테이블을 유지하지 않습니다. 호출자는 실제 기기 측정값을 제공하고 호스트 디스플레이가 변경될 때 이를 업데이트하는 책임이 있습니다. 모든 4 표면 모서리는 제공된 반경을 사용하며, 디스플레이 모서리에 닿는 모바일 하단 모서리도 포함됩니다. 형상은 Qt 의 원형 둥근 사각형이며, 반경만으로는 하드웨어 초타원형을 설명할 수 없습니다.

<a id="content"></a>

## 내용

재사용 가능한 `Component`를 `contentComponent`를 통해 전달합니다. 인스턴스화된 뷰는 `loadedContent`로 노출됩니다. 닫거나 다시 열어도 활성 상태를 유지하며 구성 요소가 변경되면 교체됩니다. Fit sizing을 위해 `implicitHeight`를 설정합니다. 너비는 뷰포트 콘텐츠를 따릅니다.

```qml
Component {
    id: exportView
    Column {
        spacing: LV.Theme.gap12
        LV.Label { text: "Project overview.png" }
        LV.LabelButton { text: "Export image"; onClicked: exportSheet.close() }
    }
}
LV.LabelButton { text: "Export"; onClicked: exportSheet.open() }
```

또는 항목이나 레이아웃을 시트의 기본 `content` 슬롯 안에 직접 넣으세요. 너비를 `parent.width`에 바인딩합니다. 한 번에 하나의 콘텐츠 경로만 표시됩니다. `contentComponent`는 인라인 하위 항목보다 우선합니다.

```qml
LV.Sheet {
    id: shareSheet
    title: "Share project"
    cornerRadius: 55 // 예시이며 앱에서 기기 측정값을 바인딩한다
    Column {
        width: parent.width
        spacing: LV.Theme.gap12
        LV.Label { text: "Invite people to this project." }
        LV.LabelButton { text: "Copy link"; onClicked: shareSheet.close() }
    }
}
```

내장 콘텐츠 Flickable 은 오버사이즈 콘텐츠를 스크롤하고 헤더는 고정됩니다. ListView 를 소유하고 고정 하단 액션이 있는 뷰의 경우 `scrollContent: false` 를 설정하면 로드된 뷰는 사용 가능한 콘텐츠 영역을 채우고 내부 스크롤을 소유합니다. 기본 스크롤 뷰포트 안에 두 번째 인터랙티브 Flickable 을 넣지 않도록 하세요.

## API

|속성|기본값/의미|
| --- | --- |
| `presentation` |`Sheet.Automatic`; `Theme.mobileTarget`에서 확인됩니다. `Sheet.Mobile` 및 `Sheet.Desktop`는 명시적 미리보기를 지원합니다.|
| `detent` |`Sheet.Fit`; `Sheet.Medium`는 모바일 뷰포트 높이의 57% 및 `Sheet.Large` 90%를 사용합니다.|
| `cornerRadius` |모바일 24/데스크톱 16 논리 픽셀; 발신자는 측정된 장치 반경을 제공합니다.|
| `resolvedCornerRadius` |삭제되었으며 형상-한계가 설정된 반경이 실제로 렌더링되었습니다.|
| `title`, `description` |헤더 텍스트입니다.|
| `showHeader`, `showDescription`, `showCloseButton` |`true`; 가까운 목표는 44 × 44입니다.|
| `showGrabber` |모바일 프레젠테이션을 따릅니다. 이 영역을 아래로 드래그하면 닫힐 수 있습니다.|
| `preferredWidth`, `preferredHeight` |데스크탑 560 × 420; 뷰포트 및 `desktopMargin`(24)에 의해 제한됩니다.|
| `topSafeInset`, `bottomSafeInset` |실시간 `WindowSafeAreaObserver` 측정; 내장된 미리보기를 재정의합니다.|
| `scrollContent` |`true`; false를 사용하면 콘텐츠 보기가 자체 스크롤 및 고정 작업을 수행할 수 있습니다.|
| `dismissOnBackground`, `dismissOnEscape`, `dismissOnDrag` |`true`; 프로그래밍 방식의 `close()`는 항상 사용 가능한 상태로 유지됩니다.|
| `animationDuration` |모바일 320 ms / 데스크탑 200 ms; 모션 감소 호스트 또는 테스트의 경우 0으로 설정합니다.|
| `contentComponent`, `loadedContent`, `content` |구성요소 입력, 인스턴스화된 보기 및 인라인 데이터 슬롯.|

표준 `Popup` 수명 주기를 사용하세요: `open()`, `close()`, `visible`, `opened`, `aboutToShow`, `aboutToHide`, `closed` 입니다. 이는 기존 `LV.Modal.open` 부울 값과 다릅니다. 모달 포커스 포함, 오버레이 스택킹, 바깥쪽 클릭 차단, Escape 처리, 포커스 반환은 Qt 퀉 컨트롤을 사용합니다. `modal` / `dim` 는 내장 카탈로그 시연용으로만 오버라이드합니다. 콘텐츠 액션과 저장되지 않은 변경 정책은 애플리케이션 책임으로 남아 있습니다.

모바일 높이는 사용 가능한 상단 안전 영역에 따라 한계가 설정된입니다. 하단 안전 영역은 시트 내부에 예약되어 있습니다. 데스크탑은 상위 크기가 조정될 때 중앙에 유지됩니다. 시트는 시뮬레이션된 OS 홈 표시기를 그리지 않습니다.

<a id="dependencies-and-verification"></a>

## 종속성 및 확인

기존 Qt Quick Controls `Popup`, Flickable, LVRS 레이블/버튼/토큰 및 `WindowSafeAreaObserver` 를 사용합니다. 추가 라이브러리, 패키지, 라이선스 또는 배포 의존성이 도입되지 않습니다. 유지되는 Qt 구현에서 상호작용을 유지하면 별도의 모달 입력 스택을 피합니다. Qt 참조: [Popup](https://doc.qt.io/qt-6.8/qml-qtquick-controls-popup.html).

`LVRSTests_sheet`는 측정된 반경 변경, 잘못된/0/과대 크기 입력, 반응형 배치, 맞춤/중간/대형 크기 조정, 구성 요소 및 인라인 콘텐츠, 한계가 설정된 스크롤, 모달 입력 차단, 해제, 포커스 반환 및 시각적 카탈로그 예를 다룹니다. 저장소 루트에서 빌드하고 실행합니다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

시각적 카탈로그 → 표면 → 시트는 편집 가능한 예시 반경과 함께 공유, 내보내기 및 폴더 선택 예를 제공합니다. 데스크탑 테스트는 QML 동작을 검증합니다. 실제 장치 측정 및 네이티브 물리적 화면 모양에는 소비자 애플리케이션의 장치 통합이 필요합니다.

5 렌더링된 카탈로그 예제를 저장하려면 `build/tests/LVRSTests_sheet gallery_loads` 를 실행하여 `LVRS_SHEET_CAPTURE_DIR` 를 `build/` 아래 경로로 설정합니다. 네이티브 macOS 검증은 `QT_QPA_PLATFORM=cocoa` 와 `QSG_RHI_BACKEND=metal` 를 사용하며, `DYLD_LIBRARY_PATH`, `QML_IMPORT_PATH` 및 `QML2_IMPORT_PATH` 를 현재 `build/` 를 가리키도록 유지하여 오래된 설치된 SDK 를 로드하는 것을 피합니다.

<a id="shared-motion"></a>

## 공유 모션

모바일이 바닥에서 미끄러지고 멈춤쇠로 되돌아옵니다. 데스크탑이 부드럽게 축소됩니다. 드래그는 직접적으로 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

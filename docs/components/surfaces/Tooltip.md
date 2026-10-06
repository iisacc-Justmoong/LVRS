<a id="tooltip"></a>

# 툴팁

위치: `src/qml/components/surfaces/Tooltip.qml`

`LV.Tooltip` 는 재사용 가능한 QML `Component`, 인라인 항목 또는 일반 `text` 를 허용하는 비모달 스피치 버블 툴팁입니다. Qt Quick Controls `ToolTip` 를 확장하며 추가 제 3 자 의존성이 필요하지 않습니다. Qt Quick Shapes 는 둥근 몸체와 꼬리를 하나의 벡터 윤곽선으로 그립니다.

<a id="usage"></a>

## 사용법

```qml
LV.LabelButton { id: helpButton; text: "Help" }

Component {
    id: helpView
    Item {
        implicitWidth: 220
        implicitHeight: column.implicitHeight
        Column {
            id: column
            width: parent.width
            spacing: LV.Theme.gap8
            LV.Label { style: header2; text: "Version history" }
            LV.Label {
                width: parent.width
                style: body
                text: "Restore an earlier version without losing your current work."
                wrapMode: Text.WordWrap
                sizeToContentHeight: true
            }
        }
    }
}

LV.Tooltip {
    target: helpButton
    contentComponent: helpView
}
```

콘텐츠 루트의 `implicitWidth` 와 `implicitHeight` 를 설정하거나 (또는 Column 과 같이 이를 제공하는 레이아웃을 사용) 합니다. 호스트는 사용 가능한 너비를 할당하고 긴 콘텐츠를 수직 Flickable 로 처리합니다. 제공된 뷰, 예를 들어 ListView 가 스크롤을 소유하고 사용 가능한 높이를 채워야 할 때 `scrollContent: false` 를 사용합니다. `loadedContent` 는 로더가 생성한 인스턴스를 노출하며, `contentComponent` 를 교체하면 해당 인스턴스를 교체합니다. 컴포넌트가 제공되지 않을 때 인라인 콘텐츠를 사용하며, 둘 다 비어 있을 때 `text` 는 대체 경로 입니다.

<a id="origin-and-placement"></a>

## 원산지와 배치

- `target`: 원점과 자동 상호 작용을 소유하는 항목입니다. 수동 프리젠테이션의 경우 선택 사항입니다.
- `anchorPoint`: 대상-로컬 논리 좌표의 한 점으로, 기본값은 중심입니다. 대상이 없으면 좌표는 창 오버레이를 기준으로 합니다.
- `openAt(item, point, component)`: 원점과 선택적으로 구성 요소를 설정한 다음 엽니다. 자체 수명을 관리하는 포인터/캔버스 미리보기에 대해 `automatic: false; delay: 0`를 설정합니다.
- `preferredPlacement`: `Automatic`, `Above`, `Below`, `LeftSide` 또는 `RightSide`. 선호도가 맞지 않으면 뒤로 물러납니다. 측면 이름은 Qt Popup의 관련 없는 `Left`/`Right` 변환 원본 상수를 방지합니다.
- `maximumWidth` / `maximumHeight`: 본체 크기 제한, 기본적으로 `320` 모두.
- `availableRect`: 허용된 오버레이 공간 뷰포트, 기본값은 호스트 창의 안전 영역입니다. 뷰포트는 현재 디스플레이와 교차됩니다. 사용자 정의 직사각형은 버블을 프레임으로 제한할 수 있습니다.
- `edgeMargin`: `8`; `contentPadding`: `12`; `cornerRadius`: `12`; `tailLength`: `10`; `tailWidth`: `16`.
- `surfaceColor`, `borderColor`: LVRS 테마 색상.

적합한 방향이 우선합니다. 자연스러운 몸체에 하나도 맞지 않으면 가장 큰 몸체 영역을 보여주는 방향이 우선하며 콘텐츠 뷰포트 는 제한됩니다. 바디가 가장자리에서 멀어지지만 꼬리의 끝은 정확한 원점에 머무르며, 그 베이스는 바디의 직선 가장자리를 따라 미끄러집니다. 부모 변환, 스크롤, 대상 이동, 및 창 크기 조절은 가시화되는 동안 추적됩니다. 숨겨진, 비활성화된, 분리된, 또는 뷰포트 밖의 타겟은 툴팁을 닫으며, 스크롤 조상에서 잘린 기원이 포함됩니다.

`resolvedPlacement`, `resolvedAnchorPoint`, `bodyRect`, `tailPoint`, `availableContentWidth` 및 `availableContentHeight`는 확인된 형상을 노출합니다. 앵커는 오버레이 좌표에 있습니다. 본문/꼬리 좌표는 팝업에 로컬입니다. 데스크톱과 모바일의 모든 크기는 논리적 픽셀입니다.

<a id="standard-interaction"></a>

## 표준 상호작용

`automatic: true` 는 호버, 키보드 포커스 또는 타겟을 누르고 있을 때 표시됩니다. 네이티브 `delay` 은 `500ms` 로 기본값이며, 만료되기 전에 해제/나아가면 표시가 취소됩니다. `timeout` 은 `5000ms` 로 기본값이며, `-1` 는 해제될 때까지 열려 있습니다. 같은 트리거가 활성화된 동안 타임아웃이나 Escape 키는 툴팁을 즉시 다시 열지 않습니다.

트리거를 나가면 `hideDelay` ( `120ms` ) 후에 닫혀서 포인터가 풍부한 툴팁으로 이동할 수 있습니다. 바디는 인터랙티브 컨트롤을 호스팅할 수 있습니다. 툴팁 표시는 애플리케이션을 어둡게 하거나, 바깥쪽 상호작용을 차단하거나, 키보드 포커스를 가져오지 않습니다. Escape 키와 바깥쪽 클릭은 이를 해제합니다. `animationDuration` ( `120ms` 또는 `0` 은 애니메이션 없음) 짧은 페이드를 제어합니다. `open()`, `close()` 및 상속된 ToolTip 지연/타임아웃 API 는 계속 사용 가능하며, `automatic: false` 는 애플리케이션 주도 가시성을 허용합니다.

이는 로컬 `LV.Tooltip` 인스턴스이며 Qt의 공유 `ToolTip` 연결된 속성을 대체하지 않습니다. 임의 콘텐츠의 경우 `contentComponent` 또는 인라인 항목을 사용하세요.

<a id="examples-and-validation"></a>

## 예시 및 검증

시각적 카탈로그 → 표면 → 도구 설명에는 4개의 프레임 모서리 예, 이동 가능한 원점, LVRS 컨트롤이 포함된 풍부한 상태 보기 및 자동 텍스트 도움말이 포함되어 있습니다.

`ctest --test-dir build -R LVRSTests_tooltip --output-on-failure` 는 방향 대체 경로, 엣지 이동 및 렌더링된 꼬리, 컴포넌트 대체, 인라인 콘텐츠, 제한된 스크롤, 변환/이동 타겟, 리사이즈, 호버 지연, 타임아웃, Escape, 터치 누름, 그리고 비모달 입력을 확인합니다. `LVRS_TOOLTIP_CAPTURE_DIR` 을 `build/` 하위의 디렉토리로 설정하여 갤러리 캡처를 수행하고, 네이티브 렌더러를 사용하여 벡터 윤곽을 검사합니다.

참고 자료: [Qt ToolTip](https://doc.qt.io/qt-6.8/qml-qtquick-controls-tooltip.html), [Qt 빠른 모양](https://doc.qt.io/qt-6.8/qml-qtquick-shapes-shape.html).

<a id="glass-25-background"></a>

## 유리 25 배경

기존 바디 및 꼬리 윤곽은 이제 `PanelMaterial`: 25% 중성 톤, 16px 확산, 그리고 앱 기본 원형 색상을 40% / 11% 강도로 가립니다. `surfaceColor` 는 기본값으로 `Theme.materialTint` 이며, `surfaceOpacity` 는 기본값으로 0.25입니다. `primaryColor`, `backdropSource` 및 `backdropBackground` 는 커스텀 호스트에 대해 공급될 수 있습니다. 배치, 콘텐츠 대체, 스크롤 및 해제 동작은 변경되지 않았습니다. [재료](Materials.md)를 참조하세요.

<a id="shared-motion"></a>

## 공유 모션

배치와 꼬리가 원점을 계속 추적하는 동안 기포는 94%에서 자라서 반동합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

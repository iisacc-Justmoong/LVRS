<a id="interaction-and-motion"></a>

# 상호작용과 모션

LVRS 는 구성 요소 계열을 위해 하나의 동작 정책을 제공합니다. 구현은 기존 Qt 6.8 Quick 애니메이션 엔진을 사용하며, 추가적인 런타임 런타임, 라이선스 또는 패키지 의존성이 없습니다. Qt 는 이미 중단 가능한 속성 동작과 NumberAnimation 를 제공하므로, 별도의 애니메이션 라이브러리는 설치된 프레임워크를 중복하게 됩니다. [Qt 동작](https://doc.qt.io/qt-6/qml-qtquick-behavior.html) 와 [Qt NumberAnimation](https://doc.qt.io/qt-6/qml-qtquick-numberanimation.html)를 참조하세요.

<a id="global-policy"></a>

## 글로벌 정책

```qml
import LVRS as LV
// 이 QML 엔진 안에서 공유되며 애플리케이션 루트에서 설정한다.
Component.onCompleted: {
    LV.Motion.speed = 1.0
    LV.Motion.reducedMotion = false
}
```

|설정/토큰|기본값|의미|
| --- | --- | --- |
|활성화됨|참|애니메이션 프레젠테이션 활성화|
| reducedMotion |false|애플리케이션 제어 접근성 기본 설정. 자동 OS 감지가 주장되지 않습니다.|
|속도| 1 |재생 속도. 유한한 값은 0.1–4 범위로 제한하며 잘못된 값에는 1를 사용한다.|
| pressDuration |90 ms|즉각적인 느낌의 압축|
| hoverDuration |160 ms|호버 및 포커스 항목|
| releaseDuration |360 ms|1개의 탄력적 복귀|
| buttonReleaseDuration |180 ms|버튼 / MenuItem / ListItem / HierarchyItem 릴리스 리바운드|
| surfaceDuration |420 ms|더 큰 팝업/페이지 표시|
| exitDuration |150 ms|프롬프트 해제|
| colorDuration |130 ms|의미 체계 채우기 혼합|
|오버슈트| 1.45 |공유 OutBack 곡선|

`Motion.duration(ms)` 는 속도와 전역 정책을 적용합니다. 숫자 및 색상 동작은 비활성화될 때 비행 중인 애니메이션을 해결합니다. `motionEnabled` 를 노출하는 컨트롤은 로컬로 옵트아웃할 수 있습니다. ToggleSwitch `transitionDuration`, 시트/툴팁 `animationDuration`, ContextMenu 반동 설정 및 PageRouter 제스처 해결 지속 시간은 명시적 오버라이드로 유지됩니다.

<a id="interaction-contracts"></a>

## 상호작용 계약

- AbstractButton 는 버튼, 카드, 메뉴, 목록 및 툴바 하위 클래스에 포인터, 터치 및 키보드 압축을 제공합니다. 변환은 콘텐츠 및 배경 슬롯에 연결되어 루트 타겟, 레이아웃 너비, 높이 및 암시적 크기를 보존합니다. 시각적 이동을 큰 타겟의 경우 제한합니다. 키보드 포커스 링은 입력을 가로채지 않습니다.
- PushButton / DropdownButton 와 그 라벨/아이콘 프리셋, MenuItem / ContextMenuItem, 모든 17 ListItem 유형 및 HierarchyItem 는 명시적인 `released()` 응답을 사용합니다: 첫 번째 누름 프레임 전에 완료된 탭을 위한 작은 최소 압축과 함께 하나의 180ms OutBack 반환입니다. 릴리스는 기존 색상 정책을 통해 기존 호버/기본 채우기를 복원하며, 영구적인 릴리스 색상이나 테두리를 추가하지 않습니다. 각 구성 요소는 `release` 를 포함한 일시적인 입력 단계를 소유하며, 이는 [구성 요소 인스턴스 상태](instance-states.md)에 설명되어 있습니다. 드래그 아웃은 리바운드가 없이 취소됩니다. 다시 누르는 것은 반환을 중단하며, 비활성화/움직임 감소/로컬 옵트아웃은 변형을 즉시 지웁니다. 키보드 스페이스/엔터와 터치 릴리스는 동일한 이벤트 경로를 사용합니다. 탭 초점이 선택과 독립적으로 누름/릴리스 동안 가시성을 유지합니다. 메뉴/목록 중립 호버는 surfaceAlt 를 사용하며, 계층 구조는 기존 fantom 호버를 유지합니다.
- 스테퍼와 ComboBox는 소형 합성에 동일한 응답을 적용합니다. 신호 및 삽입된 콜백은 애니메이션 완료와 관계없이 활성화 시 실행됩니다.
- CheckBox 마크와 RadioButton 도트가 선택된 상태로 성장합니다. ToggleSwitch는 손잡이 스쿼시, 이동 스트레치, 네이티브 드래그 및 끝점 리바운드를 유지합니다.
- 슬라이더 값과 포인터 매핑은 드래그 동안 즉시 유지됩니다. 프로그래밍 방식/키보드 변경은 트랙에 클램핑된 표시된 위치를 애니메이션화하며, 가시적인 엄지는 누름에 반응합니다. ProgressBar 는 필드를 트랙에 클램핑하고 실제 수치 모델을 유지하면서 표시된 진행을 애니메이션화합니다.
- AbstractInputBar / InputField, TextEditor 및 CodeEditor는 초점 윤곽선에 애니메이션을 적용합니다. 텍스트, 캐럿, 선택 및 IME 좌표는 크기가 조정되거나 지연되지 않습니다. ColorPicker는 이러한 컨트롤과 슬라이더를 구성합니다.
- TableCellItem는 선택 항목/현재 색조를 혼합합니다. 테이블 TableHeader 및 TableRow는 대표자의 입력 피드백을 상속합니다. 형상 크기 조정은 직접적입니다.
- ContextMenu, Popover 및 Tooltip은 진입 시 리바운드되고 종료 시 페이드됩니다. 시트는 모바일에서는 하단 가장자리 번역을 사용하고 데스크톱에서는 적당한 규모를 사용합니다. Alert 및 Modal은 종료가 완료될 때까지 닫는 시각적 레이어를 유지합니다.
- 확장 시 계층 공개 화살표가 회전합니다. 스크롤 리바운드는 동일한 곡선을 사용합니다. 끌기 및 모델 순서는 동기식으로 유지됩니다. PageRouter는 소규모 전환을 사용하므로 제스처 소유 페이지 x와 경쟁하지 않습니다. 네비게이터와 전환 도우미가 이를 전달합니다.
- HStack / VStack은 명시적인 간격 변경에 애니메이션을 적용하고, Spacer는 최소 길이에 애니메이션을 적용하며, ZStack / Label / MenuDivider는 명시적인 불투명도 변경에 애니메이션을 적용합니다. 레이아웃 관리 하위 형상에는 경쟁 동작이 제공되지 않습니다.
- MaterialSurface, PanelMaterial, WindowMaterial 및 AppCard 블렌드 저작 색조 변경. 네이티브 창은 플랫폼 이동/크기 조정 상호 작용을 유지합니다. 서랍과 콘텐츠 컨트롤은 LVRS 모션을 사용합니다.
- EventListener, 가드, 테마, 메소드 디스패치 및 지오메트리 관찰자는 비시각적입니다. 독립적인 입력 표면을 제조하는 대신 상태/이벤트를 눈에 보이는 소비자에게 전달합니다.

<a id="reusable-primitives"></a>

## 재사용 가능한 기본 요소

```qml
Rectangle {
    id: tile
    width: 160; height: 80
    property bool expanded: false
    color: expanded ? LV.Theme.primary : LV.Theme.panelBackground08
    LV.StateColorBehavior on color {}
    rotation: expanded ? 8 : 0
    LV.SpringBehavior on rotation {}
    transform: LV.InteractionMotion {
        target: tile
        pressed: pointer.pressed
        hovered: pointer.containsMouse
    }
    MouseArea { id: pointer; anchors.fill: parent; hoverEnabled: true; onClicked: tile.expanded = !tile.expanded }
    LV.FocusRing { anchors.fill: parent; active: tile.activeFocus }
}
```

불투명도에는 단조 완화(OutCubic)를 사용하고 오버슈트가 부적절한 경우에는 한계가 설정된 데이터 지오메트리를 사용합니다. 지속적인 입력을 위해서는 네이티브 직접 추적을 선호합니다. 레이아웃 소유 하위 x/y/너비/높이 또는 백엔드 모델 값에 애니메이션을 추가하지 마세요.

`InteractionMotion.releaseOnSignal` 는 기본적으로 false 로 설정되어 다른 소비자의 기존 다운 상태 반환을 유지합니다. true 일 때, `playRelease()` 를 실제 릴리스 이벤트에서 호출하며, `stopRelease()` 는 중단된 펄스를 지웁니다. `deformationProgress` 는 시각적 변환에 의해 사용된 실제 압축/리바운드를 보고하며, `pressProgress` 는 기존 누름 상태 계약을 유지합니다. 작성된 버튼, 메뉴 및 목록 행 계열은 상속된 `AbstractButton.releaseOnSignal` 속성을 통해 이 정책을 활성화합니다.

`InteractionMotion.releasing`는 명시적 해제 애니메이션이 실행되는 동안에만 true입니다. AbstractButton가 소유한 `interaction` 상태가 이를 소비합니다. 동작을 줄이고 비활성화하고 다시 누르면 시각적 반응과 함께 위상이 고정됩니다.

<a id="visualcatalog-verification"></a>

## VisualCatalog 검증

컴포넌트 스튜디오는 모든 배포된 QML 파일과 WindowSafeAreaObserver ( 88 엔트리) 를 색인화합니다. 검색은 유형, 요약 및 소스 경로를 일치시킵니다. 각 엔트리는 `CatalogMotion.js` 에 명시된 레시피, 실제 소비자 플레이그라운드, 응답 설명, 검사 가이드 및 사용/소스 참조를 포함합니다. 내부 렌더러는 지원 유형으로 표시됩니다. 공유 패밀리 미리보기는 구성과 변형을 보여줍니다.

전역 1× / 0.5× / 0.25컨트롤을 사용하여 타이밍을 검사하고, 즉시 상태에 대해 Reduce motion 을 적용하며, 데모 상태를 재현하기 위해 Reset preview 를 사용하세요.

```bash
cmake -S . -B build -DLVRS_BUILD_TESTS=ON -DLVRS_BUILD_EXAMPLES=ON
cmake --build build -j 8
env DYLD_LIBRARY_PATH="$PWD/build" QML_IMPORT_PATH="$PWD/build" QML2_IMPORT_PATH="$PWD/build" ctest --test-dir build --output-on-failure
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

`LVRSTests_motion` 는 모든 19 버튼 계열, 포인터/키보드/터치 입력, 중단된 리바운드, 비활성화/로컬 옵트아웃, 애니메이션 중 감소된 운동, 안정적인 레이아웃 및 직접 슬라이더 드래깅을 실행합니다. `LVRSTests_catalog` 는 모든 QML 파일에 레시피가 있는지 확인하고, 모든 인덱싱된 프레임을 로드하며, 검색/리셋을 확인합니다. 기존 테스트는 컴포넌트 의미론과 내장 소스 리소스 동등성을 다룹니다. 네이티브 Metal 시각적 검사는 오프스크린 테스트와 별도의 확인입니다. 설치된 패키지는 이 빌드와 별도로 유효성을 검사해야 합니다.

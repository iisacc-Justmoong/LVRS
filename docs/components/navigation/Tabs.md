# Tabs

## Purpose

`Tab`·`TabBar`는 데스크톱 콘텐츠 전환을, `MobileTab`·`MobileTabBar`는 iOS 및 Android 형태의 하단 목적지 탐색을 제공한다. [Figma Tabs](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1035-6)의 6개 계열·40개 변형을 네 가지 QML 타입의 속성 조합으로 표현한다.

기존 Qt Quick Controls와 LVRS AbstractButton·MaterialSurface·FocusRing·모션·색상 토큰을 재사용한다. 추가 의존성은 없다. 모바일은 LVRS가 그리는 QML 컨트롤이며 UIKit UITabBar나 Android Views를 직접 삽입하는 API가 아니다. iOS 시스템 Liquid Glass의 OS별 동작을 제공한다고 보장하지 않는다.

## API

| 타입 | 주요 속성 |
| --- | --- |
| Tab | `text`, `selected`, `tabStyle`(Underline/Surface), `displayState`, `iconName`/`iconSource`, `showIcon`, `badge`, `showBadge` |
| TabBar | `model`, `currentIndex`, `autoSelect`, `widthPolicy`(Equal/Content/Scrollable), `tabStyle`, `delegate`, `motionEnabled` |
| MobileTab | Tab의 입력·선택 계약과 `platformStyle`(Automatic/IOS/Android), `showLabel`, `badgeDot` |
| MobileTabBar | TabBar의 모델·선택 계약과 `platformStyle`, `presentation`(Expanded/Search/Minimized), `bottomSafeInset`, `backdropSource` |

`Tab.displayState`는 StateAutomatic(실제 입력), StateDefault, StateHover, StatePressed, StateSelected, StateFocus, StateDisabled 순이다. 기본값은 StateAutomatic이다. 전시·테스트용 강제 상태와 실제 `selected` 값은 별도이다.

모델은 문자열 또는 `{text, iconName, badge, badgeDot, enabled, objectName, labelObjectName, accessibleName}` 객체의 배열이다. 모델을 갱신할 때 새 배열을 대입한다. `itemAt(index)`는 해당 컨트롤을 반환하며 `count`는 모델 항목 수이다. `activated(index)`는 유효하고 활성화된 탭의 실제 입력에만 발생한다. `activate(index)`도 같은 검증을 적용한다.

`autoSelect: true`는 입력 시 currentIndex를 갱신한다. 외부 라우터나 팝업을 함께 사용하는 앱은 `autoSelect: false`와 바인딩된 currentIndex를 사용한다. 이때 activated 처리기가 화면을 바꾸거나 팝업을 열며, 탭바는 외부 바인딩을 덮어쓰지 않는다. 배열 변경 후 자동 선택 모드에서는 유효한 기존 인덱스를 유지하거나 첫 활성 항목으로 이동한다.

## Usage

```qml
LV.TabBar {
    model: [{text: "Overview"}, {text: "Activity", badge: "8"}]
    widthPolicy: LV.TabBar.Equal
    onActivated: index => stack.currentIndex = index
}

LV.MobileTabBar {
    model: [
        {text: "Home", iconName: "home"},
        {text: "Library", iconName: "nodesfolder"},
        {text: "Settings", iconName: "generalsettings"}
    ]
    currentIndex: controller.currentIndex
    autoSelect: false
    // 외부 창이 이미 안전 영역을 적용했으면 0으로 유지한다.
    bottomSafeInset: 0
    onActivated: index => controller.currentIndex = index
}
```

## How It Works

- Tab은 32 px, TabBar는 40 px이며 선택 밑줄은 2 px이다. Content는 자연 너비·최소 120 px, Equal은 균등 분배, Scrollable은 최소 192 px 항목과 가로 스크롤을 사용한다.
- 모바일 자동 스타일은 Android에서 Android, 나머지 환경에서 iOS 프리뷰를 선택한다. 명시적인 platformStyle로 다른 플랫폼을 미리 볼 수 있다.
- iOS는 54 px 항목과 위 16 px, 좌우 22 px 간격의 떠 있는 캡슐을 사용한다. 기본 높이는 안전 영역을 제외한 70 px이다. Figma의 95 px 프레임 중 아래 25 px는 고정 OS 인셋으로 사용하지 않는다.
- iOS 형태의 레이블은 `FontPolicy.systemFamily`를 통해 Qt가 보고하는 시스템 글꼴을 사용한다. Android 형태는 사용 가능한 Roboto를 사용하며 호스트에 없으면 기존 FontPolicy의 대체 글꼴을 쓴다. 운영체제의 숨겨진 글꼴 이름을 하드코딩하지 않는다.
- iOS Search는 모델의 마지막 항목을 별도 검색 원형 버튼으로 배치한다. 호출자가 마지막 항목의 검색 아이콘·이름을 제공한다. Minimized는 선택 항목 하나를 48 px 터치 영역으로 표시하며 탭하면 `expandRequested()`를 발생시킨다. 호출자가 presentation을 Expanded로 변경한다.
- Android는 높이 64 px, 아이콘 24 px, 선택 표시 56×32 px를 사용한다. Search/Minimized 속성은 Android에서 일반 배치로 처리한다. 다크 색상은 Figma Material 3 Blue dark에서 확인한 Theme.mobileTabAndroid* 토큰을 사용한다.
- 모바일 목적지는 3–5개와 320 px 이상의 너비를 권장한다. 짧은 가시 레이블과 별도의 accessibleName을 사용할 수 있다. 라벨은 줄임표로 영역을 보호한다.
- 키보드 좌우·Home/End는 활성 탭으로 포커스를 이동하고, Enter/Space는 선택한다. 포커스를 이동할 때 스크롤 영역 안으로 항목을 노출한다. 마우스·터치·접근성 press는 동일한 activated 경로를 사용한다.
- PageTabList/PageTab 역할과 선택 상태를 노출한다. 이미지·레이블 장식은 중복 읽기를 막기 위해 접근성 트리에서 제외한다.
- Safe area는 부모 창 또는 bottomSafeInset 한 곳에서만 적용한다. 회전과 너비 변경은 선택 값과 외부 콘텐츠 인스턴스를 교체하지 않는다.

## Verification

`ctest --test-dir build -R 'LVRSTests_(tabs|catalog)' --output-on-failure`로 상태 조합, 비활성 항목 건너뛰기, 키보드·터치, 외부 선택 바인딩, 스크롤 포커스, 모바일 3/4/5개 항목과 320/360/412 px 배치, 안전 영역 및 카탈로그를 검사한다. VisualCatalog의 Tabs에서 실제 컨트롤을 조작할 수 있다. OS 실기기 검증과 호스트 Qt 프리뷰 검증은 구분한다.

[Qt Controls](https://doc.qt.io/qt-6/qtquickcontrols-index.html)와 [접근성 API](https://doc.qt.io/qt-6/qml-qtquick-accessible.html)를 기존 프로젝트 의존성 안에서 사용한다.

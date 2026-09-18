# MobileNavigationBar / MobileNavigationTab

LVRS 스타일의 모바일 시스템 탐색 컴포넌트이다. 기존 `MobileTabBar`의 네이티브 형태와 별도로 제공한다. 디자인을 먼저 [Figma 보드](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1072-1243)에 작성한 다음 QML로 구현하였다. [탭바 20개 변형](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1074-2064)은 플랫폼 2종 × 탭 수 1–5 × 레이블 유무를 다룬다. 모든 변형의 `Search visible` 속성은 기본 꺼짐이며 [검색 유무 비교 예제](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1083-1805)에서 두 상태를 확인할 수 있다. [이동 프로토타입](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1076-1982)에서 Home / Library를 선택할 수 있다.

```qml
LV.MobileNavigationBar {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    model: [
        { iconName: "home-1", accessibleName: "Home" },
        { iconName: "nodesfolder", accessibleName: "Library" },
        { iconName: "generalsettings", text: "Settings" }
    ]
    search: { accessibleName: "Search library" }
    autoSelect: false
    currentIndex: controller.currentIndex
    onActivated: index => controller.currentIndex = index
    onSearchRequested: searchPanel.open()
}
```

## 입력과 선택

`model`은 `{ iconName, iconSource, text, accessibleName, enabled }` 배열이다. `text`를 생략하면 아이콘만 표시한다. 텍스트가 없는 목적지에는 의미 있는 `accessibleName`을 제공한다. 레이블은 Pretendard Caption 11이며 긴 문자열은 말줄임 처리한다. `itemAt(index)`, `count`, `currentIndex`, `autoSelect`, `activated(index)`는 기존 탭바와 같은 계약이다. 배열을 변경할 때는 새 배열을 대입한다. 자동 선택에서는 유효한 선택을 유지하거나 첫 활성 탭을 선택한다. 외부 제어 모드에서는 바인딩된 인덱스를 변경하지 않는다.

`search`는 선택 인자이며 기본값은 `null`이다. 생략하거나 `null`·`undefined`를 지정하면 검색 버튼을 표시하지 않는다. `search: ({})`로 기본 검색 버튼을 추가하거나 `{ iconName, iconSource, text, accessibleName, enabled, visible }` 객체로 구성한다. `visible: false`는 구성을 유지하면서 검색을 숨긴다. 읽기 전용 `searchVisible`은 실제 표시 여부를 제공한다.

검색이 있는 경우에만 독립 버튼을 물리적 최우측에 표시한다. 기본 아이콘은 `inputFieldSearch`이며 탭 수·선택 인덱스에 포함되지 않는다. 입력 시 `searchRequested()`만 발생한다. 검색의 가시 텍스트 역시 생략 가능하다. `search: { enabled: false }`는 보이는 비활성 버튼이다. 숨긴 검색은 너비와 간격을 예약하지 않으며 접근성 및 키보드 포커스 대상에서도 제외된다. 표시 여부를 바꾸어도 탭 선택은 유지한다.

`MobileNavigationTab`은 단독 사용 가능한 `Tab` 파생 컴포넌트이다. `showLabel`은 기본적으로 `text.length > 0`이며 `drawSelection`으로 개별 선택 배경을 제어한다. 탭바 안에서는 개별 배경을 끄고 탭바가 소유한 하나의 배경을 이동한다.

## 크기와 기기 곡률

| 항목 | 값 |
| --- | --- |
| 탭바·검색 표면 높이 | 56 |
| 탭 자연 너비 / 최소 너비 | 56 / 48 |
| 탭 터치 높이 / 아이콘 | 48 / 24 |
| 표면 안쪽 / 바깥쪽 여백 | 4 / 16 |
| 검색과 탭 사이 최소 간격 | 12 |
| 기본 전체 높이 | 88 + bottomSafeInset |
| 표면 / 선택 배경 / 테두리 | panelBackground03 / panelBackground10 / panelBackground08 |
| 기본 곡률 | iOS 28 / Android 12 |

`platformStyle`은 `MobileTab.Automatic`, `MobileTab.IOS`, `MobileTab.Android`를 사용한다. `deviceCornerRadius >= 0`이면 기기의 명시적 논리 픽셀 값을 우선 사용하고 표면 높이의 절반까지 제한한다. 값 0도 유효한 각진 기기 사양이다. Android 12 이상에서는 [WindowInsets.getRoundedCorner](https://developer.android.com/reference/android/view/WindowInsets#getRoundedCorner(int))가 제공하는 실제 하단 곡률을 `WindowSafeAreaObserver.bottomCornerRadius`로 읽는다. 창의 회전·화면 변경 시 다시 확인한다. iOS 및 정보를 제공하지 않는 환경은 플랫폼 기본값을 사용한다. iOS의 물리적 곡률을 추측하는 기종 목록이나 비공개 API를 사용하지 않는다. 특정 기기의 정확한 값이 필요한 경우 `deviceCornerRadius`로 주입한다.

탭 묶음은 물리적 왼쪽에서 축소되고 검색을 지정한 경우에만 물리적 오른쪽에 고정된다. RTL에서도 이 위치 계약은 같다. 320 px처럼 좁은 화면에서 5개 탭과 검색을 함께 배치하면 탭의 48 px 터치 너비를 유지하고 탭 묶음만 가로 스크롤한다. 검색을 생략하면 같은 화면에서 5개 탭이 자연 너비 56으로 모두 들어간다. 검색 버튼은 스크롤하지 않는다. 선택·키보드 포커스 이동은 해당 탭을 가시 영역으로 이동한다. 0개 모델에서는 구성한 검색만 남으며 검색도 생략하면 버튼이 없다. `bottomSafeInset`은 부모 창이 안전 영역을 이미 적용하지 않은 경우에만 지정한다.

## 이동과 접근성

선택 배경은 140ms 동안 앞쪽 가장자리가 거리의 88%, 뒤쪽이 15% 이동하며 높이가 48에서 42로 줄어든다. 다음 280ms 동안 새 탭의 위치·너비·높이로 부드럽게 복원된다. 방향 전환과 빠른 연속 선택은 현재 화면의 배경 위치에서 새 대상으로 이어진다. 인덱스 변경이 없는 크기·모델 변경은 즉시 재배치한다. `motionEnabled`, `Motion.enabled`, `Motion.reducedMotion`으로 애니메이션을 끄면 진행 중 동작도 최신 선택 위치에 즉시 정착한다. `Motion.speed`는 전체 시간을 조절한다.

탭에는 `PageTab` 역할과 선택 상태를, 검색에는 독립 `Button` 역할을 제공한다. 선택 아이콘은 primary, 작은 선택 레이블은 대비를 확보한 TitleHeader 색상이다. 방향키와 Home/End는 비활성 항목을 건너뛰며 포커스를 이동한다. Enter/Space·터치·마우스·접근성 press는 동일한 입력 경로를 이용한다. 포커스 이동만으로 선택을 변경하지 않는다.

## 검증

`cmake --build build` 후 `ctest --test-dir build --output-on-failure`를 실행한다. `LVRSTests_mobile_navigation`은 1–5개 탭 배치, 독립 검색, 검색 생략·표시 전환 시 공간 회수와 포커스 제외, 곡률 재정의, 선택 제어, 접근성, 좁은 화면, 이동 중 재선택 및 동작 줄이기를 검사한다. `LVRSTests_platform_integration`은 기기 관찰자의 미지원 값과 창 수명을 검사한다. `LVRS_MOBILE_NAVIGATION_CAPTURE_PATH`를 지정하여 `LVRSTests_mobile_navigation gallery_capture`를 실행하면 갤러리 화면을 저장한다. 호스트 렌더링 검증과 iOS/Android 실기기 검증은 구분한다. 이 변경은 재설치를 요구하거나 수행하지 않는다.

# 네이티브 Alert와 AlertButton

`lvrs::Alert`, `Widgets::alert`와 LVUI `Alert`은 원본
`src/qml/components/surfaces/Alert.qml`을 iiGUI 자체 C++ 위젯으로 조합한 구현이다.
`AlertButton`도 독립적인 C++ 타입과 AOT 예약어로 제공한다. 창·입력·가상 접근성 경계는
iiGUI, 앱 진입점·작업자·프레임·최종 리소스 부착은 iiSDKCore가 소유한다.
NSButton·NSPanel·NSPopover나 Qt 런타임을 사용하지 않는다.

## 원본 배치와 액션

카드 폭은 `max(0, min(창 폭 - 2×sidePadding, maxWidth, max(minWidth, preferredWidth)))`이다.
기본값은 24·500·280·500이며, 창이 좁으면 원본처럼 최소 폭 아래까지 줄어든다.
카드는 중앙에 놓이고 기본 반경은 `min(36, 짧은 변×0.4)`이다.
`.cylinder`는 짧은 변의 절반을 사용한다. 위 콘텐츠 패딩 46·아래 36, 아이콘과 텍스트 간격
28, 텍스트 좌우 패딩 24·제목/본문 간격 14를 적용한다. 제목은 26단위 Bold·줄 높이 26,
본문은 13단위 Medium·줄 높이 13이며 지정 폭에 맞춰 줄바꿈한다.

`buttonCount`는 2와 3만 명시적 개수이다. 그 외 정수는 3차 텍스트가 있으면 3개,
2차 텍스트가 있으면 2개, 나머지는 1개를 선택한다. 표시할 액션의 빈 텍스트는 `Button`이다.
`description`은 `message`, `button1Text/2Text/3Text`는 기본/2차/3차 텍스트의 별칭이다.
동일 속성과 별칭을 함께 지정하면 AOT 오류이다.

- 한 개: 좌우 24, 위 28·아래 32, 높이 56의 기본 액션이다.
- 두 개: 같은 여백에 간격 14, 왼쪽 2차·오른쪽 기본 액션이며 각각 높이 56이다.
- 세 개: 좌우 24·상하 18, 간격 12로 기본·2차·3차를 세로 배치하며 높이는 56·56·44이다.

일반 2차 액션에는 투명 배경과 `#596168` 테두리가 있고, 세 액션의 2차 글자는 기본적으로
원본 destructive 색상이다. `secondaryDestructive`로 명시할 수 있다. 2줄 본문·기본 제목과
아이콘에서 두 액션은 500×379, 세 액션은 500×479이다. 텍스트·아이콘 표시·개수 변경은
같은 위젯의 크기를 갱신하며 포커스 액션 역할도 가능한 범위에서 유지한다.

## 버튼 메서드와 수명

`button1Method/2Method/3Method`에는 `vm.method` 형태의 지연 C++ 메서드 참조를 지정한다.
생성 코드는 `const lvrs::ButtonEvent&`를 받는 메서드를 우선 선택하고, 해당 호출이 없으면
인자 없는 메서드를 호출한다. 이벤트에는 실제 소스 `Widget`과 `trigger == "clicked"`가 있다.
메서드를 실행한 다음 대응하는 `onPrimaryClicked/onSecondaryClicked/onTertiaryClicked`를 호출한다.
문자열 메서드 조회나 실행 시 LVUI 해석기를 넣지 않는다. `vm.method()`는 이 입력에서 오류이다.

액션은 자동으로 닫지 않는다. 소유자가 `root().close_popup()` 또는 `Alert::close()`를 호출한다.
`dismiss()`·`cancel_popup()`은 일반 닫기와 구분하여 `dismissed`를 한 번 통지한다.
배경 입력은 항상 차단하며 기본 외부 클릭과 Escape는 닫지 않는다. `dismissOnBackground`와
선택적인 네이티브 확장 `closeOnEscape`를 켰을 때만 해당 취소 경로를 사용한다.
Tab과 이전 포커스 복귀는 iiGUI 팝업 정책을 따른다.

Alert은 닫혀 있을 때도 자체 정적 위젯을 유지한다. 다시 열면 같은 액션 핸들을 사용한다.
이는 열림마다 자식 콘텐츠를 새로 만드는 [Modal](Modal.md)과 다른 원본 수명이다.
부모 View를 해제하면 Alert·AlertButton과 콜백을 해제하며 이전 핸들은 무효가 된다.
활성 메서드를 교체해도 실행 중 캡처는 반환 뒤 해제한다. 메서드가 소유 트리를 제거한 경우
이전 세대의 후속 클릭 통지는 전달하지 않는다.

독립 AlertButton은 기본 높이 20·좌우 여백 8·반경 4이다. `dialogStyle`은 좌우 여백 16·반경
16과 원본 대화 상자 색상을 선택하며 높이 56을 강제하지 않는다. Alert이 액션별 높이를 지정한다.
다섯 톤, enabled, 선택적 높이·여백·반경·텍스트/primary 색상과 누름·해제·취소 이벤트를 지원한다.
포커스 테두리는 버튼 반경을 따르는 1.5단위 GPU 선이다. 입력 수단에 따른 표시 정책과
전체 상속 focusRing 옵션·모션은 아직 부분 구현이다.

## 원본 이미지와 실제 유리 재질

원본 `alert-adjustments.svg`·`alert-file-text.svg`를 변경 없이 바이너리에 내장한다.
비정사각 이미지는 비율을 유지하여 중앙에 맞춘다. 기본 1/2개 액션은 64단위 adjustments, 세 액션은 56단위 file-text이며 프레임은 86·반경 28이다.
`alert_icon_source`로 원본 SVG를 조회할 수 있고 원본 라이선스 고지를 함께 설치한다.
`imageSource/appIconSource`는 알려진 내장 자산 문자열 또는 가져온 C++ 객체의
`std::shared_ptr<const ii::gui::RasterImage>` 멤버 참조이다. 빈 문자열은 원본 작은 사각형
대체 표시이다. 임의 파일·URL을 암묵적으로 읽지 않는다.

macOS Metal에서는 카드 아래의 앱 콘텐츠를 실제 GPU Gaussian blur로 흐린다.
dim·이전 팝업·카드 글자는 원본 캡처에서 제외하고 반투명 tint와 흰색 20% 테두리를 합성한다.
blur 반경은 원본처럼 2~64로 제한하며 기본 28이다. `glassEnabled: false`에서는 아래 레이어의
선명한 픽셀 위에 반투명 tint를 유지한다. 같은 배경에서 카드 글자나 버튼만 바뀌면 기존 blur를
재사용한다. [iiGUI 재질 계약](../../../iiGUI/docs/BackdropMaterials.md)에 ROI·64 MiB 작업 텍스처
한도·선형 색 공간·지원 백엔드와 계측 범위를 명시한다.

원본의 아래 16·blur 32·검정 28% 그림자도 실제 GPU 마스크로 합성한다. 위치·색상만
바뀌면 같은 R8/R16Float 마스크를 재사용한다. `shadowEnabled` 또는 C++의
`set_shadow_enabled`로 표시를 바꿀 수 있고 offset/blur/color를 명시할 수 있다.
[그림자 계약](../../../iiGUI/docs/Shadows.md)에 검증 범위와 공유 메모리 한도를 명시한다.

## 프레임 모션

기본 열림은 원본 `Motion.surface` 420ms·OutBack·overshoot 1.45이고 닫힘은 150ms·InCubic이다.
카드 scale은 `0.92 + 0.08 × revealProgress`, opacity와 dim은 진행 값을 0~1로 제한한다.
그림자와 유리는 원본처럼 scale/opacity를 적용하지 않는 형제이며, 유리는 닫는 즉시
비활성화한다. 카드와 정적 자식의 exit 픽셀만 유지하고 입력은 즉시 차단한다.
도중 다시 열면 현재 표시 값에서 새 곡선을 시작한다.

`lvrs::Motion`은 iiGUI의 실제 C++ 모션 설정 객체이다. `MotionTokens`에 원본
90·160·360·180·420·150·130ms와 overshoot를 제공한다. Alert의 `motion`에는 명시적인
`std::shared_ptr<lvrs::Motion>`을 전달할 수 있다. 생략하면 같은 호스트의 설정을 공유한다.
LVUI의 `motion: vm.motion`은 스코프 소유 C++ 멤버 참조로 컴파일한다.
`motionEnabled: false` 또는 `set_motion_enabled(false)`는 로컬 모션을 끈다.
공유 객체의 `set_reduced_motion(true)`/`set_enabled(false)`는 실행 중에도 즉시 목표 값으로
이동한다. `set_speed`는 0.1~4배 기준으로 시간을 바꾸며, 게임 일시 정지와 독립적이다.

실제 디스플레이 콜백→Core update→GPU present로 구동하며 16ms 타이머를 사용하지 않는다.
모션만 바뀌면 글자·이미지·그림자 마스크·같은 배경의 blur를 재사용한다.
[iiGUI 모션 계약](../../../iiGUI/docs/NativeMotion.md)에 지원 백엔드와 한도를 명시한다.
독립 AlertButton의 press/color 모션과 일반 SpringBehavior 예약어의 완전 이식은 아직 남아 있다.

## 사용과 검증 범위

```lvui
import vm AlertBackend
ApplicationWindow {
    Alert(id: "confirmation", open: true, title: "Native Alert",
          description: "Choose an action.", buttonCount: 2,
          primaryText: "Apply", secondaryText: "Close",
          button1Method: vm.apply, onSecondaryClicked: vm.closeDialog)
}
```

Alert에는 임의 자식 View를 넣지 않는다. `open: true`는 마운트와 부모 백엔드 구성 뒤 최초
개방한다. 콘텐츠 생성 함수 안의 초기 개방은 현재 진단하며 활성화 뒤 명시적으로 열 수 있다.
C++ 타입은 `Alert::create`와 `AlertButton::create`, 단순 위젯 API는 `Widgets::alert/alert_button`이다.
타입 핸들은 텍스트·액션 개수/활성/메서드·primary 색상·아이콘/유리 표시 변경을 제공한다.

`tests/native/alert.cpp`는 원본 기하, 여섯 액션 위치, disabled, 메서드 교체, 포커스, 좁은 창,
100회 재개방의 동일 위젯, 원본 SVG 바이트와 실제 blur 픽셀을 검사한다. AlertButton 검사는
포인터·키보드의 pressed→released→method→clicked, 비활성화 취소와 실행 중 소유권 해제를
검사한다. `examples/NativeAlertGallery`는 실제 AOT의 인자 있는/없는 메서드·여섯 액션·독립
버튼·초기 개방·Core 표시 프레임·작업자 결과·크기 변경·종료를 함께 실행한다.

아직 부분 이식이다. 임의 backdropSource/overlay 부모, 일반 반응형 속성 재바인딩과 전체 상속 API,
임의 이미지 URL·일부 작은 아이콘 프레임 조합이 남아 있다.
GPU 배경 흐림은 현재 Metal만 구현했고 D3D11/Vulkan은 요청 시 명시적으로 실패한다.
Windows/Linux/mobile/WASM 위젯 입력·표시 검증도 미완료이다. 전체 원장의 complete로 세지 않는다.

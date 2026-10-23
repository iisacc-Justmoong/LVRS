# 네이티브 Modal

`lvrs::Modal`, `Widgets::modal`과 LVUI `Modal`을 iiGUI 자체 C++ 위젯으로 구현했다.
원본은 `src/qml/components/surfaces/Modal.qml`이며, 창·입력·IME·가상 접근성은 iiGUI가
연결하고 앱 수명·작업자·프레임은 iiSDKCore가 소유한다. OS 대화 상자나 Cocoa 컨트롤은 만들지 않는다.

## 배치와 외형

프레임은 창의 수평·수직 중앙에서 기본 16 논리 단위 위에 놓인다. 기본 폭은
`min(980, max(520, min(760, 창 폭 - 48)))`이다. 내부 패딩 20, 섹션 간격 20,
최소 높이 210, 기본 모서리 12이다. 원본처럼 창이 최소 폭보다 작아지면 최소 폭을 유지한다.
`shapeStyle: .cylinder`는 짧은 변의 절반을 반경으로 사용한다.

왼쪽 아이콘은 기본 72×72·모서리 16·안쪽 여백 8이며 원본 배경과 테두리를 사용한다.
이름이 없으면 작은 기본 사각형을 표시한다. `iconName`은 내장 SVG 이름이고,
직접 C++에서는 `icon_source`에 네이티브 불변 이미지를 전달할 수 있다. LVUI의 `iconSource`는
스코프의 공개 이미지 멤버 참조이다. 비정사각 이미지는 비율을 유지하여 중앙에 맞춘다. 임의 URL·파일 로딩은 포함하지 않는다.

제목은 title2·고정 줄 높이 22, 설명은 body·고정 줄 높이 18이며 폭에 따라 줄바꿈하고 프레임 높이에 반영된다.
설명이 비어 있으면 `message`를 사용한다. `desc`는 LVUI에서 `description`의 별칭이며
두 이름을 동시에 지정하면 진단한다. `showIcon: false`는 아이콘 공간도 제거한다.

## 액션과 취소

`buttonCount: 0`은 자동이다. 3차 텍스트가 있으면 세 개, 2차 텍스트가 있으면 두 개,
기본 텍스트만 있으면 한 개, 모두 비면 액션이 없다. 양수 개수는 1~3으로 제한한다.
필요한 텍스트가 비면 각각 OK·Cancel·More를 사용한다. 순서는 오른쪽부터 기본·2차·3차이며
간격 12, 높이 20, 모서리 4이다. 개별 enabled 상태를 적용한다.

액션은 콜백을 실행하며 자동으로 닫지 않는다. 소유자가 확정·유지·닫기를 결정한다.
`close()`·`close_popup()`은 일반 닫기, `cancel()`·`cancel_popup()`은 취소이다.
취소는 본문 해제와 닫힘 통지 뒤 `canceled`를 한 번 전달한다. 이미 닫혔으면 다시 전달하지 않는다.
외부 누름은 `dismissOnBackground`가 true일 때 취소하고 항상 배경 입력을 차단한다.
네이티브 키보드 정책으로 자식이 처리하지 않은 Escape도 취소한다. Tab과 이전 포커스 복귀는
iiGUI의 팝업 수명 계약을 따른다. 부모 해제·숨김은 사용자 취소로 세지 않는다.

## 열림 단위 콘텐츠

```lvui
import vm ModalBackend
ApplicationWindow {
    Modal(id: "dialog", title: "Native Modal", description: "Fresh editor",
          iconName: "projectStructure", buttonCount: 2,
          primaryText: "Apply", secondaryText: "Close",
          onPrimaryClicked: vm.apply, onSecondaryClicked: vm.closeDialog) {
        VStack(spacing: 12) {
            InputField(id: "editor", text: "Fresh", onEdited: vm.edited)
        }
    }
}
```

자식 블록은 C++ `ModalContentFactory`로 AOT 생성된다. 제목·설명·아이콘·액션은 마운트 시
한 번 생성한 정적 C++ 위젯이다. 본문만 정적 외곽 안의 `bodySlot`에 열 때 생성하고
닫을 때 즉시 해제한다. 비활성 본문은 배치 간격에도 포함하지 않는다. 제목·아이콘과
액션이 모두 없으면 빈 섹션의 간격을 제거한다. 바깥 import의 백엔드는 동일 인스턴스를
빌려 쓴다. 매번 새 backend를 만들지 않는다.
콘텐츠 안 ID는 별도 활성화 범위이고 같은 이름의 바깥 ID와 충돌하지 않는다. 다른 범위의
context menu target을 직접 참조하면 컴파일 시 거부한다. `open: true`는 마운트와 백엔드
구성이 끝난 뒤 최초 개방한다. 생성 함수 안의 중첩 Modal에 초기 개방을 요구하는 것은
현재 거부하며, 활성화 완료 뒤 소유자가 명시적으로 연다.

C++에서는 `Modal::create(builder,parent,id,options,events,content)`를 사용한다.
`content`는 `(WidgetBuilder&, Widget parent) -> Result<void>`이다. 자식 객체·구독을
`builder.retain`으로 소유할 수 있다. `content_item()`은 현재 콘텐츠 컨테이너,
`root().popup_content("editor")`는 현재 자식 핸들이다. 이전 핸들은 다음 개방을 조작하지 못한다.
`action_visible`·`trigger_action`·`contains_frame_point`도 제공한다.
`root().popup_content`는 열린 동안 본문 다음 정적 외곽을 찾는다. 정적 외곽은
마운트의 `find("modal.title")`처럼 닫힌 동안에도 같은 핸들로 조회할 수 있다.
실행 중 캡처·외부 공유 소유자·작업 취소의 경계는 [활성화 계약](ActivationContent.md)을 따른다.

## 네이티브 프레임 모션

기본 열림은 원본처럼 420ms OutBack·overshoot 1.45, 닫힘은 150ms InCubic이다.
프레임 scale은 `0.92 + 0.08 × revealProgress`이며 opacity와 dim은 진행 값을 0~1로 제한한다.
본문 Loader는 닫는 즉시 제거하므로 퇴장 프레임의 높이도 본문 없이 재계산한다.
제목·아이콘·액션은 남아 퇴장하지만 입력은 즉시 차단하고 이전 포커스로 복귀한다.
도중 다시 열면 현재 표시 값에서 진행하며 본문은 새로 만든다.

`motionEnabled`/`set_motion_enabled`는 로컬 전환을 제어한다. `motion: vm.motion`에는
스코프 소유 `std::shared_ptr<lvrs::Motion>`을 빌려 주며, 생략하면 호스트 모델을 사용한다.
공유 모델의 enabled·reduced_motion·speed는 실행 중에도 적용한다. 프레임 콜백은
Core의 unscaled 시계를 사용하므로 게임 일시 정지와 독립적이다. 모션이 끝나면
명시적 연속 루프가 없는 앱은 유휴 상태로 돌아간다. [모션 계약](../../../iiGUI/docs/NativeMotion.md)을 따른다.

frameColor의 alpha도 허용하며 선형 색 공간에서 아래 배경과 합성한다. 모션을 끄면
정적 레이아웃과 색상에 즉시 도달한다. 현재 실제 합성과 위젯 실행 검증은 macOS Metal 범위이다.

## 검증과 남은 차이

`tests/native/modal.cpp`는 원본 기본 배치·아이콘·설명 대체·액션 순서/기본 텍스트/차단,
일반 닫기와 취소, 30회 새 콘텐츠 생성·해제, 크기 변경 시 기존 콘텐츠 유지와 실제 Metal
프레임 픽셀을 검사한다. `examples/NativeModalGallery`는 LVUI의 실제 자식 생성 함수와
초기 개방을 사용하며 Core 프레임·백그라운드 결과·크기 변경·종료를 함께 검사한다.

`tests/native/modal_motion.cpp`는 정적 외곽의 반복 사용, 본문 즉시 해제·퇴장·재개방,
실행 중 모션 설정, 변환 입력, 반투명 프레임 픽셀, 빈 섹션, 생성 실패와 닫힘 콜백의
재진입을 검사한다. `NativeModalGallery`의 자체 검사도 실제 프레임 전환까지 진행한다.

아직 부분 이식이다. 반응형 속성 재바인딩, 대체 overlay 부모, 전체 상속 API,
임의 icon URL과 언어 내부의 별도 자식 backend import가 남아 있다.
Windows/Linux/mobile/WASM 위젯 입력 어댑터도 미완료이다. 전체 대응표의 complete로 세지 않는다.

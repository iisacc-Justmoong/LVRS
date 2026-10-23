# 자체 콘텐츠를 가진 Popover

`Popover`는 iiGUI C++ 위젯 트리 안의 오버레이이다. 일반 레이아웃의 공간을 차지하지
않으며, OS 팝오버나 별도 OS 창을 만들지 않는다. 하나의 일반 콘텐츠 뷰와 소유 팝업들을
가질 수 있다. 여러 뷰는 `VStack`·`HStack`·`ZStack` 하나로 묶는다. 콘텐츠는 닫힌 상태에서도
측정되며 열 때마다 같은 C++ 객체·백엔드·모델을 사용한다.

```lvui
import vm EditorBackend
ApplicationWindow {
    VStack {
        LabelButton(id: "open", text: "Edit", onClick: vm.open)
        Popover(id: "editor", onOpened: vm.opened, onClosed: vm.closed) {
            ColorPicker(model: vm.model, onAccepted: vm.accepted, onCanceled: vm.canceled)
        }
    }
}
```

열기는 마운트 후 C++ 소유자가 `Widget::open_for(anchor, x, y)` 또는 `open_at(x, y)`로
요청한다. 닫기는 `close_popup()`이다. 앵커 좌표는 앵커 좌상단 기준 오프셋이며 크기 변경으로
앵커가 이동하면 팝업도 따라간다. 자신·자손으로의 앵커 순환은 거절한다.
원본 `ColorPickerButton`도 클릭 콜백을 제공하는 버튼이며 팝오버의 자동 소유자가 아니다.
이제 [ColorPickerButton](ColorPickerButton.md)의 기본 네이티브 동작도 제공하며,
공유 모델을 사용하는 실행 예제에서 그 버튼으로 팝오버를 연다. 전체 모션·상속 계약은 남아 있다.

## 생성 입력

| 입력 | 기본값·의미 |
|---|---|
| `width`, `height` | 0: 콘텐츠 자연 크기 + 패딩, 양수: 외곽 크기 |
| `padding`, `margins`, `cornerRadius` | 12, 8, 12 논리 단위 |
| `surfaceColor`, `surfaceOpacity` | 원본 `#141414` 바탕, 코팅 0.25 |
| `modal`, `dim` | 각각 false, 배경 입력 차단과 시각적 어두움은 독립 |
| `focus`, `clip` | 각각 true, false |
| `closeOnEscape`, `closeOnPressOutside` | 각각 true |
| `accessibilityName` | `Popover` |
| `onOpened`, `onClosed` | 인자 없는 C++ 메서드 참조 |

`surfaceColor`는 선택적 `#`를 붙인 6자리 RGB이다. 코팅 알파는 `surfaceOpacity`로 지정한다.
기하 값은 유한한 음이 아닌 값이며 폭·높이·패딩은 8192 이하이다. 콘텐츠가 창보다 크거나
고정 높이에 잘린 경우 `clip: true`로 스크롤·포커스 요소 드러내기를 사용할 수 있다.
기본 `clip: false`에서는 콘텐츠 박스를 넘는 자식도 그릴 수 있다.

## 입력·수명·모델

열 때 첫 입력 가능한 자식으로 포커스를 옮기고 Tab을 콘텐츠 안에서 순환시킨다.
자식의 Escape 처리가 먼저 실행되며 처리자가 없을 때 팝오버의 닫기 정책을 적용한다.
텍스트·슬라이더·색상 필드는 일반 위젯과 같은 입력·포인터 캡처를 사용한다.
모든 열린 팝업 밖의 누름은 닫기 정책을 적용한다. 비모달이면 누름이 배경 위젯으로
이어지고 모달이면 소비한다. 다른 열린 팝업 안의 누름은 외부 누름으로 판정하지 않는다.
부모를 닫으면 그 부모가 소유한 중첩 메뉴·팝오버도 닫힌다.

닫을 때 편집 중 입력의 포커스 손실 확정과 닫힘 콜백을 실행한다. 포커스를 여전히 소유하면
열기 전 위치로 돌려준다. 콜백에서 트리를 제거하거나 팝업을 재개방하면 이전 세대의 누름이
새 대상에 실행되지 않는다. 닫기만으로 스코프를 파괴하지 않으며 최종 해제는 Core 앱의
뷰 부착 수명에 따른다.

Popover는 모델의 적용·복원을 결정하지 않는다. 예제 `examples/NativePopoverGallery`는
열기 전 기준색을 저장하고 Apply는 확정, Cancel·외부 누름은 복원하는 소유자 정책을 보인다.
하나의 ColorPicker와 백엔드를 4회 재사용하고 최종 Core 종료에서 한 번 해제한다.
앱·게임 공용 `Application::frame_loop()`를 표시 신호에 연결하고 백그라운드 결과·크기 변경도
같은 앱에서 처리한다.

원본 Glass25의 배경 흐림·방사형 악센트·그레인·스프링 전환은 아직 없다. 현재는 불투명 바탕과
코팅을 사용한다. 동적 콘텐츠, 반응형 크기·정책 변경, 전체 상속 API, 독립 팝업 여러 개의
접근성·포커스와 Windows/Linux 위젯 어댑터도 남아 있다. 이 타입은 부분 이식이다.
Qt [Popup 문서](https://doc.qt.io/qt-6/qml-qtquick-controls-popup.html)는 동작 요구의 참고이며
코드나 런타임 의존성은 추가하지 않았다.

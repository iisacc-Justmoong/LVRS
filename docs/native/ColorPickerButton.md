# 네이티브 ColorPickerButton

`ColorPickerButton`은 현재 색상을 표시하는 iiGUI 자체 C++ 버튼이다. 클릭을 소유자에게
전달하며 피커나 팝오버를 자동 생성하지 않는다. 원본 전용 320×320 PNG
`resources/images/color-picker-hue-ring.png`를 바이트 변경 없이 바이너리에 내장한다.
ColorPicker의 448×448 휠과 다른 자산이며, 설치 앱은 원본 파일을 실행 중 읽지 않는다.

기본 크기는 28, 기본색은 `#874AFF`, 색상환은 표시 상태이다. `.Small`·`.Medium`·`.Large`는
22·28·36 논리 단위이다. 패딩 4, 반지름 4, 현재색 원의 지름은 가용 정사각형의 18/28이다.
임의 폭이 주어진 루트 영역에서도 중앙 색상환과 색 원의 비율을 유지한다. 알파는 원의
배경 합성에 적용하며 체커보드를 추가하지 않는다. 기본·hover·pressed 배경, 0.5 흰색 20%
테두리와 포커스의 악센트 2 테두리를 사용한다. 비활성 상태는 합성한 버튼 전체를 0.32로
표시하고 입력과 포인터 포착을 취소한다.

```lvui
import vm EditorBackend
ApplicationWindow {
    VStack {
        ColorPickerButton(id: "open", model: vm.model, buttonSize: .Large,
                          accessibilityName: "Edit color", onClick: vm.open)
        ColorPickerButton(currentColor: "#00E71F80", buttonSize: 22, showHueRing: false)
        Popover(id: "editor", onClosed: vm.closed) {
            ColorPicker(model: vm.model, onAccepted: vm.accepted, onCanceled: vm.canceled)
        }
    }
}
```

`currentColor`는 6/8자리 RGB/RGBA HEX 문자열이다. `model`에는 스코프가 소유한 공개
`std::shared_ptr<lvrs::ColorPickerModel>` 멤버를 지정한다. 둘은 함께 지정할 수 없다.
모델을 공유하면 팝오버 안의 편집 결과·적용·취소가 닫힌 팝오버 밖의 버튼에도 반영된다.
둘 다 생략하면 버튼마다 기본 보라색 모델을 소유한다. `onClick`·`onPressed`·`onReleased`·
`onCanceled`는 인자 없는 메서드 참조이다. 포인터·Space·Enter·가상 접근성의 누르기가 같은
C++ 버튼 디스패치를 사용한다. 정상 누름은 pressed→released→clicked 순서이다.

C++에서는 `Widgets::color_picker_button`과 `ColorPickerButtonOptions`를 사용한다.
반환한 `ii::gui::Widget`의 `color_model()`로 모델을 얻고 `set_color_model()`로 명시적으로
교체한다. 이전 모델의 구독은 해제되며 오래된 모델이 새 버튼을 갱신하지 않는다.
`set_color_button_size()`는 네이티브 8..256 범위에서 크기를 재측정하고,
`set_show_hue_ring()`는 콘텐츠를 재생성하지 않고 표시를 바꾼다. AOT 크기는 원본 세 이름과
22·28·36만 받는다. 직접 iiGUI를 사용할 때에는 모델과 표시할 색상환 이미지를 제공한다.

버튼 이미지는 상태·현재색·배치 크기가 바뀔 때 CPU에서 합성하고 네이티브 GPU로 그린다.
같은 상태의 프레임과 반복 캡처는 같은 이미지를 재사용한다. 작은 버튼은 4배 샘플링이며
큰 명시적 영역은 한 변 최대 2048 픽셀로 제한한다. 비활성 투명도는 합성 이미지 전체에
적용한다. 원본 색상환의 투명 경계는 premultiplied 보간으로 처리한다.

스프링 압축·반동, 상태·투명도 전환과 reduced-motion 계약, 전체 상속 배경·패딩·반지름 입력,
일반 반응형 필드 재연결과 Windows/Linux 위젯 어댑터는 남아 있다. 이식 상태는 부분 구현이다.
실행 예제는 `examples/NativePopoverGallery`이다. `tests/native/color_button.cpp`는 원본 자산·
기본값·크기·모델을 검사하며, iiGUI의 전용 검사 5개는 입력·GPU 픽셀·캐시·수명을 검사한다.

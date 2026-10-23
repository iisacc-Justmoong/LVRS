# 네이티브 ColorPicker

`ColorPicker`는 iiGUI의 자체 C++ 위젯을 조합한 내장형 색상 편집 뷰이다.
OS 색상 패널이나 OS 입력 컨트롤을 만들지 않는다. 부모가 패널·모달·팝오버의
배경·배치·열기·닫기를 소유하며, 선택기는 적용·취소 후에도 스스로 닫히지 않는다.
LVUI는 이 조합을 만드는 C++ 호출을 AOT로 생성한다. 최종 앱 수명은 iiSDKCore가 소유한다.

## 편집 형태와 배치

| `type` | 구성 | 기본 크기 |
|---|---|---|
| `.Wheel` | H/S/B 채널·휠 | 320 × 461 |
| `.HueSaturation` | HS 평면·밝기 레일·B 채널 | 320 × 437 |
| `.SaturationBrightness` | SV 평면·색조 레일·H 채널 | 320 × 437 |
| `.Grayscale` | 그레이 K 채널·스펙트럼 | 320 × 361 |
| `.RGB` | R/G/B 채널·스펙트럼 | 320 × 413 |
| `.CMYK` | C/M/Y/K 채널·스펙트럼 | 320 × 439 |

원본의 12 단위 패딩·간격, 22 높이 채널 행, 28 높이 비교/HEX/알파 행,
20 높이 최근 색상과 22 높이 액션 행을 사용한다. 폭은 320..1024이며,
패딩은 0..64 중 콘텐츠 폭 228 이상을 남기는 범위이다. 평면은 콘텐츠 폭에서
36을 뺀 정사각형으로 커진다. 최근 색상을 숨기면 높이 32, 액션을 숨기면 34가 줄어든다.
윈도우 크기를 바꾼다고 선택기 자체의 폭을 자동 재설정하지는 않는다.

## LVUI

```lvui
import vm ColorBackend
ApplicationWindow(title: "Color editor", width: 700, height: 600) {
    HStack(spacing: 16, padding: 12) {
        ColorPicker(id: "wheel", model: vm.model, type: .Wheel,
                    onColorEdited: vm.edited, onAccepted: vm.accepted,
                    onCanceled: vm.canceled,
                    onEyedropperRequested: vm.eyedropper)
        ColorPicker(model: vm.model, type: .CMYK,
                    recentColors: ["#00E71F80", "874AFFFF"])
    }
}
```

백엔드의 `model`은 공개 `std::shared_ptr<lvrs::ColorPickerModel>` 멤버이다.
같은 포인터를 받은 뷰는 명시적 Core 변경 쿼리로 같은 색상을 표시한다. 생략하면 선택기
인스턴스가 모델을 소유하고 `#874AFF`로 시작한다. 순수 모델 자체의 기본값은 흰색이다.
콜백은 `vm.method` 참조이며 호출식은 거절한다. 색상 콜백의 인자는
`ii::gui::WidgetColor`이며 취소·스포이트 요청은 인자가 없다.

`type`, `width`, `padding`, `enabled`, `showRecentColors`, `showActions`,
`applyText`, `cancelText`, `recentColors`를 생성 시 지정한다. 최근 색상은 처음 10개만
표시하며 6자리 RRGGBB 또는 8자리 RRGGBBAA 문자열을 받는다. 생략하면 원본 악센트
10색을 사용한다. 빈 목록과 숨김은 별개이다. 전역 최근 이력이나 영속 저장을 추가하지 않는다.

## C++와 입력 계약

`ColorPicker::create(builder, parent, id, options, events)`는 약한 핸들을 반환한다.
`root()`는 배치에 사용하는 iiGUI 위젯이며 실제 조합 상태는 마운트가 소유한다.
`Widgets::color_picker`는 같은 생성 경로의 루트 위젯을 반환한다. 핸들의 `set_type`,
`set_width`, `set_enabled`, `show_recent_colors`, `show_actions`, `set_recent_colors`로
같은 모델과 자식들을 유지하며 조합을 조정한다. `model()`로 공통 모델을 사용할 수 있다.
기존 모델 포인터를 바꾸려면 이 합성 뷰를 다시 마운트한다.

숫자·HEX는 Enter 또는 포커스 손실 때 사용자가 편집한 값만 확정한다. 숫자 표시를
포커스했다가 나오는 것만으로 원래 실수 값을 반올림하지 않는다. 잘못된 입력은 색상을
변경하지 않고 현재 모델의 표시로 복원한다. 숫자는 유한한 십진수이며 모델 범위로 제한된다.
HEX 입력은 최대 9문자, 선택적 `#`, 6/8자리를 지원하며 6자리는 알파를 보존한다.

`accept()`는 루트로 포커스를 옮겨 보류 입력을 확정하고 모델의 비교 기준을 갱신한 뒤
`accepted`를 보낸다. 취소도 같은 편집 포커스 정리를 거친다. 최근 색상은 그 버튼으로
포커스를 옮기므로 접근성/프로그래밍 실행 후에도 이전 HEX 입력란의 표시가 남지 않는다.
`cancel()`과 자식에 포커스가 있는 상태의 Escape는 비교 기준을 복원하고 `canceled`를
보낸다. 최근 색상 선택은 그 색상의 알파도 적용한다. `set_color`와 `set_hex`는 사용자
편집 경로를 사용하며, 외부 기준 갱신은 공유 모델의 `set_color`·`set_previous_color`를 쓴다.
`eyedropper_requested`는 소유자에게 요청만 전달한다. 실제 문서/화면 샘플링과 그 결과의
`set_color` 반영은 소유자의 책임이며 OS 색상 패널을 열지 않는다.

채널 배경은 원본의 7개 색상 정지를 이미지로 만들며 정지가 같으면 재사용한다.
미니 슬라이더의 트랙·썸·포인터 범위는 같은 메트릭을 쓴다. 비교 견본은 4 단위 체커보드
위에 이전/현재 색상을 나누고, 최근 색상은 현재 선택 테두리를 표시한다. 동적 램프·견본은
변경 시 CPU에서 생성하고 기존 GPU 합성기로 표시한다. 프레임마다 모델을 폴링하지 않는다.

## 검증 범위와 남은 차이

`LVRS.native_picker_layout/editing/lifetime`는 여섯 크기·형태 전환·공유 상태 보존,
숨김 높이·폭 확장, 숫자/HEX/알파, 무편집 포커스, 적용·취소·최근 색상·스포이트 요청과
편집 콜백 중 자기 트리 제거 후 모델 해제를 검사한다. `NativePickerGallery`는 휠·HS·CMYK
세 합성 뷰를 LVUI로 만들고 같은 모델을 공유하며 Core 작업자·실제 표시 프레임·크기 변경·
정상 종료와 스코프 해제를 검사한다.

현재 단계도 원본 전체 동등성 판정은 `partial`이다. 일반 LVUI 반응형 옵션·모델 포인터
재바인딩, 상속 API 전부, 정확한 재질·동작·포커스 장식·disabled 하위 트리 전체 투명도,
접근성 값 메타데이터 일부, Unicode 공백/Qt 숫자 변환의 가장자리 호환성 및 다른 플랫폼의
입력·표시는 남아 있다. 비교 견본과 램프도 절차적 GPU 셰이더/아틀라스 구현을 뜻하지 않는다.

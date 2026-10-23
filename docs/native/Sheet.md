# 네이티브 Sheet

`lvrs::Sheet`와 `Widgets::sheet`는 원본 `src/qml/components/surfaces/Sheet.qml`의
중앙/하단 모달 표면을 iiGUI C++ 위젯으로 구성한다. 제목·설명·닫기 버튼·grabber·본문은
부모 마운트의 수명을 가지며, 닫을 때 본문을 해제하지 않는다. 원본 Loader의
`active: contentComponent !== null` 계약을 따르며 Modal의 열림 단위 본문과 다르다.
`SheetContentFactory`는 생성 시 한 번 호출한다. 같은 본문의 편집 값은 다시 열어도
유지하고, 재개방은 스크롤과 드래그 위치만 초기화한다. 전체 마운트 해제 시 회수한다.

## 배치와 스타일

- presentation은 `automatic`, `mobile`, `desktop`이다. 자동은 컴파일 대상의 모바일
  플랫폼 여부를 사용하며 창 폭만으로 바꾸지 않는다. macOS에서도 mobile을 지정해
  같은 C++ 배치를 실행할 수 있으나 iOS/Android 장치 실행 증거를 뜻하지 않는다.
- 데스크톱 너비는 기본 560과 양쪽 24 여백으로 제한한 뷰포트 너비 중 작은 값이다.
  높이는 고정 외곽 높이와 preferredHeight(기본 420) 중 큰 값을 같은 여백 안으로
  제한하며 중앙에 둔다.
- 모바일은 전체 너비와 하단 정렬을 사용한다. fit은 본문과 외곽의 자연 높이,
  medium은 화면 높이의 57%, large는 90%를 반올림한다. 최대 높이는 화면 높이에서
  `max(24, topSafeInset)`을 뺀 값이다. 하단 안전 여백은 본문 아래에 포함한다.
- 원본의 기본 radius는 모바일 24, 데스크톱 16이며 현재 표면의 반너비/반높이로
  제한한다. surfaceColor와 backdropColor는 RGBA를 받으며 GPU의 선형 색 공간에서
  합성한다. 기본 backdrop은 검정 48%이다.
- showHeader·showDescription·showCloseButton·showGrabber, 20 기본 contentPadding,
  preferredWidth/Height·desktopMargin과 top/bottomSafeInset을 C++ `set_options`로
  바꾸어도 본문을 재생성하지 않는다. 안전 여백의 OS 자동 관찰은 아직 연결하지 않는다.

고정 헤더는 본문 스크롤에 참여하지 않는다. 본문은 [iiGUI 스크롤 계약](../../../iiGUI/docs/ScrollViews.md)을
사용하며 휠·트랙패드·자식 포커스 노출·중첩 clip·표시기를 제공한다. scrollContent가
false이면 본문 높이를 뷰포트 높이에 맞추고 넘친 픽셀을 자른다. 모바일 grabber만 아래로
드래그할 수 있으며 `min(120, height * 0.2)` 이상에서 닫는다. 그보다 짧은 드래그는
OutBack으로 복귀한다. 본문의 터치 관성 스크롤과 스크롤바 드래그는 남아 있다.

## 프레임·입력·수명

enter는 OutBack, exit는 InCubic이며 양쪽 모두 원본 animationDuration(기본 420ms)을
사용한다. 모바일은 높이에 따른 수직 이동, 데스크톱은 0.94→1 배율과 투명도이다.
드래그 복귀도 같은 공유 모션 설정을 따른다. Core의 unscaled 프레임을 사용하므로
게임 정지 중에도 진행한다. 모션 감소·속도·로컬 motionEnabled는 실제 실행 중 적용한다.
이동 합성은 Metal이 기존 글자·이미지 텍스처를 재사용한다. D3D11/Vulkan의 비항등
변환 요청은 현재 명시적으로 미지원이며 타이머로 동기화를 흉내 내지 않는다.

dismissOnBackground·dismissOnEscape·dismissOnDrag는 각각 별도 정책이다. 닫기 버튼은
명시적으로 닫으며 자식의 일반 액션에는 자동 닫기를 넣지 않는다. 모달 배경 클릭을
소비하고 이전 유효 포커스를 복원한다. 현재 opened/closed 통지와 논리적 입력 해제·포커스
복귀는 iiGUI의 즉시 전환 계약이다. 원본 Qt Popup의 전환 완료 시점 신호·상속 API 전체와
완전히 같은 타이밍으로 검증한 것은 아니다.

`root()`, `viewport()`, `content_item()`은 약한 세대별 핸들이다. `content_item()`은
보관한 콘텐츠 슬롯이며 원본 Loader의 item 객체를 그대로 노출한다는 뜻은 아니다.
네이티브 함수에서 잘못된 문자열·지오메트리·색상·열거형·시간 값은 오류로 반환한다.

## LVUI와 검증

```lvui
import vm ExportBackend
ApplicationWindow {
    Sheet(id: "export", title: "Export", presentation: .desktop,
          motion: vm.motion, onOpened: vm.opened, onClosed: vm.closed) {
        VStack(spacing: 12) {
            InputField(id: "filename", text: "Untitled", onEdited: vm.edited)
            LabelButton(text: "Export", onClick: vm.exportFile)
        }
    }
}
```

`FRAMEWORK LVRS`에서 정적 옵션과 deferred C++ 메서드 참조를 코드 생성한다. 본문의
여러 View는 원본 inline Item처럼 같은 콘텐츠 슬롯을 공유하며 필요하면 스택으로 묶는다.
Sheet 본문의 ID는 마운트 전체에서 유일해야 한다. 부모 C++ backend를 다시 생성하지
않는다. 일반 반응형 값 바인딩·실행 중 contentComponent 교체·별도 자식 backend import·
처음부터 열린 중첩 팝업·모든 모바일/데스크톱 입력과 RHI 동등성은 남아 있다.

`tests/native/sheet.cpp`는 실제 macOS 창에서 배치·수명·스크롤·정책·드래그·모션·GPU를
검사한다. `examples/NativeSheetGallery`는 두 프레젠테이션을 같은 LVUI 소스에 포함하며
Core 작업자 결과·프레임·크기 변경·본문 보존·공유 모션·정상 종료를 검증한다. 전체 LVRS의
완료를 뜻하지 않으며 [대응표](Parity.md)의 Sheet는 부분 구현 상태이다.

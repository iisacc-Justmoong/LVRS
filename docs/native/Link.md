# 네이티브 Link

`lvrs::Link`와 LVUI의 Link는 iiGUI의 자체 C++ ActionSurface로 구성한다. 원본
Link.qml의 기본 텍스트, 사용자 정의 자식 슬롯, 투명한 배경, 탐색 대상 우선순위와
상태별 색상을 구현한다. OS 링크 컨트롤이나 일반 텍스트 버튼 별칭을 사용하지 않는다.

## 시각 합성과 입력

기본값은 description 12·semibold·고정 행 높이 12, 가로 패딩 14·세로 패딩 10,
최소 높이 36, 포커스 링 반경 8이다. 배경은 투명하다. 자식 슬롯이 비어 있으면
텍스트를 표시하고 일반 자식 뷰가 있으면 텍스트 fallback을 숨긴다. 원본처럼 텍스트
자연 크기는 슬롯의 implicit 크기 계산에 참여한다. 여러 자식은 같은 콘텐츠 원점을
사용하므로 행·열 구성에는 HStack/VStack을 사용한다.

```text
import vm Backend
Window {
    Link(id: "report", to: vm.href, navigator: vm.navigator, underline: true) {
        HStack(spacing: 6) {
            Label(text: "Open report")
            Label(text: "↗")
        }
    }
}
```

자식은 같은 네이티브 위젯 세대와 기존 LVUI 백엔드 스코프를 사용한다. 자식 컨트롤의
클릭을 부모 Link로 중복 처리하지 않는다. 장식용 뷰와 빈 패딩은 부모의 동작 영역이다.
포인터·포커스·Enter/Space의 눌림/해제·취소는 iiGUI가 처리한다. 비활성화 시 하위
포커스와 눌림도 취소한다. 링크 역할과 press는 macOS 가상 접근성 피어에 연결한다.

link_color 기본값은 생성 시 appearance의 accent, hover_color는 흰색 90%,
pressed_color는 링크 RGB를 1.2로 나눈 값, disabled_color는 흰색 60%이다.
명시적인 RGBA 색상과 밑줄을 교체할 수 있다. 밑줄은 AppKit attributed text·
DirectWrite·Pango 셰이핑을 사용하며 GPU에 그 결과를 전달한다. 임의 고정 선을
추가하는 장식으로 처리하지 않는다.

## 경로와 컴포넌트 탐색

LinkOptions는 text, href, params, router, navigator, target_component, replace,
enabled, underline, tone, 네 가지 색상과 기본 패딩·크기·포커스 옵션을 받는다.
href 기본값은 `/`이며 set_to는 set_href와 같은 속성을 변경한다. LVUI의 href와
to는 별칭이므로 동시에 지정하면 컴파일 오류이다.

탐색 대상은 살아 있는 명시적 NavigationTarget이 우선한다. 없거나 만료되면 지정한
Navigator, 또는 소유 스레드의 기본 Navigator를 사용한다. target_component가 있으면
href보다 우선하며, 그렇지 않으면 빈 href는 무동작이다. replace는 현재 항목 교체,
false는 push를 요청한다. Core Value와 컴파일한 PageComponent 정체성을 보존한다.
문자열 소스나 QML 컴포넌트를 런타임에 해석하지 않는다.

trigger_navigation()은 원본의 명시적 triggerNavigation 호출에 대응한다. 수명이
끝난 위젯에는 호출할 수 없지만, 명시적인 메서드 호출 자체를 enabled·visible로
차단하지 않는다. 사용자 클릭은 iiGUI의 유효 활성화·모달·포커스 경계를 따른다.
명시적 대상의 bool은 그 대상 계약, Navigator의 bool은 위임 수락 여부이다.
오류는 Result와 cause로 전달하고 경쟁하는 동일 Link 탐색은 busy이다.

params는 NavigationStackModel과 같은 단일 항목/중첩/노드 예산으로 검사한다.
경로는 64 KiB 이하 UTF-8·NUL 없음 계약이며 경로 정규화는 최종 대상이 수행한다.
typed component가 아닌 경로의 매개변수는 PageRouter의 객체 계약도 따른다.

## C++ 객체·메서드·반응형 입력

Link::create는 root() 핸들과 약한 Link 객체를 반환하고, 빌더가 상태를 retain한다.
선택적인 LinkContentFactory는 같은 빌더와 root를 받아 일반 자식을 구성한다.
실패하면 외부 마운트 팩토리가 오류를 전파해 기존 마운트를 유지해야 한다. 같은
빌더에 이미 추가한 노드를 Link만의 별도 트랜잭션으로 되돌리는 API는 아니다.

공개 setter로 text, href/to, params, router, navigator, target_component, replace,
enabled, tone, underline, 네 색상과 method/methods를 바꿀 수 있다. 색상 교체는
현재 네이티브 옵션에서 시작하므로 최신 text·enabled 바인딩을 덮어쓰지 않는다.
Disabled tone은 별도의 interaction gate이므로 요청한 enabled를 잃지 않는다.

공통 ButtonEvent/ButtonMethod는 AlertButton에서 독립된 헤더로 분리했다. Link는
method 하나와 최대 256개 typed methods, clicked·pressed·released·canceled를
받는다. 네이티브 클릭 순서는 탐색 결과 알림 후 살아 있는 소스의 method, methods,
clicked이다. 앞선 콜백이 소스를 종료하면 남은 메서드를 호출하지 않는다. 탐색 오류는
navigation_failed가 받으며 없으면 호스트의 오류 장벽으로 전달한다. navigated는
위임 결과를 전달한다. 이미 커밋한 탐색의 수신 오류를 무조건 재시도하지 않는다.

LVUI는 기존 import의 C++ 멤버를 직접 사용한다. href/to·params·router·navigator·
targetComponent·replace·underline·tone·색상·text·enabled·visible는 Core Observable
이면 위젯 세대의 BindingScope에 구독한다. 일반 C++ 멤버는 구성 시 값으로 사용하며
임의의 C++ 대입을 자동 감지한다고 가정하지 않는다. shared_ptr::get을 Observable
접근으로 오인하지 않는다. method와 methods는 호출이 아닌 지연 참조이고 자식은
같은 백엔드를 재사용한다. 반응형 패딩·치수 등 일반 상속 속성은 여전히 남아 있다.

## 범위와 검증

원본 전체의 부분 이식이다. 상속된 spring/색상 모션, 입력 방식별 포커스 링·outset,
장기 누름·반복·모든 Qt hit-test/자식 overflow 예외, named method trigger registry,
모든 signal 순서, 전체 reactive/inherited API와 플랫폼 접근성은 남아 있다.
모바일/WASM 위젯 호스트와 Windows/Linux 실제 빌드·실행도 별도 증거가 필요하다.

tests/native/link.cpp에는 기하·경로·별칭·replace·컴포넌트·전역/만료 대상·자식·입력·
색상/비활성 tone·Core 바인딩·예산·메서드·실패·재진입·소유자 종료를 등록한다.
두 페이지 AOT 예제는 Target 43 버튼으로 링크 목적지를 바꾼 뒤 탐색하는 self-test와
자식 링크를 포함한다. iiGUI ActionSurface·밑줄 검증과 함께 모든 앱·테스트 실행은
사용자 지시에 따라 전체 작업의 최종 일괄 단계까지 미룬다. 현재 컴파일·링크·설치
소비자 정적 증거를 실행 통과나 완전한 원본 동등성으로 표시하지 않는다.

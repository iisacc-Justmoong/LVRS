# 네이티브 페이지 라우터

`lvrs::PageRouter`는 경로와 탐색 이력을 실제 iiGUI 콘텐츠 세대에 연결한다.
LVUI는 페이지를 빌드 시 C++로 생성하고 `PageComponent`는 이미 컴파일한 생성
함수를 보유한다. 런타임은 소스나 위젯 명령 언어를 해석하지 않는다. 같은 창과
자체 렌더링 표면에서 페이지를 교체하며 Cocoa·Win32 컨트롤을 생성하지 않는다.

## 구성과 소유권

```cpp
auto home = std::make_shared<const lvrs::PageComponent>(
    [](ii::gui::WidgetBuilder& widgets, ii::gui::Widget parent,
       const lvrs::NavigationEntry& context) -> ii::core::Result<void> {
        return lvui_generated::HomePage::build(widgets, parent,
            [&](auto& scope) { scope.backend_0.path.set(context.path); });
    });
auto router = lvrs::PageRouter::create({.routes = {{"/", home}}});
```

`PageRouter::mount` 또는 `Widgets::page_router`는 빌더에 일반 콘텐츠 호스트를
하나 구성하고 초기 경로를 활성화한다. LVUI에서는 다음과 같이 스코프가 소유한
공유 컨트롤러를 전달한다. 같은 컨트롤러를 동시에 두 위치에 마운트할 수 없다.

```text
import vm Backend
Window {
    PageRouter(id: "pages", router: vm.router, width: 700, height: 400)
}
```

`PageRouterOptions`는 선언 순서의 `routes`, `initial_path`, `initial_params`,
`not_found_component`, `route_resolve_cache_capacity`를 받는다. 경로는
RouteMatcher로 정규화하며 정적·이름·나머지 세그먼트를 지원한다. 경로 매개변수는
Core Value 객체이고, 호출자가 명시한 키가 캡처한 문자열보다 우선한다. 직접
컴포넌트를 지정하는 탐색은 Core Value의 다른 타입도 보존한다.

`navigator`로 명시적인 공유 탐색 도메인을 지정할 수 있다. 기본값은 소유 스레드의
Navigator이며 `register_as_global_navigator`의 기본값은 true이다. 초기 페이지
구성 전에 등록하고 위젯 스코프 종료 때 lease를 해제한다. 컨트롤러가 외부에 남아
있어도 닫힌 화면은 등록에서 제거된다. [Navigator](Navigator.md)에 형제 순서·
도메인·약한 소유권과 명령 반환값을 정리한다.

구성 이후 경로 목록은 고정이다. 페이지 생성 함수는 비어 있을 수 없다. URL·QML·
JavaScript 소스 경로는 페이지 생성 함수 대신 받지 않는다. AOT 생성 뷰의 `build`
는 기존 빌더에 본문을 생성하고 독립된 Scope와 BindingScope를 retain한다.
`Window`의 창 옵션은 이 경로에서 새 창을 만들지 않는다. 페이지의 팝업 초기
`open: true`는 거부하며 활성화 후 명시적으로 연다.

## 탐색과 트랜잭션

| 공개 연산 | 이력 동작 |
|---|---|
| `go` / `push` | 확인한 경로 페이지를 추가한다. |
| `replace` | 현재 항목만 교체한다. |
| `set_root` | 이력을 하나의 루트로 바꾼다. |
| `go_to` | 컴파일한 PageComponent를 직접 추가한다. |
| `replace_with` / `set_root_component` | 직접 지정한 컴포넌트로 현재/루트를 교체한다. |
| `back` / `pop` | 루트를 남기고 이전 항목으로 돌아간다. |
| `pop_to_root` | 첫 항목만 남긴다. |

후보의 매개변수·이력 예산을 먼저 검사한 뒤 새 위젯 세대를 구성한다. 구성이
성공하면 비공개 NavigationStackModel에 명령 하나를 커밋하고 새 페이지를 공개한다.
그다음 이전 세대를 종료하고 탐색 알림을 전달한다. push마다 전체 이력을 복사하지
않는다. `path()`로 명시적으로 이력 사본을 요청할 때만 전체 목록을 복사한다.

없는 경로, 팩토리 반환 오류·예외, 예산 초과, 잘못된 연산은 기존 화면과 이력을
유지한다. fallback을 구성했다면 요청한 경로를 보존한 채 해당 페이지를 구성한다.
상위 호스트를 종료하는 사용자 부수 효과까지 되돌리지는 않는다. 종료 중에는
`closed`, 생성·알림 중 경쟁 변경은 `busy`, 다른 스레드의 호출은
`wrong_executor`이다. 이전 페이지 소멸자가 호스트를 종료하는 경우도 오래된
노드에 접근하지 않고 종료 결과를 반환한다.

## 페이지 수명과 관찰

현재 구현은 활성 페이지 한 세대만 보유한다. 이전 항목의 경로·매개변수·컴포넌트
정체성은 이력에 남지만 위젯과 뷰 스코프는 종료한다. 뒤로 갈 때 새 스코프를
구성한다. 장기 보존할 문서·편집 데이터는 앱 모델에 둔다. 단순 페이지 이동은
파일 수정 이력이나 문서 저장을 의미하지 않는다.

`state()`는 탐색 revision·depth·현재 경로/매개변수, 마운트 여부, 뒤로 가기 가능
여부와 콘텐츠 세대를 반환한다. `current_page_item()`과 선택적 지역 ID로 현재
페이지 핸들을 얻는다. 오래된 페이지 핸들은 교체 이후 `closed`이다. 페이지
루트는 호스트 뷰포트를 채우고 `set_extent`와 상위 스택의 레이아웃을 따른다.

`observe`와 `observe_failures`는 약한 Core QueryPort를 등록한다. 성공 알림은
일관된 새 페이지/이력 공개 이후 전달한다. 수신자는 상태를 읽을 수 있고 재진입
변경은 거부한다. 성공 수신자가 실패하면 `failed`와 cause를 반환하지만 이미
커밋한 탐색은 유지한다. 재시도로 같은 명령을 중복 적용하지 않는다. 실패 알림의
수신 오류는 원래 탐색 오류를 대체하지 않는다.

위젯 세대 종료는 연결·바인딩·retain 객체 수명을 끝낸다. 임의 C++ 백엔드가 외부
스레드에 이미 넘긴 작업을 자동 취소한다는 뜻은 아니다. 작업 취소와 결과 전달은
백엔드가 소유한 Core 작업 스코프·취소 계약과 UI 실행기로 구성한다.

## 남은 동등성 범위와 검증 상태

기존 PageRouter 전체의 부분 구현이다. 비활성 페이지 캐시와 복원, 반응형 경로
목록 교체·외부 path 설정, Navigator의 대화형 위임, Link의 전체 상속 계약, ViewModels 소유권 연결, 대화형
전환·제스처·모션과 전체 신호 순서, 모바일/WASM 위젯 호스트는 아직 남아 있다.
임의 QML source나 QVariant 변환을 흉내 내어 성공으로 처리하지 않는다.

논리 추적 항목은 view_tracking_entries로 제공하고 선택된 Navigator의
[ViewStateTracker](NavigationTracking.md)에 동기화한다. PageMonitor 방문 기록 연결은
원본처럼 앱이 명시적으로 구성하며 실제 페이지 캐시와 구분한다.

`tests/native/page_router.cpp`에는 수명·타입 보존·실패 원자성·관찰·재진입·fallback·
입력·크기 변경·스레드·예산·생성/소멸 중 호스트 종료를 등록한다. 별도의
`example/NativePageRouterGallery`는 두 AOT 페이지와 Core 앱의 프레임·작업 결과·
크기 변경·정상 종료 경로를 포함한다. 현재 사용자 지시에 따라 코드 생성·컴파일·
링크·패키징과 설치 소비자 정적 검사만 수행하며 앱과 테스트 실행은 최종 일괄
단계로 보류한다. 빌드 성공을 동작 검증이나 전체 기능 완성으로 표시하지 않는다.

# 네이티브 전역 탐색

`lvrs::Navigator`는 기존 Navigator.qml의 등록 스택과 탐색 위임을 C++23으로 구현한다.
구체적인 PageRouter·Qt 객체·OS 창 대신 `NavigationTarget` 인터페이스의 상태·명령·
약한 Core 포트에 의존한다. OS 창을 활성화하거나 키보드 포커스를 가져오지 않는다.

## 도메인과 등록 수명

`Navigator::current()`는 소유 스레드의 기본 공유 인스턴스이다. 앱·독립 편집기·창의
탐색을 분리하려면 명시적으로 생성해 PageRouterOptions의 `navigator`에 전달한다.

```cpp
auto navigator = std::make_shared<lvrs::Navigator>();
auto router = lvrs::PageRouter::create({
    .routes = {{"/", home}, {"/reports/[id]", report}},
    .navigator = navigator,
    .register_as_global_navigator = true
});
```

PageRouter는 기본적으로 초기 페이지 구성 전에 등록한다. 따라서 그 페이지 안의
중첩 라우터는 부모 뒤에 등록된다. 등록 옵션이 false이면 직접 호출만 가능하다.
컨트롤러와 등록 lease를 마운트에 retain하므로 앱이 컨트롤러를 별도로 보관하더라도
위젯 스코프가 닫히면 전역 등록은 해제된다.

일반 등록은 마지막 대상이 활성 대상이다. 같은 부모의 라우터는 실제 형제 순서를
유지한다. 나중 형제가 먼저 등록되어도 앞선 형제의 재등록이 활성 대상을 바꾸지
않는다. 다른 부모는 원본처럼 스택 뒤에 추가한다. iiGUI `Widget::placement()`가
반환하는 약한 세대 정체성·부모 키·형제 순서를 NavigatorPlacement에 전달한다.
동일한 문자열 ID를 가진 서로 다른 창이나 세대를 같은 부모라고 취급하지 않는다.
배치를 생략하면 대상의 `placement()`에서 조회한다. PageRouter는 실제 iiGUI
배치를 반환하므로 수동 재등록도 같은 규칙을 따른다. 시각 트리가 없는 교체 대상은
기본적으로 그룹 없는 배치를 사용하거나 메타데이터를 재정의할 수 있다.

`register_router(target, placement)`는 대상을 약하게 등록한다.
`track_router(target, lease, placement)`는 등록 자체의 수명을 lease에 묶는다.
새 scoped 등록은 새 정체성이므로 이전 lease의 종료가 새 등록을 제거하지 않는다.
기존 scoped 대상의 단순 register_router 재등록은 스코프 수명을 유지한다.
`unregister_router(target)`는 해당 대상, 인자를 생략하면 전체 등록을 제거한다.
최대 대상 수는 4096개이다. 만료된 대상·닫힌 활성 대상·만료 lease는 접근 경계에서
정리한다. 조회에서 반환한 공유 포인터를 보관하는 것은 호출자의 명시적인 수명 연장이다.

명령과 명시적인 lease `close()`는 소유 스레드에서 호출한다. 다른 스레드에서 lease가
소멸하면 약한 생존 토큰만 만료시키고 다음 소유 스레드 접근 때 정리한다. 작업자에서
UI 객체를 정리하지 않는다. 일반적인 위젯 종료는 소유 스레드에서 등록을 해제한다.

## 명령과 관찰

`go`, `replace`, `set_root`, `go_to`, `replace_with`, `set_root_component`, `back`,
`pop_to_root`를 활성 대상에 전달한다. Core Value와 컴파일한 컴포넌트 정체성을
보존한다. 빈 경로·null 컴포넌트·활성 대상 없음은 false이다. bool은 원본처럼 위임이
받아들여졌는지를 의미한다. 루트에서 back처럼 대상의 정상 무동작도 위임은 true이다.
구체적인 PageRouter의 bool은 이력 변경 여부이므로 이 둘을 구분한다. 대상의 실제
오류와 예외는 Result와 cause로 전달하며 페이지 구성 실패를 성공으로 바꾸지 않는다.

`state()`는 revision·등록 수·활성 여부·현재 대상 상태를 반환한다. `router()`,
`router_stack()`, `can_navigate()`도 제공한다. `observe(weak_ptr<NavigatorPort>)`는
등록 목록과 현재 상태 변경을 구분한다. 대상에 직접 명령을 내려도 추적하며 서비스로
위임한 명령의 변경은 명령 경계에서 모은다. 무동작과 비활성 대상만의 변경은 현재
화면 갱신 통지를 만들지 않는다.

명령·통지 중 경쟁 변경은 busy, 다른 스레드는 wrong_executor이다. 진행 중인 호출은
내부 상태를 소유하므로 콜백에서 Navigator 소유자를 해제해도 안전하게 반환한다.
외부 호출을 가로질러 벡터 원소 참조를 보관하지 않는다. 통지 중 lease 종료는 퇴역
표시 후 다음 명령·조회 경계에 반영한다. 관찰자의 연쇄 변경을 무한히 재귀 실행하지
않는다. 조회는 마지막으로 공개한 일관된 스냅샷을 사용한다.

공개 이후 수신자가 실패하면 failed와 cause를 반환해도 커밋한 등록·탐색은 유지한다.
track_router는 통지 전에 출력 lease를 넘겨 이 오류로 등록 소유권을 잃지 않게 한다.
같은 명령을 무조건 재시도하지 말고 현재 상태를 확인한다. PageRouter 성공 포트는
읽기 가능한 통지 경계에서 추가 구독할 수 있으며 진행 중인 통지에 소급되지 않는다.

## 남은 범위와 검증

선택된 대상의 논리 스택은 도메인별 [ViewStateTracker](NavigationTracking.md)에 동기화한다.
대화형 전환의 진행·속도·미리보기·커밋/취소 위임, ViewModels 소유권과 모든 원본 신호
순서의 동등성은 남아 있다. [네이티브 Link](Link.md)는 자식 슬롯·텍스트 fallback·
밑줄·상태 색상·명시적/전역 대상과 Core 바인딩을 별도 구성 요소로 구현했다.
Link의 전체 상속 동작·모션·플랫폼 검증은 계속 남아 있다.

`tests/native/navigator.cpp`에는 비시각 교체 대상의 순서·위임·컴포넌트·약한 수명·
lease·관찰·직접 대상 통지·소유자 종료·스레드·예산·예외 사례를 등록한다.
`navigator_gui.cpp`는 실제 라우터의 형제·기본/분리 도메인·등록 제외·중첩·언마운트를
다룬다. 두 페이지 AOT 예제도 공유 Navigator를 사용한다. 현재는 코드 생성·컴파일·
링크·설치 소비자 정적 검증만 수행하며 모든 테스트·앱·self-test 실행은 사용자 지시에
따라 전체 작업의 마지막 일괄 단계로 보류한다.

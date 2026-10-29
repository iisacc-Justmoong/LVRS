# 네이티브 경로 해석과 탐색 스택

`RouteMatcher`, `RouteResolver`, `NavigationStackModel`은 기존 Qt 객체의 경로·탐색
로직을 C++23 값과 iiSDKCore 결과·통지 계약으로 옮긴 객체이다. 헤더와 구현은
`src/LVRS/`에 함께 있다. 공개 API에 Qt·QML·OS 위젯 자료형이 없다.
토큰화·공백 처리·매칭 내부 구현은 `RoutingDetail.h/.cpp` 쌍에 두고 공개 matcher와 분리한다.

현재 단계의 앱·단위 테스트·설치 소비자 실행은 사용자의 지시에 따라 전체 작업의 최종
일괄 검증까지 유보한다. 테스트 소스, 컴파일·링크, AOT 코드 생성과 패키지 정적 검사를
실제 입력·화면 표시·계측 실행의 통과로 간주하지 않는다.

## 경로와 매개변수

```cpp
auto result = lvrs::RouteMatcher::match("/reports/42", "/reports/[id]");
// 성공한 Result의 matched == true, params.at("id") == "42"
```

경로는 UTF-8 문자열이다. 앞뒤 Unicode White_Space를 제거하고, 비어 있으면 `/`,
첫 문자가 `/`가 아니면 접두어를 추가한다. 루트가 아닌 경로의 뒤쪽 `/`는 모두 제거한다.
내부의 연속 `/`, 대소문자와 Unicode 조합 형태는 그대로 보존한다. URL 파싱·percent
디코딩·query/fragment 분리·파일 경로 정규화·접근 권한 검사를 수행하는 객체가 아니다.
NUL과 잘못된 UTF-8, 최대 바이트 수와 세그먼트 수를 넘는 값은 거절한다.

원본 구현의 매칭 규칙을 유지한다.

- 리터럴 세그먼트는 정확하게 일치해야 한다. 루트 패턴은 루트 경로만 매칭한다.
- `[id]`는 한 세그먼트를 문자열로 캡처한다. 내부의 빈 세그먼트도 캡처할 수 있다.
- `[...path]`는 남은 세그먼트를 `/`로 연결한다. 빈 나머지도 매칭한다.
- 나머지 캡처를 만나면 즉시 성공한다. 그 뒤의 패턴 세그먼트는 원본처럼 검사하지 않는다.
- 빈 캡처 이름을 허용한다. 같은 이름이 반복되면 마지막 캡처를 유지한다.
- 매칭이 실패하면 중간에 수집한 매개변수를 노출하지 않는다.

Unicode 공백 처리는 명시적인 White_Space 집합을 사용한다. 서로 다른 Qt/Unicode
버전의 분류까지 동일하다고 주장하지 않는다. 기존 동작의 경계 사례는 테스트에 남긴다.

## 선언 순서와 캐시

```cpp
lvrs::RouteResolver resolver;
auto configured = resolver.set_route_patterns({"/", "/reports/[id]", "/logs/[...path]"});
auto resolved = resolver.resolve("/reports/42");
```

패턴은 등록할 때 한 번 정규화하고 리터럴·매개변수·나머지 토큰으로 컴파일한다.
요청 경로는 한 번 분할한 뒤 선언 순서대로 검사한다. 더 구체적인 패턴의 자동 우선순위는
없다. 원본의 빈 문자열 패턴은 비활성 슬롯이며 뒤 패턴의 index를 당기지 않는다.
공백만 있는 비어 있지 않은 패턴은 정규화 규칙에 따라 루트가 된다.

`resolve`는 `shared_ptr<const RouteResolution>`을 반환한다. 매칭 여부·원본 index·캡처
값을 함께 소유한다. 캐시 적중 시 같은 불변 결과를 공유하므로 매개변수 맵을 재복사하거나
패턴을 다시 해석하지 않는다. 캐시에서 제거되거나 resolver가 해제되어도 호출자가 소유한
결과는 유효하다. 미매칭 결과도 같은 방식으로 캐시한다.

캐시는 hash 조회와 연결 목록 이동을 사용한다. 적중 시 선형 목록 제거를 하지 않는다.
기본 용량은 256이며 원본처럼 최소 16으로 제한한다. 네이티브 상한은 65,536이다.
패턴 변경은 기존 캐시를 비우며 용량 축소는 가장 오래 사용하지 않은 항목부터 제거한다.
`clear_cache`는 결과 캐시만 비우고 설정 revision과 통계 누적값을 되돌리지 않는다.
`state()`의 hit/miss/pattern_matches는 재매칭 여부를 확인할 수 있는 진단 값이다.

`RouteLimits`에는 경로당 65,536바이트·4,096세그먼트, 최대 65,536패턴·총 8 MiB
원본 패턴·262,144토큰, 캐시 계산 예산 8 MiB를 둔다. 계산 예산은 키와 결과의 비용을
관리하는 값이며 allocator 메타데이터와 외부에서 계속 소유한 결과까지 포함한 프로세스
실제 메모리 상한을 의미하지 않는다. 잘못된 새 설정은 기존 패턴과 캐시를 교체하지 않는다.

## 탐색 스택과 컴포넌트 식별자

```cpp
lvrs::NavigationStackModel navigation;
auto root = navigation.apply_path_operation("/", {}, lvrs::NavigationOperation::set_root);
auto pushed = navigation.apply_path_operation("/reports/42");
auto current = navigation.state();
```

`NavigationEntry`는 정규화한 path, Core `Value` params, 이미 컴파일한 C++ 컴포넌트의
공유 식별자, view_id와 enabled/disabled 값을 가진다. 컴포넌트는 불투명한 소유 객체의
식별자이다. 문자열 소스나 바이트코드를 평가하지 않으며 이 객체 자체가 위젯을 생성하지
않는다. 빈 경로의 명시적인 엔트리와 컴포넌트 엔트리를 구별해 보관할 수 있다.

`NavigationOperation`은 push·replace·set_root이다. 빈 스택에서 모든 연산은 하나의
루트 엔트리를 만든다. pop과 pop_to_root는 마지막 루트를 제거하지 않는다. 명시적인
`set_path({})`는 스택 전체를 비울 수 있다. 유효한 no-op은 false이며 revision이나 통지를
만들지 않는다. 이 이력은 앱의 화면 상태이며 파일·문서 수정 revision이나 undo 기록이 아니다.

`stack_after_*`는 명시적인 복사본을 반환하는 미리보기이며 현재 모델을 바꾸지 않는다.
`validate_entry_operation`과 `apply_entry_operation`은 `view_id`와 enabled/disabled를
포함한 완전한 `NavigationEntry` 한 개를 검사·적용한다. PageRouter가 확정한 모델 연결 ID를
이력에 보존할 때 사용한다. 기존 path/component 편의 함수와 같은 자원 예산과 변경 장벽을
적용하며, 전체 스택 교체가 필요하지 않다.
실제 push는 스택 전체를 복사하지 않고 새 엔트리와 추적 값만 추가한다. 저장 공간은
기하급수적으로 늘리고, replace는 끝 엔트리만, pop은 제거하는 꼬리만 갱신한다.
`set_path` 전체 교체와 명시적인 미리보기·스냅샷은 입력 크기에 비례한 작업이다.

params는 Core `Value`의 타입을 보존한다. null을 기본 빈 Object로 바꾸며 그 외 타입은
보존한다. 일반 QVariant/QJSValue·QObject 속성 조회·문자열의 암묵적 truthiness 변환은
새 API에 포함하지 않는다. params의 disabled 플래그는 명시적인 bool true만 적용한다.

`NavigationLimits`는 최대 65,536엔트리, 엔트리당 계산 예산 1 MiB와 전체 스택 16 MiB,
Value 깊이 64와 노드 65,536을 둔다. `view_id`는 경로와 같은 최대 바이트 수이다.
예산 검증 실패는 기존 이력을 바꾸지 않는다. 호출자가 인자를 생성하거나 복사하는 데
이미 쓴 메모리와 allocator 구현까지 제한하는 기능은 아니다.

## 추적 값과 통지

`view_tracking_entries`는 각 엔트리의 view_id·path·enabled를 반환한다. view_id를
공백 제거한 값이 비어 있으면 경로를 사용하고, 경로도 없으면 `_component_<index>`를
사용한다. disabled나 명시적인 bool params.disabled는 enabled보다 우선한다.

`update_tracked_view_ids`는 첫 출현 순서를 유지해 중복을 제거하고, 이전 목록에서
사라진 ID를 이전 순서대로 반환한다. 원본처럼 추적 ID 목록은 스택의 파생 tracking
목록과 별도로 명시적으로 동기화한다. 비활성 ID도 이 목록에서 임의로 제거하지 않는다.

두 모델은 생성 스레드가 소유한다. 다른 스레드 접근은 wrong_executor이다. observer는
Core QueryPort의 약한 참조로 등록한다. 통지 전에 상태와 revision을 확정하고 같은
변경을 한 번 전달한다. 수신자는 통지 중 완성된 상태를 읽을 수 있으나 다시 변경하거나
observer를 추가하는 시도는 busy이다. 다른 스레드의 작업 결과는 Core 실행 컨텍스트에
전달한 다음 소유 스레드에서 반영해야 한다.

탐색 변경의 path·current·tracking·tracked_ids는 서로 다른 플래그이다. current는
현재 path/params의 변화이며 컴포넌트 식별자만 바뀐 경우에는 path 변경으로 구별한다.
수신자 예외는 실패 Result와 cause로 전달하지만 이미 적용한 변경을 되돌리지 않는다.
따라서 통지 오류를 거절된 명령으로 해석해 같은 push를 다시 실행해서는 안 된다.

컴포넌트를 제거하는 중에는 부분 적용 상태의 조회·변경도 busy로 막는다. 통지에서 공개
모델 객체의 마지막 소유자가 해제되더라도 진행 중인 명령은 내부 상태의 소유권을 유지한다.
이 수명 계약의 ASan/UBSan 실행 검증은 최종 단계에 남겨 둔다. `path_view`와
`route_patterns`의 span은 같은 스레드에서만 읽으며 다음 변경이나 모델 해제 후 보관하지 않는다.

## 빌드된 LVUI 예제와 화면 호스트

`example/NativeNavigationGallery`는 AOT로 생성한 LVUI 버튼이 scope 소유 C++ backend를
호출하고, native resolver와 탐색 스택을 갱신하는 앱이다. Core 변경 포트가 경로·깊이·
매개변수 레이블을 갱신한다. Core 작업자 결과·디스플레이 프레임·크기 변경·정상 종료의
self-test도 작성했다. 앱 소스는 예제의 `App/`에 있으며 backend 선언과 구현도
`Backend.h/.cpp`로 같은 위치에 둔다.

이 예제는 경로·이력 모델의 실제 GUI 연결을 보여 준다. 라우트별 페이지를 생성·폐기하는
`PageRouter` 화면 호스트를 구현한 예제가 아니다. 독립 페이지 생성·폐기·뒤로 가기와
실패 시 화면/이력 보존은 별도의 [네이티브 PageRouter](PageRouter.md)와
`example/NativePageRouterGallery`에서 구현한다. 다음 계약은 전체 이식 범위에 남아 있다.

- 비활성 페이지 보관·복원 정책과 반응형 route/path 설정
- PageRouter 상속 API·전체 route role adapter, Navigator의 대화형 위임·Link 연결
- 전환·대화형 back/push 제스처·미리보기·모션과 inactive 페이지 접근성·입력 격리
- ViewModels 소유권 연결, 일반 반응형 속성과 모든 원본 signal 발생 순서
- 전체 Unicode/암묵적 값 변환 호환성, 모든 플랫폼 빌드·입력·실행·설치 소비자 실행

위 모델이나 예제를 완성된 PageRouter로 표시하지 않는다. 정적·계측 빌드와 최종 실행
증거는 각각 별도 기록으로 남긴다.

선택된 네이티브 PageRouter의 추적 항목은 Navigator의 ViewStateTracker에 동기화한다.
별도의 PageMonitor와 명시적 방문 기록 연결은 [탐색 추적 계약](NavigationTracking.md)을 따른다.

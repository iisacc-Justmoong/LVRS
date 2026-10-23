# 네이티브 방문 기록과 뷰 상태 추적

`PageMonitor`와 `ViewStateTracker`는 기존 동명의 QObject 모델을 Qt 없는 C++23
객체로 구현한다. 문자열은 UTF-8, 구조 데이터는 iiSDKCore Value, 변경 통지는
약한 Core QueryPort이다. OS 창·타이머·작업자 스레드를 만들지 않는다.

## 방문 기록

PageMonitor는 방문 경로 기록이며 NavigationStackModel의 탐색 스택과 별개이다.
`record`는 Unicode White_Space를 앞뒤에서 제거하고 빈 값·연속 중복을 무시한다.
슬래시를 추가하거나 URL을 해석하지 않는다. 비연속 재방문은 새 기록이다.
`undo`는 두 개 이상일 때 마지막 기록만 제거하고 현재 경로 문자열을 반환한다.
첫 기록은 유지하며 clear는 모두 비운다. 어느 메서드도 실제 탐색을 수행하지 않는다.

state는 revision·count·current·can_undo, history는 소유한 복사본, history_view는
다음 변경까지 유효한 읽기 전용 span이다. append는 기존 방문 기록 전체를 복제하지
않으며 변경된 경우에만 통지한다. current()는 소유 스레드의 기본 인스턴스이고
직접 생성한 객체들은 독립적이다.

원본처럼 PageRouter와 자동 연결하지 않는다. 앱이 PageNavigationPort의 커밋된
경로를 record에 전달하는 방식으로 명시적으로 연결한다. 예제의 Undo visit은
방문 기록을 되돌리지만 화면·라우터·문서 데이터는 변경하지 않는다.

## 뷰 스택과 상태

ViewStackInput은 view_id·path·enabled·disabled를 받는다. ID와 경로는 앞뒤 공백을
제거한다. ID가 없으면 경로를 사용하고 둘 다 없으면 `_component_0`부터 익명
항목만 순서대로 번호를 매긴다. 경로 자체를 URL이나 라우팅 경로로 재해석하지 않는다.

원본과 같이 마지막으로 유효 활성화된 항목 하나가 Active, 그 이전의 활성 가능한
항목은 Inactive, base enabled가 false이거나 ID override가 있으면 Disabled이다.
같은 ID가 여러 번 있어도 순서를 보존하고 view/state_of/is_loaded는 마지막 항목을
조회한다. ID override는 중복된 모든 항목에 적용한다. set_view_enabled는 override를
해제하는 기능이며 원본 입력의 enabled=false·disabled=true를 강제로 바꾸지 않는다.

아직 없는 ID의 비활성화 정책도 다음 sync를 위해 보관한다. 이때 레코드가 바뀌지
않으면 revision·통지는 발생하지 않으며 반환값은 false이다. sync는 override를
유지하고 clear는 레코드와 override를 모두 제거한다. override만 있던 clear도 원본처럼
통지한다. 동일한 레코드를 다시 sync하면 새 revision을 만들지 않는다.

state는 개수와 현재 활성 ID, stack/stack_view는 레코드와 읽기 전용 span,
loaded_views/active_views/inactive_views/disabled_views는 순서를 유지한 목록이다.
snapshot은 한 시점의 typed 레코드와 파티션을 소유하며 snapshot_value는 원본의
currentActiveView·loadedViews·activeViews·inactiveViews·disabledViews·stack 키와
각 레코드의 viewId·path·state·stateCode·index·loaded·enabled·active를 Core Value로
노출한다. 동적 소스 평가나 JSON 파서를 모델에 내장하지 않는다.

sync_value는 Core Value의 객체 배열과 명시적인 문자열·bool을 받는다. 잘못된 자료형을
QVariant의 truthiness나 문자열 변환으로 추정하지 않고 오류를 반환한다. sync_navigation은
NavigationStackModel이 이미 계산한 NavigationViewEntry를 직접 받는다.

## Navigator와 실제 페이지 연결

NavigationTarget의 선택적 view_tracking_entries 계약은 논리 스택을 제공한다.
기본 구현은 빈 목록이고 PageRouter는 자신의 NavigationStackModel 추적 항목을 반환한다.
Navigator는 선택된 대상의 변경·등록·복귀·해제 때 자신의 ViewStateTracker를 동기화한다.
비활성 대상의 직접 탐색은 선택된 추적 스택을 덮어쓰지 않는다. 대상이 구성 중이면
동기화를 보류하고 다음 flush에서 다시 시도한다.

Navigator()는 독립 추적기를 생성하고 생성자에 추적기를 주입할 수도 있다.
Navigator::current()는 ViewStateTracker::current()와 같은 소유 스레드 기본 추적기를
사용한다. view_state_tracker()로 해당 도메인의 객체에 접근한다. 서로 다른 명시적
Navigator가 기본적으로 추적 상태를 공유하지 않는다.

선택 상태를 커밋한 후 추적기 통지, Navigator 통지 순서로 전달한다. 관찰자의 오류는
이미 커밋한 탐색·등록·추적 상태를 되돌리지 않으며 Result의 cause로 반환한다. 다른
관찰자도 계속 호출한다. 완성되지 않은 대상의 busy/closed 응답과 실제 오류를 구분한다.
이 순서는 네이티브 계약이며 모든 원본 QML signal 순서를 재현했다는 뜻은 아니다.

여기서 loaded는 **공급한 논리 스택에 포함됨**을 뜻한다. 현재 PageRouter가 보존하는
실제 위젯 세대는 활성 페이지 하나이며, 과거 항목을 추적했다고 비활성 화면이나 입력
상태를 캐시한 것은 아니다. ViewModels의 모델 소유권·쓰기 권한 연결도 별도 범위이다.

## 자원과 수명

모델의 읽기·쓰기는 소유 스레드에서 수행한다. 통지 중에는 읽기를 허용하고 자기 모델의
변경은 busy로 거절한다. 관찰자에서 모델 소유자가 해제되어도 진행 중인 내부 상태와
통지 수신자를 보관한다. 수신자는 약하게 등록하며 같은 port 중복 등록은 무시한다.
닫힌 port는 전달 때 정리하며 최대 256개이다.

경로·ID는 각각 64 KiB 이하 UTF-8·NUL 없음, 스택/방문 기록은 65,536개 이하와
16 MiB의 논리 데이터 예산을 적용한다. disabled override도 별도 개수·문자열 예산을
가진다. 유효성·예산 실패는 기존 상태를 보존한다. 컨테이너의 할당 오버헤드를 포함한
프로세스 RSS 상한이나 임의 메모리 고갈의 무예외 처리를 보장하는 수치는 아니다.

전체 스택 sync는 O(n)이다. ID 조회는 인덱스를 사용하고 override는 관련 중복 항목과
기존/새 활성 위치만 변경한다. 스택 전체를 복사하거나 모든 상태를 다시 계산하지 않는다.
is_loaded와 state_of는 레코드·경로 전체를 복사하지 않고 요청한 값만 읽는다.
Navigator의 전체 추적 스냅샷 동기화는 대상 스택 크기에 비례하며 증분 전송 최적화와
구분한다. 이름에 loaded가 있어도 실제 위젯·GPU 자원 소유권을 추론하지 않는다.

## 검증 범위와 남은 작업

PageMonitor 12개·ViewStateTracker 17개·Navigator 추적 4개와 실제 PageRouter 추적
연결 1개 검사 사례를 등록한다. 두 페이지 LVUI 예제는 방문/논리 뷰 수를 표시하고
독립 방문 undo를 제공한다. 사용자 지시에 따라 앱·테스트·self-test 실행은 전체 작업의
최종 일괄 단계로 미룬다. 중간 빌드·계측 빌드·설치 소비자 정적 검사와 실행 증거를
혼합하지 않는다.

전체 QObject 상속/신호·QVariant 강제 변환·QML singleton 접근, ViewModels 소유권,
비활성 페이지 캐시/복원, 대화형 전환과 모든 플랫폼 실행·접근성 동등성은 남아 있다.
두 객체가 지원하는 typed C++ API와 실제 예제 연결을 부분 구현으로 기록한다.

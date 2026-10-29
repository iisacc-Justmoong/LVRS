# 네이티브 ViewModels 레지스트리

원본 `src/backend/state/viewmodelregistry.*`의 등록·뷰 바인딩·쓰기 권한·속성 접근·
라우트 메타데이터 규약을 C++ `ViewModelRegistry`로 제공한다. `ViewModels`는 이 타입의
별칭이며, LVUI scoped import마다 독립적인 인스턴스를 만든다. 숨겨진 Qt singleton이나
QML 엔진을 만들지 않는다. 앱이 여러 뷰에 공유할 레지스트리는 C++ 스코프에서 명시적으로 소유한다.

## 모델 인터페이스

`ModelProperties`는 실제 모델의 `read_property`, `write_property`, `model_descriptor`에
접근하는 교체 가능한 인터페이스이다. Core의 ObjectProperties는 등록 값 자체를 보관하는
저장소이므로 이미 존재하는 ViewModel 상태의 복제본을 만들지 않는다. 앱이 ObjectProperties를
기본 저장소로 쓰는 경우 이 인터페이스를 통해 해당 저장소의 쿼리를 위임할 수 있다.

ViewModel과 StateModel은 이 인터페이스를 구현한다. 프로퍼티 이름은 원본의 `displayName`,
`hasError`, `stateKeys` 같은 이름을 유지한다. 쓰기는 실제 C++ setter를 호출하며 타입을
엄격하게 검사한다. 알려진 동일 값의 쓰기는 성공 true, 미공개·읽기 전용 속성은 false이다.
없는 속성은 nullopt, 명시적인 null 데이터는 Core::Value의 null로 구별한다.

사용자 모델도 ModelProperties를 구현할 수 있다. 구현은 값 반영 이후 descriptor signal을
알리고, 레지스트리는 Core 관측·구독과 타입 지정 QueryPort 알림으로 이를 전달한다.
변경 signal은 약한 토큰이며 진행 중 호출에서 바깥 객체가 종료되어도 새 작업을 접수하지 않는다.

## 등록과 수명

`set`, `register_view_model`, `remove`, `clear`, `get`을 제공한다. 토큰은 Unicode 공백을
제거하며 UTF-8을 검증한다. 처음 등록한 ViewModel의 key가 비어 있으면 등록 키를 설정한다.
register_view_model의 fallback key는 모델 key보다 우선한다. 별칭들은 같은 모델을 가리킬 수 있다.

기본 등록은 borrow이다. 기존 스코프 소유권을 레지스트리로 이동시키지 않는다.
`ModelRetention::retain`을 명시하면 레지스트리가 모델을 보존하며 마지막 별칭 제거 시
해당 보존을 끝낸다. 같은 모델의 별칭 중 하나가 소유를 요청하면 다른 별칭만 남아도 보존한다.
스코프에 보관된 모델을 별칭 shared handle로 전달할 때에는 해당 모델의 실제 수명이
그 handle의 소유자에 포함되어 있어야 한다. 레지스트리 자신의 소유 스코프를 역으로 retain하지 않는다.

borrow 모델 종료 시 별칭·뷰 바인딩·쓰기 권한을 정리한다. 조회는 이를 정리하기 위해
파일 상태를 변경하거나 전체 백엔드를 실행하지 않는다. 기본 한도는 모델 65,536개,
뷰 65,536개, 토큰 8,192바이트, 후속 알림 256라운드이며 생성자에서 더 작은 한도를 정할 수 있다.
넘는 입력을 잘라내지 않고 접수 오류로 반환한다.

## 뷰와 쓰기 권한

`bind_view(view,key,writable)`, `unbind_view`, `get_for_view`, `key_for_view`,
`claim_ownership`, `release_ownership`, `can_write`, `owner_of`를 제공한다.
한 뷰는 하나의 키에 바인딩되고, 한 등록 키는 한 뷰에만 쓰기 권한을 준다. 별칭별 권한은
원본처럼 각각 관리한다. 다른 키로 이동하면 이전 키의 소유권을 해제하며, 읽기 전용으로
다시 바인딩하면 해당 뷰의 쓰기 권한을 해제한다. ownership 해제는 읽기 바인딩을 지우지 않는다.

등록 모델 교체는 기존 뷰와 소유권을 유지하면서 제공자를 교체한다. 이전 제공자의 알림은
더 이상 반영하지 않는다. 조회는 단일 키와 모델 변경 epoch를 추적한다. 모델 getter 내부
관측을 중복 수집하지 않아 한 모델 변경이 같은 descriptor를 두 번 무효화하지 않도록 한다.
단일 키 조회·바인딩·속성 쓰기는 전체 모델 descriptor를 생성하지 않는다.

`keys`, `views`, `bindings`, `owners`, `descriptor`, `descriptors`, `last_error`를
제공한다. descriptor에는 등록 key·className·owner·views, ViewModel의 상태 정보와
StateModel의 values·stateKeys·revision·empty를 담는다. 전체 descriptors는 명시적인
스냅샷 요청이며 일반 속성 갱신 경로가 이를 자동 생성하지 않는다.

## 라우트 메타데이터

`bind_route_view_model(path,route,params,fallbackIndex)`는 viewModelKey/modelKey,
viewId, writable/modelWritable을 처리한다. route가 params보다 우선하며, viewId가 없으면
path, 다시 없으면 `_component_N`을 사용한다. 모델 키가 없으면 기존 오류를 지우지 않고 false이다.
PageRouter는 아래 공통 경로 해석과 수명 정리 API를 통해 이 규칙을 사용한다.
새 페이지가 공개되기 전까지 모델 연결을 바꾸지 않으며, 화면 퇴역은 논리 이력의
마지막 참조를 기준으로 정리한다. 실제 실행과 원본 전체 동등성을 완료로 표시하지 않는다.

## 실행 영역과 검증

모델과 레지스트리는 소유 스레드에서 사용한다. 알림 중 레지스트리 재진입 쓰기는 busy,
다른 스레드의 쓰기는 wrong_executor이다. 반영 후 수신자 실패는 변경을 롤백하지 않고
다른 수신자에게 전달한 뒤 오류를 반환한다. 알림 중 발생한 모델 변경은 모델별로 모아 전달한다.
등록 준비 중 소유자가 종료되면 닫힌 레지스트리에 새 항목을 반영하지 않는다.

`tests/native/view_model_registry.cpp`와 `registry_aot.cpp`에 모델·권한·수명·종료·
생성 뷰·Core 값 표시 검사를 준비한다. LVRS 실행 검사는 전체 목표의 최종 일괄 단계에 남긴다.
Qt parent의 암묵적 채택·deleteLater 시점·동적 QObject 속성·QVariant 전체 변환 및
플랫폼 동등성은 명시적인 차이와 남은 감사 항목이다.

2026-10-27에는 일반·ASan/UBSan 구성에서 레지스트리 모델 검사 24개와 생성 뷰 조작
검사 6개를 담은 실행 파일을 컴파일·링크했다. 기존 모델·컨트롤·설치 소비자를 합친
관련 LVRS 검사 85개는 등록 상태이며 아직 실행하지 않았다. LVUI의 관련 검사 68개씩은
두 구성에서 통과했고, 팩터리 66개의 생성 C++는 두 구성에서 동일하다. 공개 헤더 여덟 개와
라이브러리 객체를 대조한 재배치 소비자는 아홉 팩터리를 포함하여 링크했다.
기록은 소스 저장소의 `build/lvui-completion/model-registry/verification-20261027.json`이다.
자동 PageRouter 연결과 전체 실행·플랫폼 동등성은 이 기록으로 완료되지 않는다.

## 라우터 연결과 스코프 종료

`route_binding(path, route, params, index)`는 모델 키가 없어도 일관된 화면 ID를
결정하는 순수한 값 변환이다. 레지스트리 등록·권한·에러를 바꾸지 않는다. 기존
`bind_route_view_model`도 이 규칙을 사용한다. PageRouter는 이 ID를 논리 이력에
보존하고 페이지 공개가 성공한 뒤 모델을 연결한다. 자세한 순서와 fallback 예외는
[페이지 라우터](PageRouter.md)에 정리한다.

`validate_view_admission`은 소유 스레드·명령 중 여부·정규화한 ID의 비어 있음과
바이트 예산만 검사한다. 에러 상태·등록·권한을 바꾸지 않는다. 모델 키와 소유권의
업무상 거절은 실제 연결 때 결정하므로, 현재 제거할 화면의 권한까지 미리 거절하지 않는다.

`retire_views(span)`는 삭제할 화면 ID만 받는 전용 수명 정리 명령이다. 중복과
없는 ID는 무해하고 다른 화면의 연결과 `last_error`를 보존한다. 입력 전체를 먼저
검사하며 한 번에 최대 65,536개 ID를 받는다. 레지스트리의 실제 view 용량보다 많은
미등록 ID가 섞여 있어도 실제 등록된 연결만 해제한다.

일반 명령/알림 중 호출하면 현재 연결에 대한 해제 요청을 모으고, 해당 명령의
커밋과 알림 뒤에 처리한다. 임의의 중첩 쓰기를 허용하는 기능이 아니다. 외부 모델
관찰자가 화면을 종료해도 원래 모델 변경은 보존하고 제거된 화면의 쓰기 권한은
남기지 않는다. 관찰자가 예외를 던져도 다른 대상과 수신자 정리를 계속하고, 명령을
소유한 호출자에게 커밋 이후 오류를 반환한다. 소유 스레드와 입력 예산은 그대로 검사한다.

새 수명 정리·공통 경로 해석과 PageRouter의 관련 검사는 작성·등록되며, 실제 실행은
저장소 지침에 따라 전체 작업의 마지막 일괄 검증에 남긴다.

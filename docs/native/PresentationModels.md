# 네이티브 ViewModel·StateModel·ProgressModel

원본은 `src/backend/state/viewmodel.*`, `statemodel.*`와
`src/backend/model/progressmodel.*`이다. 네이티브 API는 `src/LVRS/`에 같은 이름의
헤더·소스 쌍으로 제공한다. Qt 객체·QML 엔진·JS 배열이나 OS 컨트롤을 사용하지 않는다.

## 값과 실행 영역

메타데이터와 상태는 `ii::core::Value` 및 `Value::Object`의 소유 값이다. 읽기 결과와
스냅샷은 모델의 변경 가능한 내부 저장소를 차용하지 않는다. Core의 UTF-8·유한 수·
엄격한 타입 계약을 사용하며 임의의 QObject/QVariant 메타타입을 복제하지 않는다.
원본 QVariant와의 전체 변환·비교 동등성은 별도 감사 항목으로 남긴다.

모델은 생성된 소유 스레드에서 읽고 쓴다. 다른 스레드의 쓰기는 `wrong_executor`, 읽기는
예외로 거부한다. 변경 명령 중 재진입 쓰기는 `busy`이다. Core 관측과 타입 지정 QueryPort
알림을 사용하며, 알림 중 오류는 이미 반영된 값을 롤백하지 않고 다른 수신자에게 계속
전달한 뒤 반환한다. 뷰 상태 변경은 앱의 파일 저장·생성 큐를 자동 호출하지 않는다.

## ViewModel

`key`, `display_name`, `busy`, `error`, `has_error`, `metadata`와 `snapshot()`을 제공한다.
텍스트 setter는 Unicode 공백을 제거하고, 같은 값에는 알림을 발행하지 않는다.
`clear_error()`는 error 속성만 바꾼다. 각 속성의 Core 관측은 다른 속성과 분리된다.
기존 QML 이름의 `displayName`, `hasError`는 스냅샷 키에 유지한다.

## StateModel

ViewModel을 상속하며 `values`, `state_keys`, `revision`, `empty`, `value_or`, `has_value`,
`set_value`, `remove_value`, `apply_patch`, `clear_values`, `state_snapshot`을 제공한다.

- 키는 Unicode 공백을 제거한다. 키 목록과 키별 알림 순서는 UTF-16 순서로 정렬한다.
- 빈 키의 쓰기·삭제는 `false`와 `Empty state key` 오류 속성을 사용한다.
- 같은 값의 `set_value`와 같은 내용의 `apply_patch`는 성공 `true`이며 revision은 유지한다.
- 빈 patch는 `false`, 없는 키 삭제는 `false`, 존재하는 null은 없는 키와 구별한다.
- 변경 후 revision → values → 필요한 keys → 정렬된 value 알림 순서를 유지한다.
- 상태 스냅샷은 상속된 ViewModel 정보와 values·stateKeys·revision·empty를 포함한다.

`StateSource`는 value-or-fallback과 revision 관측의 교체 가능한 C++ 인터페이스이다.
ProgressModel은 구체 StateModel에 의존하지 않고 이 인터페이스를 사용한다.

## ProgressModel

직접 minimum·maximum·start·current 값과 상태 키 네 개를 유지한다. 상태 제공자가
있으면 상태 값을 사용하고 숫자로 읽을 수 없으면 직접 값으로 돌아간다. 이 경계에서만
원본의 숫자·bool·숫자 문자열 변환을 적용하며 Core::Value의 엄격한 변환 규칙은 바꾸지 않는다.

정방향·역방향 범위를 모두 처리한다. 절댓값 `0.000001` 미만 범위는 이진 진행이고,
일반 정규화는 0..1로 제한한다. fill 시작은 두 정규화 값의 최솟값, 길이는 절대 차이다.
원통형 반경과 직사각형 반경 계산도 원본 수식에 대응한다.

상태 제공자는 약한 참조이며 교체하면 이전 구독을 해제한다. 제공자 종료 시 직접 값으로
돌아가도록 최종 관측 알림을 처리한다. 원본의 QPointer 무효화와 비교해 종료 알림은 명시적인
네이티브 수명 계약이다. 제공자 내부의 임의 비동기 실행이나 임의 Qt 객체 호출은 수행하지 않는다.

## 검증 경계

`tests/native/presentation_models.cpp`에 기본값·정규화·patch·알림 순서·null·범위·
상태 변환·교체·종료·스레드·재진입·수신자 실패·Core 관측 검사를 준비한다.
현재 저장소 지침에 따라 LVRS 테스트 실행은 전체 목표의 최종 일괄 단계에 모은다.
중간 C++ 구문 검사·컴파일·링크와 설치 헤더 검사를 실행 통과로 표시하지 않는다.
ProgressBar는 [네이티브 컨트롤](ProgressBar.md)에서 이 모델을 사용한다.
`set_configuration`의 값·키·제공자 일괄 반영 검사도 추가한다. 플랫폼 실행과 전체 상태
레지스트리는 여전히 완료 감사 대상이다.

2026-10-26에는 일반 구성과 ASan·UBSan 구성에서 네이티브 라이브러리, 모델 실행 검사
23개를 담은 실행 파일, 재배치 설치 소비자의 컴파일·링크를 완료했다. 직접 scoped import와
중첩 C++ 모델을 사용하는 LVUI 뷰 두 개도 각 구성에서 생성·컴파일했다. 설치 소비자는
최종 실행 단계에서 별도 검사 1개로 등록되어 있으며, 이 시점에는 실행하지 않았다.

기록은 소스 저장소의 `build/lvui-completion/presentation/verification-20261026.json`이다.
라이브러리 객체·공개 헤더의 설치 대조, 테스트 등록 목록, 원본과 제품 비변경 경계를 포함한다.
`partial` 84개, `pending` 65개이며 `complete`는 0개이다. 실행을 수행하지 않은 컴파일
결과나 C++ API 대응만으로 전체 원본 동등성 또는 전체 목표 완료를 주장하지 않는다.

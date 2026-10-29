# 네이티브 스냅샷 이력

원본 `src/backend/model/modelundostack.*`를 기준으로 한 C++ API이다.
`ModelUndoStack.h/.cpp`는 Qt, QML, 파일 저장, 생성 엔진과 독립적이다.
이력 저장과 이동은 네이티브 모델이 수행하며 뷰는 깊이와 실행 가능 여부를 관측한다.

## 값과 명령

스냅샷은 소유된 `ii::core::Value`이다. `std::nullopt`는 없는 스냅샷이며,
`Value{}`는 명시적인 null 데이터로 저장할 수 있다. 원본 QVariant의 전체 메타타입이나
숫자 간 묵시적 비교를 재현하지 않고 Core의 엄격한 값·비교 계약을 사용한다.

- 기본 한도는 128이다. 1 미만은 1로 제한하며, 32비트 정수 범위를 넘는 한도는 거부한다.
- `push_snapshot`은 없는 값과 연속 중복을 무시한다. 중복 push는 redo를 지우지 않는다.
- 새로운 스냅샷은 redo를 지운다. 한도를 넘으면 가장 오래된 undo부터 제거한다.
- `take_undo_snapshot(current)`는 마지막 undo를 반환하고 유효한 current를 redo로 이동한다.
- `take_redo_snapshot(current)`는 마지막 redo를 반환하고 유효한 current를 undo로 이동한다.
- 빈 이력은 빈 이동 결과이다. `clear`는 두 스택을 비우며 빈 상태에서는 알리지 않는다.

선두 제거에 나머지 스냅샷을 이동하지 않는 deque를 사용한다. 깊이와 실행 가능 여부의
조회는 전체 스냅샷을 복사하거나 직렬화하지 않는다. 수치 성능 개선률을 주장하지 않는다.

## 반영과 오류

반환한 `SnapshotTransfer`는 이동된 `snapshot`과 `observation_error`를 분리한다.
외부 Result 오류는 이동 반영 이전의 실패이다. 이동 이후 수신자가 실패해도 옮긴
스냅샷을 버리지 않으며, 호출자는 데이터를 적용하고 알림 오류도 처리할 수 있다.
동기 변경 수신자 한 개의 실패는 나머지 수신자에게 전달하는 것을 중단하지 않는다.

일반 push·clear·limit 변경의 알림 오류는 이미 반영된 변경에 대한 Result 오류이다.
이 경우 모델을 읽어 반영된 상태를 확인하며 파일 트랜잭션의 롤백과 혼동하지 않는다.
소유 스레드 밖의 쓰기는 wrong_executor, 읽기는 예외로 거부한다. 알림 중 재진입 쓰기는 busy이다.

## 관측

`limit`, `undo_depth`, `redo_depth`, `can_undo`, `can_redo`는 Core의 범위 관측에 연결한다.
limit와 stack 의존성은 분리한다. 한도를 줄여 실제 이력을 자르면 limit 다음에 stack도
알린다. 이는 원본의 limitChanged만 발행하여 깊이 표시가 남을 수 있는 경우를 바로잡은
명시적인 네이티브 동작이다. 이력을 자르지 않은 한도 변경은 깊이 독자를 무효화하지 않는다.
수신자는 약하게 등록하며 수신 중 소유자가 제거되어도 진행 중 명령의 내부 상태만 보존한다.

## LVUI와 검증 경계

`NativeHistory.lvui`는 C++ 백엔드의 기억·undo·redo·clear 메서드를 버튼에 연결한다.
`NativeHistoryScoped.lvui`는 모델을 직접 가져와 깊이와 실행 가능 여부를 표시한다.
두 뷰 모두 UI 검사가 파일 revision을 바꾸거나 생성 작업을 실행하지 않는다.

`tests/native/model_undo_stack.cpp`에는 값·분기·한도·관측·오류·수명 검사 12개를 준비한다.
`tests/native/history_aot.cpp`에는 실제 생성 C++와 창 없는 위젯을 사용하는 버튼·외부 갱신·
조회 비변경·퇴역·직접 import 검사 5개를 준비한다. 통합 소스 위치는
`LVRS_LVUI_INTEGRATION_SOURCE`로 선택하며, 해당 fixture가 없는 독립 저장소에서는
추가 AOT 통합 대상을 만들지 않는다. 설치 소비자는 공개 패키지를 재배치한 후 일곱 개의
실제 LVUI 팩터리를 컴파일한다.

저장소 작업 지침에 따라 LVRS 실행 검증은 전체 목표의 마지막 일괄 단계에 남긴다.
빌드와 테스트 등록을 실행 통과로 표시하지 않는다. QVariant 변환·전체 원본 신호 순서와
플랫폼 실행 검증은 완료 감사 대상이다.

2026-10-27에는 일반·ASan/UBSan 구성에서 이력 모델 검사 실행 파일과 실제 생성 뷰의
조작 검사 실행 파일을 컴파일·링크했다. LVUI 전체 팩터리 검사 대상 64개의 생성 코드는
두 구성에서 동일하다. 일곱 팩터리를 포함하는 재배치 설치 소비자도 두 구성에서 링크했다.
연관 LVRS 검사 55개는 등록 상태이며 아직 실행하지 않았다. 기록은 소스 저장소의
`build/lvui-completion/snapshot-history/verification-20261027.json`이다. 동등성 감사표는
이 시점에 partial 85개·pending 64개·complete 0개이다.

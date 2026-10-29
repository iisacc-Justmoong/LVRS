# 네이티브 목록 자료 경계

`lvrs::ModelSource`는 기존 `backend/model/modelsource.*`의 조회·변경 통지 계약을 C++23, iiSDKCore `Value`, `Observable`, `QueryPort`로 표현한다. `lvrs::ModelAdapter`는 기존 singleton의 세 조회를 상태 없는 C++ 객체로 제공한다. Qt/QML 원본과 Dreamscapes 제품 소스는 변경하지 않는다.

## 자료와 수명

- `ModelInput(Value)`는 값 목록을 소유한다. 배열만 목록이며 숫자·문자열·객체를 임의의 행 목록으로 해석하지 않는다. `ModelEntry`의 누락과 명시적인 null을 구분한다.
- `ItemModel`은 루트 행·열 수, 역할 ID/이름, 셀 값, 유효 인덱스를 제공하는 읽기 인터페이스이다. 원본 데이터와 수정 책임은 구현체에 있다. `data()`의 `nullopt`는 누락이고 `Value{}`는 존재하는 null이다.
- `ModelSequence`는 기존 동적 `get`/`at` 자료의 명시적인 C++ 대체 인터페이스이다. 기존 일반 객체 순차 자료처럼 변경 후 `invalidate()`를 호출한다.
- `ModelEntry(shared_ptr<const ModelProperties>)`는 기존 네이티브 속성 제공자를 빌린다. 별도의 속성 사본을 생성하지 않는다.
- 모델·순차 자료·객체는 약한 참조이다. 호출자가 실제 소유자를 유지해야 한다. 소유 모델의 삭제는 남은 소스를 비우고 revision/count를 알린다. 이전 모델 교체·뷰 폐기 시 구독을 닫는다. 제공자 생성·조회·통지·폐기는 동일한 소유 스레드에서 수행한다.

## 원본 계약 대응

| 원본 API | 네이티브 의미 |
|---|---|
| source / column / count / revision / itemModel | C++ getter와 Core 읽기 의존성 |
| setSource | 자료 동등성 검사, 후보 구독 준비 후 교체, 변경된 count만 추가 통지 |
| setColumn | 음수는 0, 같은 값은 무변경, column/revision 통지 |
| at / row | 목록 값 또는 선택 열의 역할 맵, 잘못된 행은 누락/빈 맵 |
| roleValue | `optional<Value>`, 공백을 제거한 역할명, 누락 시 optional fallback, 존재하는 null 유지 |
| textValue | 순서 있는 역할 후보 중 첫 비-null 값, scalar는 직접 문자열화 |
| boolValue / intValue | 기존의 정수 우선 변환과 fallback |
| invalidate | revision 증가, count가 같아도 revision/count 통지 |
| ModelAdapter.isItemModel / count / row | 동일 ItemModel 계약을 통한 읽기 전용 조회 |

`row()`는 이름 없는 역할과 누락된 값을 제외하고 display(역할 0), edit(역할 2)를 보충한다. `index`, `row`, `column`은 같은 이름의 사용자 역할을 덮어쓴다. 요청한 열이 범위를 벗어나면 0열을 사용하되 유효하지 않은 모델 인덱스는 빈 맵을 반환한다. 동일 이름의 역할이 여러 ID에 대응되면 증가하는 역할 ID의 마지막 값이 남는다. 기존 QHash의 중복 역할 순서는 정해져 있지 않으므로 유효 모델은 고유 역할명을 사용해야 한다.

`boolValue`는 일반적인 문자열 truthiness가 아니다. 기존 구현이 `canConvert<int>()`를 먼저 검사하므로 문자열 `"true"`는 false이고 `"12"`는 true이다. 유한 실수는 0.5 동률을 0에서 먼 쪽으로 반올림한다. 정수와 유효한 signed 64-bit 십진 문자열은 하위 32비트로 축소한다. signed 64-bit 범위를 벗어나는 실수는 정의되지 않은 변환 대신 fallback을 사용한다. 이는 Qt 6.8.3의 [수치 변환](https://github.com/qt/qtbase/blob/v6.8.3/src/corelib/kernel/qmetatype.cpp)과 [반올림](https://github.com/qt/qtbase/blob/v6.8.3/src/corelib/global/qnumeric.h)을 확인한 범위이다.

## 변경 통지와 데이터 분리

소스 교체는 source → revision → 필요한 count, 열 변경은 column → revision, 명시적 무효화와 모델 변경은 revision → count 순서이다. 삽입·삭제·이동·reset·layout·data·제공자 폐기를 처리한다. 알림 중 조회는 허용하고 동일 소스에 대한 재진입 명령은 busy로 거부한다. 다른 제공자에서 알림 중 들어온 변경은 최대 256회 정착 루프로 반영한다. 수신 오류는 이미 반영된 상태를 롤백하지 않으며 실패 결과로 전달한다. 재시도 전에 현재 revision을 확인해야 한다.

뷰의 선택 열, 조회, 자료 구독은 원본 문서 수정 API를 호출하지 않는다. 표시 갱신이 생성·저장·파일 revision 변경으로 연결되지 않는다. 이 경계에는 widget, OS 컨트롤, 작업 스레드 생성, 폴링 또는 JS 엔진이 없다.

## 검증과 남은 범위

`tools/native_model_source_contract.py`는 원본 네 파일의 SHA-256과 원본 선언 목록을 고정하고 네이티브 대상 클래스 내부의 API를 대조한다. 같은 헤더에 있는 다른 클래스의 동명 메서드로 누락된 API가 가려지는 경우도 회귀 검사에 포함한다. 선언 감사는 실제 C++ 컴파일·링크 및 실행 검사를 대체하지 않는다.

`tests/native/model_source.cpp`의 15개 계약과 `model_source_aot.cpp`의 7개 통합 계약을 등록했다. 배열·역할·열·변환·이벤트·약한 수명·수신 오류·재진입·원본 비변경·뷰 identity·Static 값을 검사하도록 준비한다. 저장소 지침에 따라 LVRS 앱과 테스트의 실제 실행은 전체 작업 마지막 일괄 단계로 유보한다. 컴파일/링크와 실행 통과는 구분한다.

임의의 QObject/JS 객체, 사용자 등록 QVariant 변환, JS getter의 부작용, QVariant subtype 전체를 재현하는 런타임을 새로 만들지 않는다. 이 자료는 native provider로 명시적으로 표현해야 한다. 바이트열의 잘못된 UTF-8과 Qt의 대체 문자 정책, 숫자 문자열 표시의 극단적인 지수 표기, 동적 타입 전체의 oracle 비교, 각 플랫폼 실행은 아직 동등성 검증 대상이다. 이 단계는 원본 선언 전체와 상속·플랫폼 계약의 완료가 아니다.

## 공급자 소멸의 최종 알림

ItemModel은 소멸에 진입하면 먼저 읽기·관측·명령 접수를 닫는다. 일반 변경 알림은
닫힌 상태에서 중단하지만, 소유 실행 영역에서의 소멸 알림은 분리한 수신자 목록에 마지막으로 전달한다.
ModelSource는 만료된 공급자를 되살리지 않고 revision과 count를 무효화한다.
소멸 중 새 구독 거부·한 번의 최종 알림·기존 원천 구독 해제와 읽기 안전성을 회귀 검사한다.

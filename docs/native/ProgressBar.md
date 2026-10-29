# 네이티브 ProgressBar

원본은 `src/qml/components/control/display/ProgressBar.qml`이다. C++ API는
`ProgressBar.h/.cpp`에 있으며 LVUI의 `FRAMEWORK LVRS`가 이 컨트롤을 생성한다.
기존 `Widgets::progress`의 iiGUI 기본 막대는 호환 API로 유지한다.

## 상태와 수명

`ProgressBar::create`는 하나의 자체 C++ 위젯과 `ProgressModel`을 생성한다.
`set_options`와 소스 모델 변경은 같은 위젯·모델을 갱신하며 파일이나 생성 작업을 변경하지 않는다.
`root`, `options`, `state`, `model`, `drawing` 및 `state_number`, `normalized_value`,
`resolved_radius`를 제공한다. `drawing(size)`는 주어진 배치 크기의 불변 네이티브 그리기
명령을 반환하며, 동일 상태와 크기에는 캐시를 재사용한다. 수치 계약은
[PresentationModels.md](PresentationModels.md)를 따른다.

상태 제공자는 교체 가능한 `StateSource` 인터페이스이며 약한 참조이다.
`progress_source(owner, source)`는 shared/weak 제공자의 기존 수명을 사용하고,
스코프에 직접 저장된 제공자는 복사하지 않고 소유 스코프를 차용한다. 전달하는 owner는
해당 객체의 실제 수명을 포함해야 한다. 퇴역한 핸들은 새 세대의 컨트롤에 접근하지 못한다.

`ProgressModel::set_configuration`은 직접 값 네 개·상태 키 네 개·제공자를 함께 검증하고
계산한 뒤 한 번에 반영한다. 모든 알림 수신자는 반영된 전체 구성을 읽는다.
같은 구성은 알리지 않으며, 반영 이후 수신자의 실패는 이미 반영된 구성을 롤백하지 않는다.

## 배치와 그리기

기본 크기는 100×6이며 regular 높이는 3이다. 명시적인 width·height는 실제 배치 크기를
정하고, 암시적 크기는 100×bar_height로 유지한다. `large_height`, `regular_height`,
`corner_radius`도 입력이다. 단위는 현재 네이티브 레이아웃 단위이며 전역 스케일 적용의
전체 원본 동등성을 완료로 표시하지 않는다.

track·fill·1px outline을 iiGUI 패널 명령으로 그린다. 기본 track은 `#0D000000`,
fill은 원본의 `Theme.accentBlue`, outline은 `#14000000`이다. 사용자 fill은 optional이며
생략하면 기본값을 사용한다. sRGB 입력은 iiGUI의 선형 색상으로 변환한다.
원통형 반경은 실제 사각형 변의 최솟값/2, 기본 반경 입력은 100이다.

실제 할당 크기에서 시작 위치와 길이를 원본 수식으로 제한한다. 수치 모델의 fill 값과
화면용 displayed 값은 분리하며, `displayed_start`·`displayed_progress`를 해제하면
자동 모델 바인딩으로 돌아간다. 화면용 값을 수치 모델로 다시 쓰지 않는다.

## 모션

원본 `SpringBehavior`의 구현인 360ms OutBack, overshoot 1.45를 Core 타임라인으로 계산한다.
컨트롤마다 스레드나 타이머를 만들지 않고 iiGUI 호스트의 공유 Core 프레임 소스를 사용한다.
중간에 목표가 바뀌면 현재 표시값에서 다시 시작한다. 앱의 game pause·time scale과 분리된
프레임 시간을 사용한다. 호스트의 Motion 또는 명시적인 공유 Motion을 사용하며,
motion 비활성·reduced motion 변경은 목표값으로 정착한다.

프레임 소스 접수 실패는 오류로 반환하고 이미 반영된 모델 값은 유지하되 표시를 목표값으로
정착시킨다. 실패한 시점의 중간 값에 멈추지 않는다. 잘못된 스타일·크기 입력은 반영 전에 거부한다.

## 검증 경계

`tests/native/progress_bar.cpp`에는 기본값·역범위·제공자 수명·크기·인스턴스 보존·잘못된
입력·표시값 분리·퇴역·그리기·OutBack·목표 변경·프레임 접수 실패·스코프 차용 검사를 준비한다.
이 검사는 OS 창을 만들지 않는다. 저장소 작업 지침에 따라 LVRS 테스트 실행은 전체 작업의
마지막 일괄 단계에 남긴다. 생성 C++의 컴파일·링크를 실행 통과로 보고하지 않는다.

Qt Item의 전체 상속 API, 객체 ID를 통한 선언적 읽기 전용 속성 참조, 플랫폼별 실제
그리기·입력 및 스케일 동등성은 남은 감사 항목이다. 이 컨트롤을 추가한 사실만으로 LVRS나
LVUI 전체 완료를 주장하지 않는다.

2026-10-26 검증 기록은 소스 저장소의
`build/lvui-completion/progress-bar/verification-20261026.json`이다. 일반·ASan/UBSan
구성의 라이브러리·검사 실행 파일·재배치 설치 소비자를 컴파일하고 링크했다. LVUI 팩터리
62개의 생성 코드가 두 구성에서 동일하며, 컴파일러 검사 34개씩 통과했다. LVRS 실행 검사
38개는 각 구성에 등록되어 있으나 실행하지 않았다. 이 기록 시점의 동등성 현황은
partial 84개, pending 65개, complete 0개이다. 이후 현황은 Parity 문서를 따른다.

## 최종 작업 공간 실행에서 보완한 그리기 경계

원본의 cornerRadius 값은 유지하면서 실제 패널 반경은 배정된 너비·높이의 절반으로
제한한다. 네이티브 렌더러의 유효 장면 기하 조건에 맞추는 조정이다. 기본 100
반경과 높이 6 조합을 그대로 GPU에 넘기면 유효하지 않은 패널이 된다. 1px 테두리도
아주 작은 배정에서는 같은 절반 한계로 줄인다. 기본·소수 픽셀 크기의 유효 장면을
paint 회귀 검사에 추가하며 모델의 원래 수치 조회 계약은 변경하지 않는다.

진행 비율의 계산 결과는 부동소수점 오차를 고려한 엄격한 허용치로 비교한다. 크기 검사는
호스트 전체를 채우는 세대 루트 대신 일반 스택의 자식으로 막대를 구성하여 명시·자연 높이
계약을 확인한다. 호스트의 채우기 배치를 컨트롤의 높이 오류로 오인하지 않는다.
## 곡선 검사의 시간 기준

OutBack 중간값 검사는 Core의 수동 프레임 시계를 알려진 시각에 고정한 뒤 다음 전환을
시작한다. `set_options()` 이후에 읽은 실제 시계를 시작 시각으로 가정하면 스케줄링 지연을
곡선 오차로 오인한다. 준비 전환을 완료하고 즉시 상태를 0으로 되돌린 뒤 동일한 프레임
시각에서 360ms 전환을 시작하며, 180ms의 값을 원본 곡선과 1e-6 이내로 대조한다.
실제 시간 대기나 애니메이션 구현의 완화는 추가하지 않는다. 이 검사 보정 후 두 구성의
ProgressBar 관련 13개 항목을 재시도 없이 별도로 빌드·실행해 확인한다.


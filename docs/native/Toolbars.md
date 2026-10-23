# 목록·계층 도구 막대와 푸터

ListToolbar·ListFooter·HierarchyToolbar·ToolbarButton을 iiGUI 자체 C++ 버튼·컨테이너로
구성한다. 기존 QML은 동작 비교 기준이며 새 API·AOT·라이브러리는 Qt를 사용하지 않는다.

## 고정 슬롯

ListToolbar는 빈 아이콘도 포함한 세 슬롯을 유지한다. 기본 아이콘 18, 버튼 22,
가로 여백 8·세로 4·간격 4로 자연 크기는 90×30이다. 빈 슬롯과 interactive=false는
클릭을 막으며 set_options는 버튼·콜백 정체성을 보존한다. icon_clicked는 0부터 시작하는
슬롯 번호와 내장 아이콘의 정규 이름을 받는다. 기존 qrc 아이콘 경로도 Icons::normalize로
해결한다. 메모리 RasterImage를 직접 공급할 수 있으며 파일·네트워크 I/O는 수행하지 않는다.

ListFooter는 세 ListFooterSlot을 받는다. 기본 버튼은 addFile·generaldelete·settings이고
세 번째는 메뉴 표시자이다. 아이콘 18·좌우 2·높이 22, 메뉴의 좌우 4/2·간격 -2를
적용해 자연 크기 88×26을 얻는다. 메뉴 표시자는 팝업을 자동 생성하지 않으며 클릭
콜백이 앱의 메뉴를 연다. 슬롯의 on_clicked 다음 button_clicked를 호출하되 앞 콜백이
소유 트리를 종료하면 후속 콜백은 실행하지 않는다. 숨긴 슬롯은 행 배치에서 제외한다. 기본 눌림 색상은 원본 accentMuted 토큰이며,
기본 블루의 명시 값과 다른 primary의 25% tint 공식을 적용한다.

## 계층 탐색과 활성 항목

HierarchyToolbar는 기본 최소 폭 200·아이콘 슬롯 22·여백 8/2를 사용한다. 모델은
최대 256개 HierarchyToolbarEntry이고, 비어 있으면 같은 세대의 manual_root를 사용한다.
ToolbarButton을 root 아래 구성하면 기본 슬롯으로 들어가며 register_button으로 연결한다.
LVUI의 직접 ToolbarButton 자식은 생성기가 자동 등록한다. 수동 버튼은 모델 모드에서
숨겨지고 모델을 비우면 같은 핸들·콜백으로 되돌아온다.

ToolbarId는 정수 또는 문자열이다. 모델의 명시 ID가 없으면 행 번호를 사용한다.
정규화는 기존 ID/버튼을 우선하고, 사용할 수 없으면 selected인 활성 가능 항목·첫 활성
가능 항목·첫 항목 순서로 고른다. 모두 비활성인 경우 첫 항목을 표시하지만 실행하지
않는다. 원본처럼 숨김 자체는 정규화나 프로그램 trigger_index의 제외 조건이 아니다.
set_active_id·normalize는 클릭이나 active_changed를 방출하지 않는다.

사용자 활성화는 상태 변경 시 active_changed, button_triggered, 각 명명 이벤트,
엔트리 on_triggered 순서이다. ToolbarTrigger는 버튼·ID·행 번호·Core Value item과
첫 이벤트·페이로드, emit·activate 명령을 제공한다. 모델 교체·소유자 종료 후 보관된
명령은 closed가 된다. 콜백 중 모델이 바뀌면 이전 revision의 후속 통지는 중단한다.
중첩 activate는 16단계로 제한하고 콜백 예외는 선택 반영 후 failed로 반환한다.

같은 ID/순서의 set_items는 기존 버튼을 갱신한다. 구조 변경은 content_host의 독립
세대를 구성·검증한 뒤 교체한다. 아이콘·페이로드 검증 실패는 기존 모델을 유지한다.
이벤트는 엔트리당 64개, Value는 깊이 32·노드 8192·1 MiB, 모델 전체는 4 MiB이다.
분산 간격은 현재 실제 폭과 보이는 버튼의 폭을 사용하며 모델의 자연 폭은 원본처럼
숨김 항목도 포함한 전체 슬롯 수를 기준으로 한다.

## LVUI와 검증 범위

네 예약어는 실제 create 호출과 Core Observable 구독으로 연결한다. 버튼의 iconName·
buttonId·활성/치수, ListToolbar의 세 아이콘·레이아웃, ListFooter의 typed C++ 슬롯,
HierarchyToolbar의 buttonItems·activeButtonId·레이아웃을 갱신한다. 임의 JS 딕셔너리를
해석하지 않으며 복합 설정은 scoped C++ 멤버를 사용한다. 콜백을 담은 구성은 값 동등성으로 비교할 수
없으므로 반응형 모델에는 Observable<HierarchyToolbarItems>, 반응형 푸터에는
Observable<shared_ptr<const ListFooterSlot>>를 사용한다. 불변 구성의 공유 포인터를 교체해
통지하며 plain vector/slot도 받는다. toolbar_items/ footer_slot 어댑터는 소스를 해석하지 않는다.

모든 앱·테스트·self-test 실행은 전체 작업 마지막의 일괄 단계로 보류한다. 현재는
도구 막대 33개와 공통 버튼 5개, AOT 생성 사례의 컴파일·링크·패키지 정적 검사를
진행한다. 실행 통과·메모리 안전성·시각 동등성을 주장하지 않는다.

일반 IconButton의 자동 수동 등록, 임의 QObject/배열 모델 어댑터·QML props 사전과
글리프/외부 URL 로딩, 모든 상속 속성·signal 순서·모션·전역 Theme 교체·각 플랫폼
입력/접근성 동등성은 남아 있다. 개별 컴포넌트의 options 외에 root를 직접 바꾸면
해당 변경이 options에 역반영되는 계약도 제공하지 않는다. 네 타입은 부분 구현이다.

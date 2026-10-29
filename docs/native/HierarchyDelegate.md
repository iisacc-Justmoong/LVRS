# 계층 행 delegate의 네이티브 컨텍스트

`HierarchyDelegateContext`는 마운트된 한 행의 표시 입력과 생성한 HierarchyItem 핸들을
연결한다. 복사한 컨텍스트는 같은 상태를 공유한다. 소스 옵션은 Core 읽기 의존성을 제공하고
실제 위젯 핸들은 약하게 보유한다. 모델·문서·위젯 세대를 새로 소유하지 않는다.

HierarchyList의 마지막 `HierarchyDelegateFactory` 인자는 WidgetBuilder·부모·지역 ID·
컨텍스트를 받으며 `Result<HierarchyItem>`을 반환한다. 기존 HierarchyItemFactory와
동시에 지정하지 않는다. 생성은 가시 범위와 overscan에서만 수행한다.

`create_item`은 소스에서 주어진 선택/펼침/깊이/키/ID/자식 여부/드래그 상태·컨트롤러와
논리 폭/행 높이를 유지한다. 사용자 레이블·아이콘·패딩·색·count·countView를 추가할 수 있다.
소스가 비활성화한 enabled/activatable/draggable을 사용자 설정으로 다시 켜지 않는다.
위젯 세대가 컨텍스트를 보유하며 퇴역 시 닫는다. 제품은 정확히 한 번만 생성한다.

소스 입력을 읽는 렌더러는 `apply`로 계산한 옵션을 적용한다. 성공한 경우에만
applied_revision을 현재 source revision으로 옮긴다. `update_source`는 새 입력을 저장한
뒤 Core 관측으로 렌더러를 깨운다. 렌더러가 적용하지 못하면 실패를 반환하고 이전 실제
위젯을 유지한다. 소스 revision은 이미 갱신되었을 수 있으며 롤백했다고 해석하지 않는다.
같은 입력의 재시도도 새 알림을 발행할 수 있다. 사용자 상태의 다음 정상 publication으로
기존 Bindings가 복구하면 해당 revision을 적용 완료로 기록한다.

구성·소스 알림 중 재진입 소스 변경은 busy이며 옵션 조회와 렌더러의 apply는 가능하다.
모든 접근은 소유 스레드에서 수행한다. 닫힌 컨텍스트는 상태 진단 외의 새 조작을 거부한다.
close 자체가 다른 소유자의 위젯이나 모델을 삭제하지 않는다.

클릭/펼침은 목록의 원래 명령을 먼저 전달하고 이어서 사용자 콜백을 전달한다. 첫 콜백의
예외가 다음 콜백을 누락시키지 않으며 첫 오류를 마지막에 반환한다. 원래 명령이 위젯이나
컨텍스트를 퇴역시키면 사용자 콜백은 호출하지 않는다. 포인터·드래그 소비도 원래 목록으로
연결한다. 선택·표시·스크롤이 모델 revision이나 파일 저장을 발동하지 않는다.

## 기존 C++ 팩터리

기존 옵션/이벤트 인자를 받는 팩터리도 유지한다. 생성 결과와 이전 소스의 차이로 사용자
설정을 구별하고, 나머지 필드는 새 소스 값을 상속한다. 키·기하·구조 상태는 목록이 유지한다.
기존 서명에는 명시적인 override 목록이 없으므로 소스와 같게 지정한 값의 의도를 구분하지
못한다. 확정적인 반응형 사용자 설정은 새 컨텍스트와 렌더러를 사용한다. 기본 행은 추가
컨텍스트를 만들지 않는다.

## LVUI 연결과 남은 경계

LVUI itemDelegate는 HierarchyItem을 루트로 하는 View 또는 타입 지정 구성을 받는다.
첫 입력 타입은 `lvrs.HierarchyDelegateContext`이며 나머지 입력은 기본값을 가져야 한다.
생성 C++는 소스 옵션을 기본값으로 사용하고 명시한 표시 값을 덮어쓴 뒤 create_item/apply를
호출한다. 루트 import와 countView의 import·Bindings는 각 위젯 세대에 속한다.

키·선택·펼침·폭·높이 등 목록 소유 입력을 delegate에서 명시하면 소스 위치로 진단한다.
높이는 generatedRowHeight나 컨트롤러의 행 높이 override로 정의한다. delegate의 임의
높이를 자동 측정하는 경로, 표시 override가 모든 탐색/가시성 정책에 미치는 효과, 전체
nodeData/모델 role 변환·source 쓰기·상속 스타일·모바일/접근성/플랫폼 동등성은 계속 남는다.

`tests/native/hierarchy_delegate.cpp`, hierarchy_views와 hierarchy_aot에 관련 검사를
정의한다. 현재 지침에 따라 컴파일·설치 소비자 검증과 최종 실제 실행 결과를 구분한다.


2026-10-27 hierarchy-delegates 체크포인트에서는 HierarchyItem 루트 LVUI itemDelegate,
행별 HierarchyDelegateContext, 소스/적용 revision 확인, 원래 이벤트와 사용자 이벤트 연결,
기존 C++ 팩터리 스타일 보존과 중첩 countView를 추가했다. LVUI 전체 검사는 일반과
ASan/UBSan 각각 299개를 통과했다. 생성 팩터리 71개와 재배치 소비자 14개를 두 구성에서
컴파일·링크한다. LVRS는 컨텍스트 16개·컨트롤러 27개·위젯 35개·생성 뷰 23개·모델/투영
69개로 총 170개를 빌드·등록하며, 실제 실행은 전체 목표의 마지막에 수행한다.
실제 창/GPU·접근성·다른 플랫폼·성능 개선률과 GUI 전체 메모리 계측의 증거로 해석하지 않는다.
메모리 구성의 iiGUI 의존성은 일반 설치본이다. 최종 대조는
`build/lvui-completion/hierarchy-delegates/verification-20261027.json`에 기록한다.

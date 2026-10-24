# Card·Avatar의 C++23 네이티브 구현

기존 QML Card와 Avatar의 선언·기하·색상·전용 SVG를 기준으로 한다. 공개 헤더와 구현은
src/LVRS/Card.h·Card.cpp, Avatar.h·Avatar.cpp에 함께 있다. AppCard와 일반 Card는 별개이다.
LVUI는 빌드 시 이 객체를 생성하는 C++를 만들며 Core가 앱 진입점·작업·프레임·수명을
소유한다. iiGUI 자체 ActionSurface·Image·Panel·Text가 그리며 OS 컨트롤은 생성하지 않는다.

Card는 File·FilePreview·Folder·Project·Device·Model·Member·Link를 제공한다. File은
140×160·210×240·360×280, FilePreview는 480×320, 정보 카드는 폭 256/최소 높이 280을
기본값으로 한다. 정보 영역은 행·진행률에 따라 늘어난다. 소스의 기본 글꼴·행 높이,
테두리 1/선택 2, 일반 inset 18+border와 Small File 12+border, 사진 자르기·그라데이션·
선택 표시를 적용한다. 자체 사진은 둥근 마스크를 갖는다. preview-only는 선택 표시와
탭·포인터 포커스가 없다. display_state는 표시만 덮어쓰며 원본 selected는 바꾸지 않는다.

파일명·설명·상세·메타데이터·상태·요약·각주·도메인·프리뷰 제목, label/value/icon 행,
진행률과 사용자 색상, 메뉴·액션 표시를 받는다. 진행률은 모델값을 보존하고 표시만
0..1로 제한한다. 루트 클릭은 checked를 먼저 반영하고 selected_changed, clicked 순서로
전달한다. set_selected는 선택 콜백만 전달한다. 내부 메뉴와 액션 버튼은 부모를 토글하지
않는다. 비활성 activate는 성공하는 무동작이다. 접근성 이름과 설명에는 원본 필드를 넣는다.

preview_source·icon·Avatar.source는 shared_ptr<const RasterImage>이다. 크롭은 불변 원본
복제 없이 샘플 좌표로 처리한다. Avatar는 24·40·64 크기와 원형 center-cover를 제공한다.
source가 없으면 empty/0, 준비된 source가 있으면 ready/1이다. URL·파일 읽기/비동기
디코더/네트워크 캐시/취소는 리소스 공급 계층이 담당해야 한다. Qt URL 로더와 동일한
loading/error 상태를 구현했다고 간주하지 않는다. LVUI의 asynchronous·cache·문자열 URL은
진단으로 거절하며 무시하지 않는다. example/NativeMediaGallery는 Core 작업이 만든
불변 이미지를 owner 이벤트 루프로 전송해 기존 위젯에 반영하는 실제 AOT 예제이다.

CardContent의 preview·icon은 WidgetFactory를 받으며 내용은 독립된 세대로 교체된다.
실패한 팩토리는 기존 내용을 유지한다. LVUI 중괄호 자식은 card/inline 기본 프리뷰
슬롯으로 가고, previewComponent와 동시에 지정하면 생성 오류이다. 반응형 팩토리는
불변 CardComponent(shared_ptr<const WidgetFactory>)를 Core Observable에 넣어 교체한다.
본문이나 데이터가 바뀌어도 기존 자식·백엔드를 재생성하지 않는다. Card/Avatar의 래퍼는
옵션 변경용 API이며 root().set_*를 직접 사용한 값 전체를 DTO로 역추적하지는 않는다.

공통 iiGUI는 이미지 영역·둥근 마스크·안쪽 border·3점 gradient와 이미지 뒤 텍스트 합성을
제공한다. 네이티브 paint provider는 읽기 전용 상태만 받아 cached WidgetDrawing을 내보낸다.
UI 조회/변경은 그리기 중 busy로 거절된다. 폰트 폭과 아이콘은 옵션 준비 시 캐시한다.
전경 텍스처는 실제 콘텐츠가 차지하는 디바이스 영역으로 제한한다. 현재 이미지/텍스트
래스터는 공통 CPU 경로이고 Metal·D3D·Vulkan은 최종 합성을 담당한다. GPU 텍스처 메모리나
프레임 시간의 실제 개선 수치는 최종 실행 검증 전에는 주장하지 않는다.

29개 Card/Avatar 사례, 17개 iiGUI 합성 사례, LVUI 생성 검사와 갤러리 self-test를 등록한다.
컴파일·링크·AOT·비공개 패키지 재배치와 실제 실행은 별도 증거이다. 앱·테스트·self-test는
사용자 지시에 따라 전체 목표의 최종 일괄 단계로 보류한다. ASan/UBSan 빌드도 실행 증거는
아니다. 정상/계측 빌드·설치 소비자 구성 결과는 Verification 문서와 build/ 증거로 기록한다.

남은 원본 계약은 URL 로딩 상태/캐시, 사용자 프리뷰 서브트리의 둥근 clip, 다중 행 마지막
줄 말줄임, 전체 상속 API·시그널 순서·모션·라이브 글로벌 Theme 변경, 플랫폼 AX와 입력 및
Windows/Linux/mobile/WASM 실행이다. 자체 이미지 마스크와 사용자 자식 clip은 구분한다.
64개 overlay 예산을 유지하며 대규모 카드 그리드 가상화는 아직 별도 구현이 필요하다.
따라서 parity는 partial이며 완료로 승격하지 않는다.

상세·메타데이터를 접근성 설명으로 합칠 때 구분자의 UTF-8 바이트도 사전 검증한다.
합성된 설명이 한도를 넘으면 네이티브 옵션을 변경하기 전에 거절한다.

# Windows 네이티브 빌드 검증

Theme의 `nodestest` 별칭은 실제 SVG 이름인 `nodesTest`를 가리킨다. QML 자원과
네이티브 아이콘 생성기는 같은 정규 이름을 사용하며 파일 시스템의 대소문자 처리에
의존하지 않는다. 잘못된 자원 이름을 건너뛰는 대신 원본 별칭을 수정하였다.
Qt 패키지는 정규 `nodesTest.svg` URL과 기존 소문자 `nodestest.svg` URL을 함께
제공한다. 두 URL에서 읽은 실제 SVG 바이트가 같은지도 검사한다.

`LVRS.native_icons`는 소문자 별칭 정규화와 원본 SVG 바이트의 일치를 검사하고,
기존 전체 아이콘 목록의 실제 래스터 검증도 유지한다. LVUI 키워드 목록은 수정된
LVRS 소스에서 다시 생성하고 검사한다.
Tooltip은 팝업의 기존 접근성·입력 계약을 사용한다. 불필요한 no-op 입력 처리기를
팝업에 설정하지 않으며 이름 갱신은 popover options로 전달한다. CodeEditor는
실제 텍스트 입력 제공자를 먼저 등록한 뒤 접근성 이름을 설정한다. 기본 머티리얼의
0 크기는 빈 절대 스택으로 유지하며 같은 인스턴스를 나중에 확장할 수 있다.

Windows 갤러리는 D3D11, macOS는 Metal, Linux는 Vulkan의 실제 API를 검사한다.
Alert의 실제 흐림·그림자 픽셀 검사는 모든 실행 플랫폼에서 유지한다. 보관된 soft-effect
재사용 계수는 이를 구현한 Metal에서 검사하고, Windows에서는 inline GPU effect의
실행 계수 증가를 검사한다. Metal 수준의 효과 보관 최적화는 Windows의 완료 기능으로
기록하지 않는다. 네이티브 대응표의 partial 상태도 그대로 유지한다.

Windows 문서 저장은 stage 읽기 핸들을 열기 전에 `ReplaceFileW` 게시를 완료한다.
대상 파일의 메타데이터 보존은 provider에 맡기고, 게시된 파일에서 새 읽기 소스를 연다.
이전 문서 소스의 읽기 핸들은 그대로 남아 undo가 원래 바이트를 읽을 수 있다.
[Microsoft의 replacement 파일 공유 규칙](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew)을 따른다.

FrameLoop 포화 검사는 플랫폼별 창 초기화 시점에 기대지 않고 남은 slot을 명시적으로
점유한 뒤 변경 거절과 상태 보존을 확인한다. Card의 custom preview는 하위 scope의
정규화된 id 문자열 대신 실제 factory가 반환한 위젯의 수명과 정체성을 검사한다.
표 갤러리는 Enter의 key_down·key_up 전체 수명으로 편집 제출을 확인한다.

Tooltip 타이머 검사는 Windows 창이 소유하는 16ms Core 이벤트 펌프를 기준 작업 수에
포함한다. 닫기·retirement 뒤 Tooltip의 작업은 모두 사라져야 하지만 살아 있는 Host의
플랫폼 이벤트 펌프가 0개가 되어야 한다고 요구하지 않는다.

표의 스크롤 알림에서는 현재 뷰포트 설정을 다시 쓰지 않고 표시 내용만 갱신한다.
스크롤 설정 변경은 모델·옵션 변경 경로에서 수행한다. 알림 중 재진입 거절 때문에
창이 종료되던 문제를 10만 행 갤러리와 직접 뷰포트 스크롤 회귀 검사로 확인한다.
내부 표 구성 예외는 원래 오류 메시지를 보존하여 호스트 진단에 전달한다.

Qt 호환 텍스트 모델은 LF를 포함한 UTF-8 원시 바이트로 저장한다. Windows의 텍스트
모드 줄바꿈 변환이 파일 기반 행 인덱스를 어긋나게 하지 않도록 저장 바이트와 한글·
이모지 행을 함께 검사한다. 아이콘 기준 파일도 실제 `nodesTest.svg` 대소문자를 따른다.
포인터 기반 Qt 검사는 창 노출 뒤 활성화까지 기다린다. 사용자 정의 제목 영역의
버튼은 기존 `windowDragExclusionItems` 계약으로 창 이동 영역에서 제외한다.
활성화한 포커스 캡처 검사는 임의의 100ms 대기 대신 실제 프레임의 외부 윤곽 픽셀을
기존 기준으로 확인할 때까지 기다린다. 전체 프레임과 여섯 구성 요소의 crop을 보관한다.

# 네이티브 텍스트 문서 기반

TextDocumentModel과 TextMarkup을 C++23·Qt 없는 경로에 구현한다. 이는 기존 TextEditor와
CodeEditor를 받치는 문서·문자 경계 기반이다. PlainText/NoWrap 화면은 별도의
[네이티브 CodeEditor](CodeEditor.md)에 연결했다. RichText TextEditor의 문자 서식·줄 바꿈과
전체 선택·IME·플랫폼 제스처 동등성까지 완료했다는 뜻은 아니다. 기존 QML 구현은 유지한다.

## 문서와 위치

`line_info`는 원본 읽기 없이 줄의 identity·시작·길이·디코딩 바이트 수를 조회한다.
`line_index`는 모델의 Unicode 캐시를 공유하는 불변 TextIndex를 반환한다. 원본 줄이
수정되어도 이미 얻은 인덱스는 유지된다. 이 경계로 화면 캐시와 저장 데이터의 수명을 분리한다.
줄 끝·시작과 ASCII 줄의 위치 검증은 이미 확인한 메타데이터를 사용하므로 위치 확인만을
위해 원본 줄 전체를 다시 읽거나 디코딩하지 않는다. 비 ASCII 줄의 내부 위치는 기존
Unicode 인덱스로 surrogate 경계를 검사한다.

TextDocumentModel::create는 소유 스레드의 Core EventLoop와 옵션, 선택적 저장소를 받는다.
그 루프는 모델보다 오래 존재하고 같은 스레드에서 디스패치되어야 한다. 공개 경계는 UTF-8
문자열과 네이티브 filesystem::path이며 커서·선택 범위는 UTF-16 단위이다. 대리 쌍을 가르는
위치는 오류이다. 문자소 이동·삭제는 iiGUI TextIndex를 사용하며 결합 문자·한글 자모·ZWJ
이모지를 분리하지 않는다. 세로 이동은 대상 줄의 대리 쌍 중간에 멈추지 않도록 조정한다.

빈 문서에도 한 줄이 존재한다. 메모리와 파일 입력의 CRLF·CR은 내부에서 LF로 정규화한다.
UTF-8 파일 앞의 BOM은 인코딩 표식으로 처리한다. 잘못된 파일 바이트는 한 바이트씩 U+FFFD로
대체한다. set_text/replace_range는 잘못된 UTF-8을 거절한다. Qt 디코더의 모든 잘못된 시퀀스
묶음, 줄 중간 BOM과 문자 경계 버전 차이까지 동일하다고 주장하지 않는다.

state는 경로·유무, 줄·UTF-16 문자 수, 파일/메모리 줄 수, 커서, dirty, 로드 상태·진행률,
오류, 청크 크기와 이력을 제공한다. line_at은 index/number/text/length/file_backed를 돌려준다.
line_text/line_length, text_range, position_for_line_column/line_column_for_position 및 단어
경계 조회를 제공한다. text는 명시적 전체 구체화이며 큰 문서 화면은 줄·범위 API를 사용해야 한다.

move_cursor와 set_cursor_position은 문서 내용·document_revision·dirty·이력을 변경하지 않는다.
insert_text/insert_newline/replace_range/문자소 삭제는 변경 구간을 반영한다. 동일한 값의
set_text와 범위 교체는 알림·이력을 추가하지 않는다. set_text는 전체 교체 API이며 이력을
초기화한다. 대화형 편집은 replace_range로 전달해야 한다.

## 파일 기반 저장과 작업 소유권

원본의 줄별 오프셋 계약을 유지한다. 청크마다 완성된 줄의 바이트 오프셋·길이·UTF-16 길이를
추가하며 원문 전체를 지속 문자열로 보관하지 않는다. 긴 미완성 줄은 한 버퍼에 이어 붙인다.
청크마다 누적 줄 버퍼를 복사하지 않는다. 줄 인덱스는 ID 해시를 우선순위로 하는 순서 트리이며
줄 수·문자 수·바이트 수·파일 줄 수를 누적한다. 위치 조회와 범위 접합은 기대 O(log n) 경로를
따르고, 구조 편집에서 전체 줄 목록을 다시 만들지 않는다. 수정한 줄만 메모리 줄이 된다.

load_file/reload_file은 Core의 소유 콜백으로 턴당 최대 load_chunk_size 바이트를 읽는다.
최초 작업 admission은 기존 문서를 교체하기 전에 확보한다. 후속 admission이나 읽기가 실패하면
실패를 기록하고 로드를 멈추며 이미 반영한 줄을 보존한다. 파일이 없으면 연결된 빈 문서가 된다.
cancel_load와 유효한 편집은 남은 로드 작업을 취소한다. 이 경우 미완성 줄은 반영하지 않는다.
loading 중 저장은 거절한다. 모델 폐기·close는 작업 admission을 닫고 대기 작업을 취소한다.
Core가 다른 스레드에서 로드 콜백을 디스패치하면 모델을 닫고 wrong_executor를 보고한다.

TextDocumentStorage/TextDocumentSource/TextDocumentWrite는 교체 가능한 인터페이스이다.
기본 구현은 iiFileProvider Native 0.9 이상이며 생성·쓰기·임시 파일·원자적 publication은 모두
해당 모듈을 통한다. LVRS·iiGUI의 하위 모듈이 LVRS를 역참조하지 않는다. 저장소 구현 형식은
공개 모델 API로 노출하지 않는다. CMake 소비자는 iiFileProviderNative 패키지도 제공해야 한다.

save_file은 줄을 스트리밍해 staging에 쓰고 성공 시 한 번에 공개한다. Windows에서는
FILE_SHARE_DELETE를 갖는 읽기 핸들, POSIX에서는 열린 inode를 보존한다. 공개 전에 새 읽기
핸들도 확보하므로 공개 성공 뒤 재개방 실패로 결과가 뒤바뀌지 않는다. 저장은 내용과 커서를
바꾸지 않고 파일 기반 레코드를 재연결하며 document_revision을 증가시키지 않는다.

원본 핸들은 경로의 원자적 교체 후에도 이전 버전을 읽을 수 있다. 외부 프로세스의 동일 inode
제자리 쓰기는 잠금으로 격리되지 않는다. 캐시되지 않은 줄을 읽을 때 크기와 바이트 지문을
확인해 인덱스와 다른 값을 거절한다. 이는 외부 쓰기의 완전한 트랜잭션 잠금을 뜻하지 않는다.

## 이력·알림·예산

범위 편집 이력은 트리 경로를 공유한다. undo/redo는 내용과 커서를 복원하고 저장된 clean
체크포인트와 dirty를 비교한다. 저장 때문에 이전 파일 핸들이 끊어지지 않는다. 이력 개수와
보존 비용 예산으로 오래된 항목을 제거한다. 비용은 보존 줄 레코드·문자열에 대한 보수적 산정이며
프로세스 RSS 측정값은 아니다. 예산보다 큰 단일 편집은 반영하되 해당 이력을 보존하지 않는다.

TextDocumentPort는 약한 구독이며 변경 구간, 내용/커서/저장소/이력 여부와 작업 경로를 받는다.
범위 교체는 선택적인 `edit` 메타데이터도 제공한다. `first`·`removed`·`inserted`는 UTF-16
단위이며 삽입 길이는 CRLF/CR 정규화 후 값이다. `transaction`은 이미 존재하는 단조 증가
clean/history 식별자를 재사용하고 undo/redo에서도 원본 편집의 식별자를 유지한다.
`operation`은 replace/undo/redo, `undoable`은 해당 범위 이력의 보존 여부이다. 전체 reset,
청크 로드, 저장·탐색·clean 체크포인트에는 범위 교체 메타데이터를 합성하지 않는다.
`TextDocumentEdit.h`의 위치·선택 매핑은 문자를 복사하거나 문서를 전체 조회하지 않는다.
64비트 단위의 오버플로와 유효하지 않은 affinity를 거절하며 호출자는 scalar-valid한 원본
위치와 편집을 전달해야 한다. 선택은 경계 삽입을 제외하고 방향을 보존하며 접힌 커서는 삽입
뒤를 따른다. 중첩해 사라진 선택은 교체 뒤 경계에 접힌다. 이는 원본 문자 복원이 필요한
선택 undo의 대체물이 아니므로 CodeEditor는 별도의 제한된 뷰 메타데이터를 사용한다.

경로 속성을 다시 연결하더라도 진행 중인 로드의 이벤트 경로는 실제 작업 경로이다. 알림 중
조회는 허용하고 재진입 수정은 busy이다. 수신자 오류는 이미 반영된 명령의 오류이므로 무조건
재시도해서는 안 된다. 실패한 수신자 뒤의 살아 있는 구독에도 변경을 전달한 후 최초 오류의
원인을 반환한다. close는 admission을 닫고 객체 해제는 남은 소유 데이터도 반환한다.

기본 청크는 64 KiB, 파일은 1 TiB, 줄은 400만 개, 한 줄·명시적 구체화는 각각 64 MiB이다.
조회 인덱스 캐시는 16 MiB·4096줄, 이력은 128개·64 MiB 보존 비용이다. 초과는 Result 오류로
보고하며 조용히 잘라내지 않는다. 호출자는 명시적 구체화·파일·캐시 예산을 조정할 수 있다.
stats는 소스 읽기·캐시·인덱스 방문·경로 복제·구체화 횟수를 제공한다. 이는 지연 시간 실측이 아니다.

## TextMarkup

normalize는 줄바꿈과 br/p, strong/b, em/i, code 태그를 기존 마커로 정규화하고 나머지
태그를 제거한다. render_plain_text는 기존 계약대로 마크다운 마커를 유지한다. render_html은
이스케이프한 텍스트에서 **bold**, __bold__, *em*, _em_, `code`와 줄바꿈을 변환한다.
코드 구간은 강조 변환에서 보호한다. 사용자 텍스트의 {{{CODE_0}}} 같은 리터럴을 내부 토큰으로
오인하지 않는다. 재귀 정규식이나 전체 문자열의 반복 치환을 사용하지 않는다.

입력은 8 MiB·UTF-8, 출력은 64 MiB로 제한한다. HTML 브라우저·전체 Markdown·리치 문서
레이아웃 엔진을 제공하지 않는다. 복잡하게 겹친 강조 구분자와 모든 잘못된 HTML의 Qt 동등성은
남아 있다. Unicode 소스는 iiGUI에 고정한 utf8proc를 사용하며 Qt나 상위 기능 모듈에 의존하지 않는다.

## 검증 상태

문서·문자 경계·마크업 검사 코드는 컴파일하고 실행은 전체 LVRS 작업의 마지막으로 보류한다.
일반·ASan/UBSan 컴파일, 설치 소비자와 패키지 정적 확인을 실행 성공과 구분한다. 두 편집기의
네이티브 화면과 전체 상속·시그널·플랫폼 동등성은 이후 같은 목표에서 계속 구현한다.

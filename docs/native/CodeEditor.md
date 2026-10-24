# 자체 네이티브 CodeEditor

`lvrs::CodeEditor`는 Qt 없는 PlainText·NoWrap 편집 위젯이다. 기존 `CodeEditor.qml`과 현행 Theme를 기준으로 스니펫 제목·언어, 최소 기본 크기 180×144, 헤더 22, 간격 4, 좌우 여백 10, 상하 여백 8, 모서리 8, 본문 13/500을 반영한다. Theme의 textLineHeight는 13, centeredTextHeight는 16이며 네이티브 행 상자는 두 값 중 큰 값으로 유지한다. 기본 centeredTextY는 77이다. macOS는 Menlo, 다른 데스크톱은 Monospace 기본값이다. 본문/선택 문자 0.9, 플레이스홀더/헤더 0.6, 비활성 0.3 투명도와 원본 subSurface/surfaceAlt RGBA를 사용한다. iiGUI의 자체 GPU 위젯과 네이티브 폰트 서비스만 사용하며 NSTextView, EDIT, Qt TextEdit를 생성하지 않는다.

문서 내용·파일 읽기·저장·undo/redo는 `TextDocumentModel`, 선택·커서·조합 문자열·스크롤·캐시는 각 뷰가 소유한다. `CodeEditorOptions::model`로 모델을 공유할 수 있다. 모델이 없으면 `WidgetBuilder::event_loop()`의 iiSDKCore 루프에 연결한 모델을 만든다. `text`는 생성 시 초기 값이며 `set_options`로 재주입하지 않는다. 내용은 `set_text`, 모델 교체는 새 뷰 생성으로 명시한다.

보이는 줄만 배치한다. 줄의 불변 identity와 Unicode 인덱스는 모델의 `line_info`·`line_index`에서 얻는다. `line_info`는 원본 읽기 없이 인덱스 메타데이터만 조회한다. 깊은 세로 이동은 거대한 물리 좌표나 줄별 위젯 생성으로 변환하지 않는다. 가로 이동은 `iiGUI::TextRasterCache`를 통해 잘린 범위만 네이티브 래스터화한다. 본문과 선택 문자의 RGB가 같으면 이미지를 공유하고, RGB가 다르면 각 네이티브 결과를 캐시하여 crop으로 나눈다. 이모지 팔레트를 흰색 마스크나 RGB tint로 덮어쓰지 않으며 투명도는 합성 속성으로 분리한다. 커서 깜박임·같은 팔레트의 선택 이동은 글리프 이미지를 바꾸지 않는다. 새로운 RGB에는 최초 래스터화가 필요할 수 있다. 실제 컬러 글꼴 형식과 fallback 지원은 운영체제의 폰트 서비스에 따른다.

문서가 큰 경우 `onDocumentEdited`/`document_edited`의 변경 범위 통지를 우선한다. 전체 문자열 `text()`·`text_edited`·`submitted`는 명시적으로 요청하거나 해당 콜백을 등록한 경우에만 물질화하며 모델의 용량 한계를 따른다. 선택·스크롤·커서 이동은 문서 수정 revision·저장·편집 history를 만들지 않는다. 모델의 통지 중에는 읽기만 허용하며 동기 재편집은 `busy`이다.

범위 편집 통지의 `TextDocumentEdit`는 실제 UTF-16 교체 시작·제거 길이·정규화한 삽입 길이와 기존 문서 이력의 transaction 식별자를 포함한다. 외부 편집 시 `map_text_selection`으로 각 뷰의 선택을 재배치한다. 선택의 앞/뒤 경계에만 삽입된 내용은 기존 선택에 넣지 않고, 접힌 커서는 삽입 뒤로 이동한다. 역방향 선택은 방향을 유지한다. undo/redo는 편집을 시작한 뷰의 이전/이후 선택을 복원하고 다른 뷰는 역방향/정방향 교체를 반영한다. 문서 포맷·저장과 뷰의 선택은 서로 별도이다.

뷰별 선택 이력은 `selection_history_entries` 기본 128개, 최대 4096개이며 0은 비활성화이다. `selection_history_size`는 보유한 메타데이터 개수이다. 오래된 항목은 회수하며 문서 reset·전체 이력 삭제에서 비운다. 이 예산은 문서의 undo 데이터 예산과 별개이므로 메타데이터가 먼저 회수되어도 내용 undo는 유지하고 선택은 일반 범위 재배치로 처리한다. 빈 undo/redo는 커서와 뷰 revision을 바꾸지 않는다. 알림 수신자 하나가 실패해도 나머지 살아 있는 수신자에게 이미 반영된 변경을 전달한 뒤 오류를 반환한다. 오류 결과를 무조건 재시도해서는 안 된다.

UTF-16 위치를 사용하고 surrogate 중간은 거절한다. 화살표·삭제는 확장 자소 경계, 단어 이동은 모델의 Unicode 경계를 따른다. 세로 이동은 선호하는 시각 x 좌표를 유지한다. Shift 범위 선택, Home/End, 문서 시작/끝, PageUp/Down, SelectAll, Tab, Enter, Ctrl/Cmd+Enter 제출, overwrite, copy/cut/paste, undo/redo를 제공한다. readOnly일 때 선택·복사는 가능하다. 파일 로딩 중·실패·부분 취소 상태에서는 입력을 막아 읽은 접두부를 조용히 편집하지 않는다. 이 위젯은 자동 저장하지 않는다.

IME preedit는 원본 범위를 대체하는 가상 투영이다. 줄 추가·선택·밑줄·주변 문자열·후보 창 caret을 뷰에만 반영한다. commit·focus loss·Cocoa unmark에서 한 번의 모델 교체로 확정하고 Escape/빈 preedit 취소는 내용을 수정하지 않는다. 이미 표시 중인 marked range와 다른 명시적 replacement range는 기존 조합을 먼저 확정한 후 새 조합을 시작한다. 외부 문서 변경은 미확정 조합을 취소한다. 실제 플랫폼 입력기로 확인하는 작업은 최종 실행 단계에 남아 있다.

커서 깜박임은 Core TaskScope에 소유된 500ms 단일 예약을 연결한다. 포커스/편집 가능 상태를 잃으면 예약을 취소한다. 예약이 거절되면 커서를 고정 표시하며 오류 상태를 남긴다. 뷰 종료는 작업·관찰 포트를 회수하고 이전 핸들은 `closed`를 반환한다.

기본 예산은 단락 1 MiB, shape 캐시 512개/추정 96 MiB, 래스터 캐시와 한 프레임의 글리프 이미지 32 MiB, 보이는 줄 512개이다. 초과한 단락은 오류를 표시하고 내용을 잘라 편집하거나 저장하지 않는다. 캐시 수치는 보유한 객체의 보수적인 admission 비용이며 드라이버·폰트 서비스 전체 메모리 계측이 아니다. 가로 범위는 방문해 배치한 줄의 최대 너비로 증가한다. 모든 파일 줄의 정확한 최대 너비를 얻기 위한 전체 스캔은 수행하지 않는다.

원본과의 완전 동등성은 아직 충족하지 않았다. 사용자 cursorDelegate, 모든 상속 TextEdit API, 내용 기반 implicitWidth, fontStyleName/letterSpacing, 모바일 네이티브 제스처와 입력 힌트, double/triple click 선택, 모든 bidi caret affinity, 관성 이동과 고급 접근성은 남아 있다. 새 선택 재배치·undo 복원·팔레트/투명도 경로도 실제 플랫폼 입력과 렌더링 검증은 마지막 일괄 단계까지 보류한다. 헤더는 고정하고 본문을 헤더 아래에서 clip하므로 원본 Flickable 전체 영역의 겹침 동작과 다를 수 있다. `TextEditor`의 RichText/Wrap·서식 편집·파일 자동 저장은 별도 구현 대상이다. 이 PlainText 위젯으로 이를 대체하지 않는다.

`tests/native/code_editor.cpp`, `tests/native/code_projection.cpp`, iiGUI 텍스트 배치/입력 공급자 테스트와 설치 소비자를 함께 빌드한다. 사용자 지시에 따라 실행은 전체 LVRS 이식의 마지막 일괄 단계로 보류한다. 중간 컴파일·설치 스테이징·심벌 감사는 시각/입력 동작의 실행 통과 증거가 아니다.

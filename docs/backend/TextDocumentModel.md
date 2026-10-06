# TextDocumentModel

위치: `src/backend/text/textdocumentmodel.h`, `src/backend/text/textdocumentmodel.cpp`

`TextDocumentModel`는 `LV.TextEditor` 뒤에 있는 내부 C++ 문서 엔진입니다.

<a id="purpose"></a>

## 목적

- 연결된 `filePath`, 문서 줄 인덱스, 커서 위치, 더티 상태, 지연 로드 상태 및 파일 오류를 소유합니다.
- 가상화된 QML 뷰포트에 대해 문서 줄을 `QAbstractListModel`로 노출합니다.
- 파일을 바이트 청크 단위로 점진적으로 로드하고 `QString` 라인 스토리지를 유지하는 대신 깨끗한 파일 라인을 파일 오프셋 레코드로 유지합니다.
- QML를 문서 저장 역할에서 제외하세요. QML는 입력만 렌더링하고 전달합니다.

## API

속성:

- `filePath`, `hasFilePath`
- `text`, `characterCount`
- `lineCount`
- `fileBackedLineCount`, `memoryLineCount`
- `cursorLine`, `cursorColumn`, `cursorPosition`
- `dirty`
- `lastError`
- `loading`
- `loadedByteCount`, `totalByteCount`, `loadProgress`
- `loadChunkSize`

모델 역할:

- `lineIndex`
- `lineNumber`
- `text`
- `length`

방법:

- `lineText(line)`, `lineLength(line)`
- `textRange(start, end)`
- `positionForLineColumn(line, column)`
- `previousWordBoundaryPosition(position)`, `nextWordBoundaryPosition(position)`
- `loadFile(path?)`, `reloadFile()`, `saveFile(path?)`, `cancelLoad()`
- `markClean()`, `clear()`
- `moveCursor(line, column)`, `moveCursorLeft()`, `moveCursorRight()`, `moveCursorUp()`, `moveCursorDown()`
- `moveCursorLineStart()`, `moveCursorLineEnd()`
- `moveCursorDocumentStart()`, `moveCursorDocumentEnd()`
- `moveCursorWordLeft()`, `moveCursorWordRight()`
- `insertText(value)`, `insertNewline()`
- `replaceRange(start, end, value)`
- `removePreviousCharacter()`, `removeNextCharacter()`

신호:

- `fileLoaded(path, length)`, `fileLoadFailed(path, error)`
- `fileLoadProgress(path, loadedBytes, totalBytes)`
- `fileSaved(path, length)`, `fileSaveFailed(path, error)`

<a id="behavior-contract"></a>

## 행동 계약

- 파일은 UTF-8 텍스트로 읽혀집니다.
- `loadFile()`는 대상 파일을 열고 청크 로드를 예약한 후 반환합니다. 하나의 전체 문서 `QString`를 미리 작성하지 않습니다.
- `loadFile()`는 존재하지 않는 대상 경로를 빈 연결된 문서로 처리하므로 `LV.TextEditor`는 첫 번째 편집 시 파일을 생성할 수 있습니다.
- 로드는 이벤트 루프 턴당 최대 `loadChunkSize` 바이트를 읽고 청크가 도착할 때 디코딩된 라인을 목록 모델에 추가합니다.
- 깔끔하게 로드된 라인은 `(byteOffset, byteLength, length)` 레코드로 저장되며 `lineText()`, 모델 대표 또는 `text`가 콘텐츠를 요청할 때만 디코딩됩니다.
- 편집은 터치된 라인 레코드만 메모리 지원 텍스트 레코드로 승격시킵니다. 손대지 않은 파일 기반 라인은 소스 파일에 연결된 상태로 유지됩니다.
- `loading`는 최종 청크가 디코딩되거나 `cancelLoad()`/failure가 스트림을 중지할 때까지 true입니다.
- `loadedByteCount`, `totalByteCount` 및 `loadProgress`는 바이트 수준 로드 진행 상황을 보고합니다.
- `text` 속성은 내부 엔진 표면이며 읽을 때 로드된 모든 라인을 구체화합니다. 대용량 문서 UI는 모델 행, `lineText(line)` 및 진행/저장 속성을 선호해야 합니다.
- 저장은 `QSaveFile`를 사용하므로 성공적인 쓰기는 호출자 관점에서 원자적입니다.
- 뉴라인은 모델 내부에서 `\n`로 정규화됩니다.
- 빈 문서인 경우에도 모델에는 항상 최소한 한 줄이 포함됩니다.
- 편집 작업은 라인 스토리지를 변경하고 `dirty`를 설정합니다.
- `replaceRange()`는 `LV.TextEditor` 선택 및 IME 커밋 처리에 사용됩니다. 가능한 경우 손대지 않은 라인 레코드를 보존하고 변경된 범위만 메모리 지원 레코드로 승격합니다.
- `textRange()`는 관련 없는 문서 텍스트를 구체화하지 않고 모델 기반 클립보드 복사에 사용됩니다.
- 단어 경계 이동은 한글 음절을 포함한 Unicode 문자와 숫자를 단어 문자로 취급하고, 문서 전체를 구체화하는 대신 라인 레코드를 통해 경계를 해결합니다.
- 단일 문자 이동 및 삭제는 Unicode 문자소 경계를 사용하므로 대리 쌍 문자는 커서 이동, 백스페이스 또는 삭제로 인해 분할되지 않습니다.
- `loadFile()`는 라인 스토리지를 재설정하고, 커서를 문서 시작으로 재설정하고, `dirty`를 지우고, 파일 콘텐츠를 행으로 스트리밍하고, 스트림이 완료되면 `fileLoaded`를 내보냅니다.
- 편집 작업은 로드된 텍스트를 변경하기 전에 `cancelLoad()`를 호출합니다. 부분 문서를 작성하는 대신 `loading`가 실패하는 동안 저장이 실패합니다.
- `saveFile()`는 현재 라인 레코드를 `QSaveFile`로 스트리밍하고, 저장된 파일에 대한 파일 지원 라인 레코드를 다시 작성하고, `dirty`를 지우고, `fileSaved`를 내보냅니다.

<a id="consumer"></a>

## 소비자

- `LV.TextEditor`는 하나의 `TextDocumentModel` 인스턴스를 소유하고 이를 QML `ListView`의 지원 모델로 사용합니다.
- 애플리케이션 코드는 이 모델에 연결하는 대신 더 작은 `LV.TextEditor` 파일 API(`filePath`, 선택적 `read()`, 자동 동기화 신호)를 사용해야 합니다.

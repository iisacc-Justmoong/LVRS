# TextEditor

위치: `src/qml/components/control/input/TextEditor.qml`

`TextEditor`는 파일 연결 리치 텍스트 편집기입니다. Qt Quick `TextEdit` 표면을 `TextEdit.RichText`에 고정하여 사용하므로 편집 동작은 코드나 일반 텍스트 버퍼보다는 Mac TextEdit 스타일의 풍부한 문서 편집에 더 가깝습니다.

<a id="purpose"></a>

## 목적

- 편집 가능한 서식 있는 텍스트 화면을 파일 시스템 경로에 직접 바인딩합니다.
- 내부 C++ 문서 엔진을 통해 UTF-8 HTML/서식 있는 텍스트를 로드하고 원자성 연속 쓰기 의미 체계를 통해 편집 내용을 동기화합니다.
- 서식 있는 텍스트 렌더링, 래핑, IME, 선택, 커서 이동, 클립보드 명령 및 플랫폼 텍스트 제스처를 위해 네이티브 `TextEdit` 편집기를 사용하세요.
- 네이티브 Return/Enter 처리는 삽입된 줄에 캐럿을 남겨둡니다. 래퍼는 커서를 이전 줄로 다시 고정하는 방식으로 문서를 대체해서는 안 됩니다.
- 파일 I/O, 읽기 진행 상황, 더티 상태, 동기화 오류를 시각적 편집기에서 제외하세요.
- 일반 텍스트 편집은 `CodeEditor` 또는 더 간단한 입력 구성 요소에 맡기십시오.

<a id="core-api"></a>

## 코어 API

파일 연결:

- `filePath`(필수): 연결된 파일 시스템 경로. `LV.TextEditor`는 이 속성을 사용하여 생성되어야 합니다.
- `chunkSize`: 지연 읽기 단계당 바이트 예산. 내부 모델은 매우 작거나 매우 큰 값을 고정합니다.

서식 있는 텍스트 화면:

- `editorItem`(읽기 전용 별칭): 고급 서식 있는 텍스트 통합을 위한 기본 `TextEdit`입니다.
- `text`: 편집기에 표시된 서식 있는 텍스트/HTML 문서 문자열입니다.
- `textFormat`(읽기 전용): 항상 `TextEdit.RichText`.
- `textDocument`: 내부 `TextEdit`가 노출하는 네이티브 문서 객체이다.
- 선택, 커서 및 클립보드 호환성 별칭은 네이티브 `TextEdit` 표면을 반영합니다.

읽기/동기화 상태:

- `dirty`: 로컬 편집이 파일 시스템 동기화를 기다리는 동안 true입니다. `read()` 성공 또는 자동 동기화 후 false입니다.
- `reading`: 연결된 파일이 청크 단위로 디코딩되는 동안 true입니다.
- `bytesRead`, `bytesTotal`, `progress`: 바이트 수준 읽기 진행.
- `error`: 가장 최근의 읽기/동기화 오류입니다.
- `empty`: 편집기에 서식 있는 텍스트 문자가 포함되어 있지 않으면 true입니다.
- `documentRevision`: 편집자가 만든 문서 편집에 대해 단조롭게 증가합니다. 파일 읽기는 진행되지 않습니다.

방법:

- `read()`: `filePath`를 다시 로드합니다. 일반 구성 및 `filePath` 변경 사항은 이미 자동으로 읽혀졌습니다.
- `forceEditorFocus()`, `insertText(value)`, `selectAll()`, `copy()`, `paste()`, `undo()` 및 `redo()`는 네이티브 편집기로 전달됩니다.

신호:

- `readFinished(path)`
- `readFailed(path, error)`
- `readProgress(path, bytesRead, bytesTotal)`
- `syncFinished(path)`
- `syncFailed(path, error)`
- `textEdited(text)`
- `documentEdited(documentText, documentRevision)`

<a id="api-usage-manual"></a>

## API 사용 설명서

<a id="import"></a>

### 가져오기

`TextEditor`는 LVRS QML 모듈에서 내보내집니다.

```qml
import LVRS 1.0 as LV
```

<a id="create-an-editor"></a>

### 편집기 만들기

`filePath`가 필요합니다. 편집기가 분리된 텍스트 버퍼가 아닌 직접 파일 시스템 편집기로 정의되므로 이 구성 요소가 없으면 구성 요소가 유효하지 않습니다.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
}
```

<a id="rich-text-editing"></a>

### 리치 텍스트 편집

편집 표면은 항상 `TextEdit.RichText`입니다. 프로그래밍 방식으로 콘텐츠를 설정할 때 HTML를 바인딩하거나 할당합니다.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
    text: "<h1>Notes</h1><p>Hello <b>bold</b> text.</p>"
}
```

애플리케이션 코드에 Qt Quick `TextEdit`의 하위 수준 서식 있는 텍스트 동작이 필요한 경우 `editorItem`를 사용하세요.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"

    Component.onCompleted: {
        editor.editorItem.selectAll()
    }
}
```

<a id="read-the-connected-file"></a>

### 연결된 파일 읽기

편집기는 생성 후 연결된 파일을 자동으로 읽습니다.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
}
```

호출자가 명시적으로 `filePath`를 다시 로드하려는 경우에만 `read()`를 호출하세요. 파일 스트림이 열리고 지연 로딩이 예약되면 `true`를 반환합니다. 완료는 `readFinished(path)`를 통해 보고됩니다.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"

    Component.onCompleted: {
        if (!read())
            console.warn(error)
    }

    onReadFinished: console.log("Read", path)
    onReadFailed: console.warn("Read failed", path, error)
}
```

<a id="realtime-output"></a>

### 실시간 출력

공개 저장 API 이 없습니다. 텍스트 편집은 `filePath` 로 자동으로 동기화됩니다. 편집기는 성공적인 쓰기 통과 단계 후 `syncFinished(path)` 를 방출하며, 파일 시스템 쓰기가 실패하면 `syncFailed(path, error)` 를 방출합니다. 네이티브 편집기 변경과 같은 이벤트 턴에서 정확한 편집된 문서를 필요로 하는 소비자는 `documentEdited(documentText, documentRevision)` 를 사용해야 합니다. 신호는 최신 리치 문서 페이로드와 단조롭게 증가하는 수정을 운반하므로 하위 소비 측 코드는 해당 쌍을 재독할 수 있는 바인딩 대신 권위 있는 것으로 취급할 수 있습니다. 원생 입력 방법 조합이 활성화된 동안, 신호 및 파일 쓰기 통과가 편집자 표면에서 커밋된 문서 페이로드가 표시될 때까지 연기됩니다.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
    onDocumentEdited: function(documentText, documentRevision) {
        console.log("Document revision", documentRevision, documentText.length)
    }
    onSyncFinished: console.log("Synchronized", path)
    onSyncFailed: console.warn("Sync failed", path, error)
}
```

<a id="track-state"></a>

### 상태 추적

파일 상태에 대한 읽기/동기화 상태 속성을 사용합니다.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
    chunkSize: 65536

    onReadProgress: console.log(bytesRead, bytesTotal)
}

LV.ProgressBar {
    value: editor.progress
    visible: editor.reading
}

LV.Label {
    text: editor.dirty ? "Syncing" : "Synced"
}
```

<a id="work-with-large-files"></a>

### 대용량 파일 작업

지연 로딩을 조정하려면 `chunkSize`를 설정하십시오. 기본값은 일반 리치 텍스트 문서에 적합합니다. 청크가 클수록 스케줄링 오버헤드가 줄어들고, 청크가 작을수록 각 이벤트 루프 단계가 더 짧아집니다.

```qml
LV.TextEditor {
    filePath: "/tmp/large-notes.html"
    chunkSize: 262144
}
```

`reading`가 true인 동안 편집자 시작 동기화를 건너뛰므로 부분 문서가 실수로 작성되지 않습니다.

<a id="change-files"></a>

### 파일 변경

`filePath`를 변경하면 연결된 파일이 변경되고 자동으로 새 경로를 읽습니다. 편집 내용은 지속적으로 동기화되므로 호출자는 일반적으로 이전 동기화 실패를 처리할 때 경로 변경 사항만 보호하면 됩니다.

```qml
function openPath(path) {
    if (editor.dirty)
        return

    editor.filePath = path
}
```

<a id="read-only-view"></a>

### 읽기 전용 보기

편집기가 텍스트 편집을 허용하지 않고 서식 있는 텍스트 파일을 로드하고 표시해야 하는 경우 `readOnly`를 설정합니다.

```qml
LV.TextEditor {
    filePath: "/tmp/report.html"
    readOnly: true
}
```

<a id="not-public-api"></a>

### 공개되지 않음 API

애플리케이션 코드는 내부 `TextDocumentModel`에 의존해서는 안 됩니다. 파일 로딩, 진행, 더티 상태 및 원자 동기화를 제공하기 위해 존재합니다. 서식 있는 텍스트 편집기 동작과 파일 상태에 대한 읽기/동기화 속성에는 `editorItem`를 사용하세요.

이러한 이름은 의도적으로 `LV.TextEditor`의 일부가 아닙니다.

- `write()`, `loadFile(path)`, `saveFile(path)`, `reloadFile()`
- `mode`, `markdownMode`, `plainTextMode`, `renderedOutput`와 같은 모드 또는 미리보기 API
- 다른 이름으로 저장 경로 매개변수

레이아웃/시각적:

- `placeholderText`, `readOnly`
- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `insetHorizontal`, `insetVertical`
- `shapeStyle`, `cornerRadius`
- `showScrollBar`, `autoFocusOnPress`
- `preferNativeGestures`, `preferNativeTextInteraction`
- `viewportBoundsBehavior`, `viewportBoundsMovement`
- `viewportFlickDeceleration`, `viewportMaximumFlickVelocity`
- 서식 있는 텍스트 뷰포트의 글꼴 및 색상 토큰

<a id="behavior-contract"></a>

## 행동 계약

- 편집기는 생성 후 및 `filePath` 변경 후 자동으로 `read()`를 호출합니다.
- `read()`는 필수 `filePath`에서 UTF-8 서식 있는 텍스트/HTML를 열고, `dirty`를 지우고, `error`를 지우고, 청크 로드를 예약합니다.
- 존재하지 않는 경로를 읽으면 파일을 생성하지 않고 빈 문서를 해당 경로에 연결합니다. 상위 디렉터리에 쓰기가 가능한 경우 첫 번째 편집에서는 동기화를 통해 파일이 생성됩니다.
- 로컬 편집은 `QSaveFile`를 통해 `filePath`에 대한 자동 연속 쓰기 동기화를 예약합니다. 성공적인 동기화는 `dirty`를 지우고, `error`를 지우고, `syncFinished`를 내보냅니다.
- 로컬 편집은 내부 문서 모델이 최신 편집기 페이로드를 받은 후 `documentEdited(documentText, documentRevision)`를 내보냅니다. `documentText`는 해당 편집 차례에 대한 신뢰할 수 있는 서식 있는 문서 텍스트이며, `documentRevision`는 내보낸 편집자 원본 편집마다 한 번씩 증가합니다.
- 입력 방법 구성 중에는 구성이 확정될 때까지 로컬 편집 게시가 유지됩니다. 그런 다음 내부 문서 모델, 파일 동기화 및 `documentEdited(...)`는 커밋된 문서 페이로드를 함께 수신하여 아직 커밋되지 않은 한국어 음절과 같은 부분 사전 편집 텍스트를 방지합니다.
- `reading`가 true인 동안 동기화를 건너뛰므로 실수로 문서 일부가 작성되는 일이 없습니다.
- 편집 화면은 `textFormat: TextEdit.RichText`, `wrapMode: TextEdit.Wrap`, 마우스 선택, 영구 선택, IME 처리 및 클립보드 동작을 갖춘 네이티브 `TextEdit`입니다.
- 모바일 대상의 기본값은 `Theme.mobileTarget`를 따른다. 서식 있는 텍스트 콘텐츠가 넘치면 편집기에 포커스가 있는 동안을 포함하여 모바일 터치 드래그가 내부 뷰포트를 스크롤한다. 탭, IME 입력, 커서 배치 및 선택은 내부 네이티브 `TextEdit` 경로를 유지한다.
- `filePath` 없이 `LV.TextEditor`를 구성하는 것은 연결된 파일이 구성 요소 계약의 일부이기 때문에 잘못된 QML입니다.
- 빈 경로와 파일 읽기/동기화 실패는 의도적으로 현재 문서를 대체하지 않습니다. `error`를 설정하고 일치하는 실패 신호를 내보냅니다.

<a id="internal-engine"></a>

## 내부 엔진

`TextDocumentModel` 는 `LV.TextEditor` 에 의해 소유되며 공개 QML 컴포넌트 API 의 일부가 아닙니다. 테스트는 객체 이름을 통해 이를 찾아 저장 동작을 확인할 수 있지만, 애플리케이션 코드는 `LV.TextEditor` 를 파일 읽기/쓰기 상태 엔진으로 취급하고 편집 동작을 위해 네이티브 리치 `editorItem` 를 사용해야 합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    filePath: "/tmp/notes.html"
    chunkSize: 65536

    onSyncFinished: console.log("Synchronized", path)
}
```

<a id="shared-motion"></a>

## 공유 모션

초점 윤곽선만 움직입니다. 커서, 텍스트 선택, IME 및 스크롤은 네이티브으로 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

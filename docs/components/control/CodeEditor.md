# CodeEditor

위치: `src/qml/components/control/input/CodeEditor.qml`

`CodeEditor`는 선택적 스니펫 헤더가 있는 코드 지향 편집기(`TextEdit.NoWrap`, `TextEdit.PlainText`)입니다.

<a id="purpose"></a>

## 목적

- 고정폭 코드 편집을 결정적으로 유지합니다.
- 스니펫 메타데이터 헤더(`title/language`)를 제공하세요.
- 별칭을 통해 하위 수준 `TextEdit` API를 노출합니다.

<a id="core-api"></a>

## 코어 API

기본 별칭:

- `editorItem`(읽기 전용 별칭)
- `text`, `readOnly`, `cursorPosition`, `selectionStart`, `selectionEnd`, `selectedText`
- `contentWidth`, `contentHeight`, `lineCount`, `textDocument`, `canPaste`

코드별:

- 읽기 전용 `wrapMode` (`NoWrap`)
- 읽기 전용 `textFormat` (`PlainText`)
- `snippetTitle`, `snippetLanguage`, `showSnippetHeader`

레이아웃/시각적:

- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `headerHeight`, `headerSpacing`, `topInset`
- `insetHorizontal`, `insetVertical`
- `shapeStyle`, `cornerRadius`
- `showScrollBar`, `autoFocusOnPress`, `preferNativeGestures`, `preferNativeTextInteraction`
- 뷰포트 스크롤 물리학: `viewportFlickDeceleration`, `viewportMaximumFlickVelocity`
- 읽기 전용 뷰포트 정책: `viewportBoundsBehavior`, `viewportBoundsMovement`

신호 및 방법:

- `textEdited(text)`, `submitted(text)`
- `forceEditorFocus()`, `insertText(value)`, `clear()`, `select()`, `selectAll()`, `deselect()`, `cut()`, `copy()`, `paste()`, `undo()`, `redo()`, `submit()`

<a id="behavior-contract"></a>

## 행동 계약

- 제출 바로가기: `Ctrl+Enter` 또는 `Cmd+Enter`.
- 헤더 영역 높이는 `showSnippetHeader`가 true인 경우에만 상단 삽입에 포함됩니다.
- IME/스크롤 안전을 위한 `InputMethodGuard` + `WheelScrollGuard`가 포함되어 있습니다.
- 편집 표면은 전체 커버 `MouseArea`를 설치하지 않습니다. 포인터, IME, 선택 및 키보드 제스처는 기본 `TextEdit`에 의해 처리됩니다.
- 모바일 타겟 기본값은 이제 `Theme.mobileTarget` 를 따르며, iOS -타겟 실행 시 기본 `TextEdit` 는 `NativeRendering` 를 사용하고 편집기 뷰포트 는 네이티브 텍스트 모드에서 인터랙티브 터치 플릭을 취하지 않으므로 소프트웨어 키보드 편집 제스처, 반복 삭제, 및 텍스트 선택 제스처는 플랫폼 네이티브 경로에 유지됩니다.
- 모바일 대상 스크롤 기본값은 편집기 뷰포트에서 경계가 고정된 상태로 유지되는 동안(`StopAtBounds`) 플릭 모멘텀(`viewportFlickDeceleration`, `viewportMaximumFlickVelocity`)을 계속 조정합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.CodeEditor {
    snippetTitle: "main.cpp"
    snippetLanguage: "C++"
    text: "int main() { return 0; }"
}
```

<a id="shared-motion"></a>

## 공유 모션

편집기 프레임은 동일한 포커스 처리를 받습니다. 구문과 텍스트 업데이트가 즉시 이루어집니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

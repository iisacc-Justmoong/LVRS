import QtQuick
import QtQuick.Controls
import LVRS 1.0

FocusScope {
    id: control

    required property string filePath

    readonly property alias editorItem: editor
    property alias chunkSize: document.loadChunkSize
    property alias text: editor.text
    property alias readOnly: editor.readOnly
    property alias activeFocusOnPress: editor.activeFocusOnPress
    property alias cursorPosition: editor.cursorPosition
    property alias cursorRectangle: editor.cursorRectangle
    property alias cursorDelegate: editor.cursorDelegate
    property alias selectionStart: editor.selectionStart
    property alias selectionEnd: editor.selectionEnd
    property alias selectedText: editor.selectedText
    property alias length: editor.length
    property alias selectByMouse: editor.selectByMouse
    property bool selectByKeyboard: true
    property alias mouseSelectionMode: editor.mouseSelectionMode
    property alias persistentSelection: editor.persistentSelection
    property alias overwriteMode: editor.overwriteMode
    property alias textMargin: editor.textMargin
    property alias inputMethodHints: editor.inputMethodHints
    property alias inputMethodComposing: editor.inputMethodComposing
    property alias tabStopDistance: editor.tabStopDistance
    property alias baseUrl: editor.baseUrl
    property alias textDocument: editor.textDocument
    property alias hoveredLink: editor.hoveredLink
    property alias contentWidth: editor.contentWidth
    property alias contentHeight: editor.contentHeight
    property alias lineCount: editor.lineCount
    property alias canPaste: editor.canPaste

    readonly property alias dirty: document.dirty
    readonly property alias reading: document.loading
    readonly property alias bytesRead: document.loadedByteCount
    readonly property alias bytesTotal: document.totalByteCount
    readonly property alias progress: document.loadProgress
    readonly property alias error: document.lastError
    readonly property int documentRevision: privateState.documentRevision

    readonly property int textFormat: editor.textFormat
    property string placeholderText: ""

    property int fieldMinHeight: Theme.controlHeightMd * 4
    property int editorHeight: fieldMinHeight
    property int insetHorizontal: Theme.gap10
    property int insetVertical: Theme.gap8
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int cornerRadius: Theme.radiusMd
    property bool showScrollBar: true
    property bool autoFocusOnPress: true
    property bool preferNativeGestures: Theme.mobileTarget
    readonly property bool preferNativeTextInteraction: preferNativeGestures
        && Theme.effectiveRuntimeProfile.ios === true
    property int viewportFlickDeceleration: Theme.mobileTarget ? 1800 : 3200
    property int viewportMaximumFlickVelocity: Theme.mobileTarget ? 12000 : 8000
    readonly property int viewportBoundsBehavior: Flickable.StopAtBounds
    readonly property int viewportBoundsMovement: Flickable.StopAtBounds

    property string fontFamily: Theme.fontBody
    property int fontPixelSize: Theme.textBody
    property int fontWeight: Theme.textBodyWeight
    property string fontStyleName: Theme.textBodyStyleName
    property real fontLetterSpacing: Theme.textBodyLetterSpacing
    property real textLineHeight: Theme.textBodyLineHeight
    property int centeredTextHeight: Theme.scaleTextMetric(16)

    property color textColor: Theme.textPrimary
    property color textColorDisabled: Theme.textOctonary
    property color placeholderColor: Theme.textTertiary
    property color placeholderColorDisabled: Theme.textOctonary
    property real placeholderOpacity: 1.0
    property color selectionColor: Theme.accent
    property color selectedTextColor: Theme.textPrimary
    property color backgroundColor: Theme.subSurface
    property color backgroundColorHover: Theme.surfaceAlt
    property color backgroundColorPressed: Theme.accentBlueMuted
    property color backgroundColorFocused: backgroundColor
    property color backgroundColorDisabled: backgroundColor

    readonly property int textLineBoxHeight: Math.max(1, centeredTextHeight, Math.ceil(textLineHeight))
    readonly property int lineHeight: textLineBoxHeight
    readonly property int resolvedEditorHeight: Math.max(fieldMinHeight, editorHeight)
    readonly property bool focused: activeFocus || editor.activeFocus
    readonly property bool hovered: control.enabled && hoverHandler.hovered
    readonly property bool pressed: false
    readonly property bool empty: editor.length === 0
    readonly property color resolvedEditAreaBackgroundColor: !control.enabled
        ? control.backgroundColorDisabled
        : control.pressed
            ? control.backgroundColorPressed
            : control.focused
                ? control.backgroundColorFocused
                : control.hovered
                    ? control.backgroundColorHover
                    : control.backgroundColor

    signal readFinished(string path)
    signal readFailed(string path, string error)
    signal readProgress(string path, var bytesRead, var bytesTotal)
    signal syncFinished(string path)
    signal syncFailed(string path, string error)
    signal textEdited(string text)
    signal documentEdited(string documentText, int documentRevision)

    implicitWidth: Math.max(Theme.inputMinWidth, editor.implicitWidth + insetHorizontal * 2)
    implicitHeight: control.resolvedEditorHeight
    activeFocusOnTab: true

    onFilePathChanged: {
        if (privateState.completed && filePath.trim().length > 0) {
            syncTimer.stop()
            let canRead = true
            if (document.dirty && document.hasFilePath && !document.loading && !control.readOnly)
                canRead = document.saveFile(document.filePath)
            if (canRead)
                read()
        }
    }

    onActiveFocusChanged: {
        if (activeFocus && !editor.activeFocus)
            editor.forceActiveFocus()
    }

    Component.onCompleted: {
        privateState.completed = true
        if (filePath.trim().length > 0)
            read()
    }

    QtObject {
        id: privateState
        property bool completed: false
        property bool composingDocumentEditPending: false
        property int documentRevision: 0
        property bool syncingEditorFromDocument: false
        property bool syncingDocumentFromEditor: false

        function syncEditorFromDocument() {
            if (syncingDocumentFromEditor)
                return
            syncingEditorFromDocument = true
            editor.text = document.text
            syncingEditorFromDocument = false
        }

        function syncDocumentFromEditor(documentText) {
            syncingDocumentFromEditor = true
            document.text = documentText
            syncingDocumentFromEditor = false
        }

        function publishDocumentEdit(documentText) {
            if (editor.inputMethodComposing) {
                composingDocumentEditPending = true
                return
            }

            composingDocumentEditPending = false
            documentRevision += 1
            control.documentEdited(documentText, documentRevision)
        }

        function publishPendingComposedDocumentEdit() {
            if (!composingDocumentEditPending
                    || editor.inputMethodComposing
                    || syncingEditorFromDocument
                    || control.reading)
                return

            const documentText = editor.text
            if (document.text !== documentText)
                syncDocumentFromEditor(documentText)
            publishDocumentEdit(documentText)
        }
    }

    TextDocumentModel {
        id: document
        objectName: "textDocumentModel"
    }

    Timer {
        id: syncTimer
        interval: 0
        repeat: false
        onTriggered: {
            if (control.reading || control.readOnly || control.filePath.trim().length === 0)
                return
            document.saveFile(control.filePath)
        }
    }

    Connections {
        target: document

        function onTextChanged() {
            if (!privateState.syncingDocumentFromEditor)
                privateState.syncEditorFromDocument()
            if (!control.reading && !control.readOnly)
                syncTimer.restart()
        }

        function onFileLoaded(path, length) {
            privateState.syncEditorFromDocument()
            control.readFinished(path)
        }

        function onFileLoadFailed(path, error) {
            control.readFailed(path, error)
        }

        function onFileLoadProgress(path, loadedBytes, totalBytes) {
            control.readProgress(path, loadedBytes, totalBytes)
        }

        function onFileSaved(path, length) {
            control.syncFinished(path)
        }

        function onFileSaveFailed(path, error) {
            control.syncFailed(path, error)
        }
    }

    function read() {
        return document.loadFile(filePath)
    }

    function forceEditorFocus() {
        editor.forceActiveFocus()
    }

    function clearSelection() {
        editor.deselect()
    }

    function insertText(value) {
        editor.insert(editor.cursorPosition, String(value))
    }

    function clear() {
        editor.text = ""
    }

    function select(start, end) {
        editor.select(start, end)
    }

    function selectAll() {
        editor.selectAll()
    }

    function deselect() {
        editor.deselect()
    }

    function cut() {
        editor.cut()
    }

    function copy() {
        editor.copy()
    }

    function paste() {
        editor.paste()
    }

    function undo() {
        editor.undo()
    }

    function redo() {
        editor.redo()
    }

    function resolvedRectangleRadius(rectWidth, rectHeight, fallbackRadius) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return fallbackRadius
    }

    Item {
        id: editArea
        anchors.fill: parent
        height: control.resolvedEditorHeight

        Rectangle {
            anchors.fill: parent
            radius: control.resolvedRectangleRadius(width, height, control.cornerRadius)
            color: control.resolvedEditAreaBackgroundColor
        }

        Flickable {
            id: flickable
            objectName: "editorViewportFlickable"
            anchors.fill: parent
            clip: true
            boundsBehavior: control.viewportBoundsBehavior
            boundsMovement: control.viewportBoundsMovement
            flickDeceleration: Math.max(1, control.viewportFlickDeceleration)
            maximumFlickVelocity: Math.max(1, control.viewportMaximumFlickVelocity)
            interactive: control.enabled && (contentHeight > height || contentWidth > width)
            contentWidth: Math.max(width, editor.x + editor.paintedWidth + control.insetHorizontal)
            contentHeight: Math.max(height, editor.y + editor.height + control.insetVertical)

            ScrollBar.vertical: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
            ScrollBar.horizontal: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            TextEdit {
                id: editor
                objectName: "textEditorRichTextEdit"
                x: control.insetHorizontal
                y: control.insetVertical
                width: Math.max(1, flickable.width - control.insetHorizontal * 2)
                height: Math.max(control.textLineBoxHeight,
                                 Math.ceil(contentHeight),
                                 flickable.height - y - control.insetVertical)
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                color: control.enabled ? control.textColor : control.textColorDisabled
                selectionColor: control.selectionColor
                selectedTextColor: control.selectedTextColor
                font.family: control.fontFamily
                font.pixelSize: control.fontPixelSize
                font.weight: control.fontWeight
                font.styleName: control.fontStyleName
                font.letterSpacing: control.fontLetterSpacing
                font.preferShaping: true
                renderType: control.preferNativeTextInteraction
                    ? TextEdit.NativeRendering
                    : TextEdit.QtRendering
                activeFocusOnPress: control.autoFocusOnPress
                cursorVisible: control.enabled && activeFocus && !readOnly
                selectByMouse: true
                persistentSelection: true
                activeFocusOnTab: true

                onTextChanged: {
                    control.textEdited(text)
                    if (privateState.syncingEditorFromDocument || control.reading)
                        return
                    if (editor.inputMethodComposing) {
                        privateState.composingDocumentEditPending = true
                        return
                    }
                    privateState.syncDocumentFromEditor(text)
                    privateState.publishDocumentEdit(text)
                }

                onInputMethodComposingChanged: {
                    if (!inputMethodComposing)
                        Qt.callLater(privateState.publishPendingComposedDocumentEdit)
                }
            }
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: control.insetHorizontal
            anchors.rightMargin: control.insetHorizontal
            anchors.topMargin: control.insetVertical
            text: control.placeholderText
            color: control.enabled ? control.placeholderColor : control.placeholderColorDisabled
            opacity: control.placeholderOpacity
            visible: control.empty && control.placeholderText.length > 0
            font.family: control.fontFamily
            font.pixelSize: control.fontPixelSize
            font.weight: control.fontWeight
            font.styleName: control.fontStyleName
            font.letterSpacing: control.fontLetterSpacing
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            renderType: Text.QtRendering
        }

        InputMethodGuard {
            target: editor
            guardEnabled: control.enabled && !control.readOnly
        }

        WheelScrollGuard {
            enabled: !control.preferNativeGestures
            anchors.fill: parent
            targetFlickable: flickable
            consumeInside: true
        }

        HoverHandler {
            id: hoverHandler
            enabled: control.enabled && !control.preferNativeGestures
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TextEditor { filePath: "/tmp/notes.html"; text: "<h1>Notes</h1><p><b>Rich</b> text</p>" }

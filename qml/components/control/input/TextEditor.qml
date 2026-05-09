import QtQuick
import QtQuick.Controls
import LVRS 1.0

FocusScope {
    id: control

    required property string filePath

    property alias chunkSize: document.loadChunkSize
    property string placeholderText: ""
    property bool readOnly: false

    readonly property alias dirty: document.dirty
    readonly property alias reading: document.loading
    readonly property alias bytesRead: document.loadedByteCount
    readonly property alias bytesTotal: document.totalByteCount
    readonly property alias progress: document.loadProgress
    readonly property alias error: document.lastError

    property int fieldMinHeight: Theme.controlHeightMd * 3
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
    property int viewportFlickDeceleration: Theme.mobileTarget ? 1800 : 3200
    property int viewportMaximumFlickVelocity: Theme.mobileTarget ? 12000 : 8000

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
    property color backgroundColor: Theme.subSurface
    property color backgroundColorHover: Theme.surfaceAlt
    property color backgroundColorPressed: Theme.accentBlueMuted
    property color backgroundColorFocused: backgroundColor
    property color backgroundColorDisabled: backgroundColor

    readonly property int textLineBoxHeight: Math.max(1, centeredTextHeight)
    readonly property int lineHeight: Math.max(textLineBoxHeight, Math.ceil(fontPixelSize * textLineHeight))
    readonly property int resolvedEditorHeight: Math.max(fieldMinHeight, editorHeight)
    readonly property bool focused: activeFocus
    readonly property bool hovered: control.enabled && hoverHandler.hovered
    readonly property bool pressed: tapHandler.pressed
    readonly property bool empty: document.characterCount === 0
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

    implicitWidth: Math.max(Theme.inputMinWidth, lineView.contentWidth + insetHorizontal * 2)
    implicitHeight: control.resolvedEditorHeight
    activeFocusOnTab: true

    onFilePathChanged: {
        if (privateState.completed && filePath.trim().length > 0) {
            syncTimer.stop()
            let canRead = true
            if (document.dirty && document.hasFilePath && !document.loading && !control.readOnly) {
                canRead = document.saveFile(document.filePath)
            }
            if (canRead)
                read()
        }
    }

    Component.onCompleted: {
        privateState.completed = true
        if (filePath.trim().length > 0)
            read()
    }

    onActiveFocusChanged: {
        if (activeFocus && inputAdapter.enabled && !inputAdapter.activeFocus)
            inputAdapter.forceActiveFocus()
    }

    QtObject {
        id: privateState
        property bool completed: false
        property bool adapterClearing: false
        property int selectionAnchorPosition: -1
        property bool mouseSelectionMoved: false

        function hasSelection() {
            return selectionAnchorPosition >= 0
                && selectionAnchorPosition !== document.cursorPosition
        }

        function selectionStartPosition() {
            return hasSelection()
                ? Math.min(selectionAnchorPosition, document.cursorPosition)
                : document.cursorPosition
        }

        function selectionEndPosition() {
            return hasSelection()
                ? Math.max(selectionAnchorPosition, document.cursorPosition)
                : document.cursorPosition
        }

        function clearSelection() {
            selectionAnchorPosition = -1
        }

        function replaceSelectionOrInsert(value) {
            if (value.length === 0)
                return
            if (hasSelection()) {
                document.replaceRange(selectionStartPosition(), selectionEndPosition(), value)
                clearSelection()
            } else {
                document.insertText(value)
            }
        }

        function removePrevious() {
            if (hasSelection()) {
                document.replaceRange(selectionStartPosition(), selectionEndPosition(), "")
                clearSelection()
                return true
            }
            return document.removePreviousCharacter()
        }

        function removeNext() {
            if (hasSelection()) {
                document.replaceRange(selectionStartPosition(), selectionEndPosition(), "")
                clearSelection()
                return true
            }
            return document.removeNextCharacter()
        }

        function moveCursorWithSelection(event, moveFunction) {
            const extend = (event.modifiers & Qt.ShiftModifier) !== 0
            const anchor = document.cursorPosition
            if (extend && selectionAnchorPosition < 0)
                selectionAnchorPosition = anchor
            moveFunction()
            if (!extend)
                clearSelection()
            event.accepted = true
        }

        function columnAtX(line, x) {
            const text = document.lineText(line)
            const targetX = Math.max(0, x)
            let low = 0
            let high = text.length
            while (low < high) {
                const mid = Math.ceil((low + high) / 2)
                hitMetrics.text = text.slice(0, mid)
                if (hitMetrics.advanceWidth <= targetX)
                    low = mid
                else
                    high = mid - 1
            }
            return Math.max(0, Math.min(text.length, low))
        }

        function documentPositionAt(x, y) {
            const line = Math.max(0, Math.min(
                document.lineCount - 1,
                Math.floor((y - lineView.y + lineView.contentY) / control.lineHeight)
            ))
            const column = columnAtX(line, x - lineView.x)
            return document.positionForLineColumn(line, column)
        }

        function handleKey(event) {
            if (!control.enabled || control.readOnly || control.reading) {
                event.accepted = false
                return
            }

            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                replaceSelectionOrInsert("\n")
                event.accepted = true
                return
            }

            if (event.key === Qt.Key_Backspace) {
                event.accepted = removePrevious()
                return
            }

            if (event.key === Qt.Key_Delete) {
                event.accepted = removeNext()
                return
            }

            if (event.key === Qt.Key_Left) {
                moveCursorWithSelection(event, function() { document.moveCursorLeft() })
                return
            }

            if (event.key === Qt.Key_Right) {
                moveCursorWithSelection(event, function() { document.moveCursorRight() })
                return
            }

            if (event.key === Qt.Key_Up) {
                moveCursorWithSelection(event, function() { document.moveCursorUp() })
                return
            }

            if (event.key === Qt.Key_Down) {
                moveCursorWithSelection(event, function() { document.moveCursorDown() })
                return
            }

            if (event.key === Qt.Key_Home) {
                moveCursorWithSelection(event, function() { document.moveCursorLineStart() })
                return
            }

            if (event.key === Qt.Key_End) {
                moveCursorWithSelection(event, function() { document.moveCursorLineEnd() })
                return
            }

            if (event.key === Qt.Key_Tab) {
                replaceSelectionOrInsert("    ")
                event.accepted = true
                return
            }

            event.accepted = false
        }
    }

    TextMetrics {
        id: hitMetrics
        font.family: control.fontFamily
        font.pixelSize: control.fontPixelSize
        font.weight: control.fontWeight
        font.styleName: control.fontStyleName
        font.letterSpacing: control.fontLetterSpacing
    }

    TextMetrics {
        id: adapterCursorMetrics
        font.family: control.fontFamily
        font.pixelSize: control.fontPixelSize
        font.weight: control.fontWeight
        font.styleName: control.fontStyleName
        font.letterSpacing: control.fontLetterSpacing
        text: document.lineText(document.cursorLine).slice(0, document.cursorColumn)
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
            if (!control.reading && !control.readOnly)
                syncTimer.restart()
        }

        function onCursorChanged() {
            lineView.positionViewAtIndex(document.cursorLine, ListView.Contain)
        }

        function onFileLoaded(path, length) {
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

    function resolvedRectangleRadius(rectWidth, rectHeight, fallbackRadius) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return fallbackRadius
    }

    Keys.onPressed: function(event) {
        privateState.handleKey(event)
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

        TextInput {
            id: inputAdapter
            objectName: "editorImeAdapter"
            x: lineView.x + adapterCursorMetrics.advanceWidth
            y: lineView.y + document.cursorLine * control.lineHeight - lineView.contentY
            width: 1
            height: control.lineHeight
            opacity: 0.01
            enabled: control.enabled && !control.readOnly
            focus: control.activeFocus
            activeFocusOnPress: false
            color: "transparent"
            selectedTextColor: "transparent"
            selectionColor: "transparent"
            cursorVisible: false
            clip: true
            inputMethodHints: Qt.ImhMultiLine
            Keys.priority: Keys.BeforeItem
            Keys.onPressed: function(event) {
                privateState.handleKey(event)
            }
            onTextChanged: {
                if (privateState.adapterClearing)
                    return
                if (text.length === 0)
                    return
                const committedText = text
                privateState.adapterClearing = true
                text = ""
                privateState.adapterClearing = false
                privateState.replaceSelectionOrInsert(committedText)
            }
        }

        ListView {
            id: lineView
            objectName: "editorViewportFlickable"
            anchors.fill: parent
            anchors.margins: control.insetVertical
            anchors.leftMargin: control.insetHorizontal
            anchors.rightMargin: control.insetHorizontal
            clip: true
            enabled: control.enabled
            model: document
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            flickDeceleration: Math.max(1, control.viewportFlickDeceleration)
            maximumFlickVelocity: Math.max(1, control.viewportMaximumFlickVelocity)
            interactive: contentHeight > height
                && (!control.preferNativeGestures || !control.focused)
            spacing: 0
            reuseItems: true
            currentIndex: document.cursorLine

            ScrollBar.vertical: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AlwaysOff
            }

            delegate: Item {
                id: lineDelegate
                required property int lineIndex
                required property string text

                width: ListView.view ? ListView.view.width : 0
                height: control.lineHeight
                readonly property int documentLineStart: document.positionForLineColumn(lineIndex, 0)
                readonly property int selectedStartColumn: privateState.hasSelection()
                    ? Math.max(0, Math.min(text.length, privateState.selectionStartPosition() - documentLineStart))
                    : 0
                readonly property int selectedEndColumn: privateState.hasSelection()
                    ? Math.max(0, Math.min(text.length, privateState.selectionEndPosition() - documentLineStart))
                    : 0
                readonly property bool lineSelected: selectedEndColumn > selectedStartColumn

                TextMetrics {
                    id: selectionPrefixMetrics
                    font.family: control.fontFamily
                    font.pixelSize: control.fontPixelSize
                    font.weight: control.fontWeight
                    font.styleName: control.fontStyleName
                    font.letterSpacing: control.fontLetterSpacing
                    text: lineDelegate.text.slice(0, lineDelegate.selectedStartColumn)
                }

                TextMetrics {
                    id: selectionTextMetrics
                    font.family: control.fontFamily
                    font.pixelSize: control.fontPixelSize
                    font.weight: control.fontWeight
                    font.styleName: control.fontStyleName
                    font.letterSpacing: control.fontLetterSpacing
                    text: lineDelegate.text.slice(lineDelegate.selectedStartColumn, lineDelegate.selectedEndColumn)
                }

                Rectangle {
                    x: selectionPrefixMetrics.advanceWidth
                    y: Math.max(0, Math.floor((parent.height - control.textLineBoxHeight) / 2))
                    width: Math.max(Theme.scaleMetric(2), selectionTextMetrics.advanceWidth)
                    height: control.textLineBoxHeight
                    color: Theme.accentBlueMuted
                    visible: lineDelegate.lineSelected
                }

                Text {
                    id: lineText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: parent.text.length > 0 ? parent.text : " "
                    color: control.enabled ? control.textColor : control.textColorDisabled
                    font.family: control.fontFamily
                    font.pixelSize: control.fontPixelSize
                    font.weight: control.fontWeight
                    font.styleName: control.fontStyleName
                    font.letterSpacing: control.fontLetterSpacing
                    textFormat: Text.PlainText
                    wrapMode: Text.NoWrap
                    renderType: Text.QtRendering
                }

                TextMetrics {
                    id: cursorMetrics
                    font: lineText.font
                    text: lineDelegate.text.slice(0, document.cursorColumn)
                }

                Text {
                    x: cursorMetrics.advanceWidth
                    anchors.verticalCenter: parent.verticalCenter
                    text: inputAdapter.preeditText
                    color: control.enabled ? control.textColor : control.textColorDisabled
                    font: lineText.font
                    textFormat: Text.PlainText
                    renderType: Text.QtRendering
                    visible: control.focused
                        && document.cursorLine === lineDelegate.lineIndex
                        && inputAdapter.preeditText.length > 0
                }

                Rectangle {
                    id: cursorVisual
                    objectName: "editorCursor"
                    x: Math.min(lineDelegate.width - width, cursorMetrics.advanceWidth)
                    y: Math.max(0, Math.floor((lineDelegate.height - height) / 2))
                    width: Math.max(1, Theme.scaleMetric(1))
                    height: control.textLineBoxHeight
                    color: control.textColor
                    visible: control.focused
                        && !control.readOnly
                        && document.cursorLine === lineDelegate.lineIndex
                }
            }
        }

        Text {
            anchors.left: lineView.left
            anchors.right: lineView.right
            anchors.top: lineView.top
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
        }

        MouseArea {
            anchors.fill: parent
            z: 10
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.IBeamCursor
            preventStealing: true
            enabled: control.enabled
            onPressed: function(mouse) {
                if (control.autoFocusOnPress)
                    inputAdapter.forceActiveFocus()

                const previousPosition = document.cursorPosition
                const nextPosition = privateState.documentPositionAt(mouse.x, mouse.y)
                if ((mouse.modifiers & Qt.ShiftModifier) && privateState.selectionAnchorPosition < 0)
                    privateState.selectionAnchorPosition = previousPosition
                else if (!(mouse.modifiers & Qt.ShiftModifier))
                    privateState.selectionAnchorPosition = nextPosition

                document.cursorPosition = nextPosition
                privateState.mouseSelectionMoved = false
                mouse.accepted = true
            }
            onPositionChanged: function(mouse) {
                if (!pressed)
                    return
                privateState.mouseSelectionMoved = true
                document.cursorPosition = privateState.documentPositionAt(mouse.x, mouse.y)
                mouse.accepted = true
            }
            onReleased: function(mouse) {
                if (!privateState.mouseSelectionMoved && !(mouse.modifiers & Qt.ShiftModifier))
                    privateState.clearSelection()
                mouse.accepted = true
            }
        }

        TapHandler {
            id: tapHandler
            enabled: control.enabled
            onTapped: {
                if (control.autoFocusOnPress)
                    inputAdapter.forceActiveFocus()
            }
        }

        WheelScrollGuard {
            enabled: !control.preferNativeGestures
            anchors.fill: parent
            targetFlickable: lineView
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
// LV.TextEditor { filePath: "/tmp/notes.txt" }

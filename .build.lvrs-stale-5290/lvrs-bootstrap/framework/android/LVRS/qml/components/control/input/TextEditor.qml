import QtQuick
import QtQuick.Controls
import LVRS 1.0

FocusScope {
    id: control

    readonly property int plainTextMode: 0
    readonly property int markdownMode: 1
    readonly property int richTextMode: 2

    property int mode: plainTextMode
    property bool enforceModeDefaults: true

    property alias text: editor.text
    property string placeholderText: ""
    property alias readOnly: editor.readOnly
    property alias cursorPosition: editor.cursorPosition
    property alias selectionStart: editor.selectionStart
    property alias selectionEnd: editor.selectionEnd
    property alias length: editor.length
    property alias selectByMouse: editor.selectByMouse
    property alias persistentSelection: editor.persistentSelection
    property alias overwriteMode: editor.overwriteMode
    property alias inputMethodHints: editor.inputMethodHints
    property alias tabStopDistance: editor.tabStopDistance
    property alias baseUrl: editor.baseUrl

    property int wrapMode: TextEdit.Wrap
    property int textFormat: TextEdit.PlainText
    property int fieldMinHeight: Theme.controlHeightMd * 3
    property int editorHeight: fieldMinHeight
    property int insetHorizontal: Theme.gap10
    property int insetVertical: Theme.gap8
    property int cornerRadius: Theme.radiusMd
    property bool showScrollBar: true
    property bool autoFocusOnPress: true
    property bool showRenderedOutput: true
    property int outputSpacing: Theme.gap8
    property int outputMinHeight: Theme.controlHeightMd * 2
    property color outputBackgroundColor: Theme.surfaceSolid
    property color outputTextColor: Theme.textPrimary

    property string fontFamily: Theme.fontBody
    property int fontPixelSize: Theme.textBody
    property int fontWeight: Theme.textBodyWeight
    property string fontStyleName: Theme.textBodyStyleName
    property real fontLetterSpacing: Theme.textBodyLetterSpacing
    property real textLineHeight: Theme.textBodyLineHeight

    property color textColor: Theme.textPrimary
    property color textColorDisabled: Theme.textOctonary
    property color placeholderColor: Theme.textTertiary
    property color placeholderColorDisabled: Theme.textOctonary
    property real placeholderOpacity: 1.0
    property color selectionColor: Theme.accent
    property color selectedTextColor: Theme.textPrimary
    property color backgroundColor: Theme.subSurface
    property color backgroundColorFocused: backgroundColor
    property color backgroundColorDisabled: backgroundColor

    readonly property int resolvedWrapMode: TextEdit.Wrap
    readonly property int resolvedTextFormat: TextEdit.PlainText
    readonly property int effectiveWrapMode: enforceModeDefaults ? resolvedWrapMode : wrapMode
    readonly property int effectiveTextFormat: enforceModeDefaults ? resolvedTextFormat : textFormat

    readonly property string normalizedInput: TextMarkup.normalize(editor.text)
    readonly property string renderedOutput: TextMarkup.renderHtml(editor.text)
    readonly property string renderedPlainText: TextMarkup.renderPlainText(editor.text)
    readonly property bool previewVisible: showRenderedOutput
    readonly property int previewHeight: previewVisible
        ? Math.max(outputMinHeight, previewText.implicitHeight + insetVertical * 2)
        : 0
    readonly property int resolvedEditorHeight: Math.max(fieldMinHeight, editorHeight)
    readonly property bool focused: activeFocus || editor.activeFocus
    readonly property bool empty: editor.text.length === 0 && editor.preeditText.length === 0
    readonly property bool canUndo: editor.canUndo
    readonly property bool canRedo: editor.canRedo

    signal textEdited(string text)
    signal submitted(string text)

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

    function undo() {
        editor.undo()
    }

    function redo() {
        editor.redo()
    }

    function submit() {
        control.submitted(editor.text)
    }

    implicitWidth: Math.max(
                       Theme.inputMinWidth,
                       editor.implicitWidth + control.insetHorizontal * 2
                   )
    implicitHeight: control.resolvedEditorHeight
                    + (previewVisible ? outputSpacing + previewHeight : 0)
    activeFocusOnTab: true

    Item {
        id: editArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: control.resolvedEditorHeight

        Rectangle {
            anchors.fill: parent
            radius: control.cornerRadius
            color: control.backgroundColor
        }

        Flickable {
            id: flickable
            anchors.fill: parent
            clip: true
            interactive: contentHeight > height || contentWidth > width
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(width, editor.x + editor.paintedWidth + control.insetHorizontal)
            contentHeight: Math.max(height, editor.y + editor.paintedHeight + control.insetVertical)

            ScrollBar.vertical: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
            ScrollBar.horizontal: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            TextEdit {
                id: editor
                objectName: "editorTextEdit"
                x: control.insetHorizontal
                y: control.insetVertical
                width: Math.max(1, flickable.width - control.insetHorizontal * 2)
                height: paintedHeight + 2
                wrapMode: control.effectiveWrapMode
                textFormat: control.effectiveTextFormat
                color: control.enabled ? control.textColor : control.textColorDisabled
                selectionColor: control.selectionColor
                selectedTextColor: control.selectedTextColor
                font.family: control.fontFamily
                font.pixelSize: control.fontPixelSize
                font.weight: control.fontWeight
                font.styleName: control.fontStyleName
                font.letterSpacing: control.fontLetterSpacing
                font.preferShaping: true
                cursorVisible: control.enabled && activeFocus && !readOnly
                selectByMouse: true
                persistentSelection: true
                activeFocusOnTab: true

                onTextChanged: control.textEdited(text)

                Keys.onReturnPressed: function(event) {
                    if ((event.modifiers & Qt.ControlModifier) || (event.modifiers & Qt.MetaModifier)) {
                        control.submitted(text)
                        event.accepted = true
                    } else {
                        event.accepted = false
                    }
                }
                Keys.onEnterPressed: function(event) {
                    if ((event.modifiers & Qt.ControlModifier) || (event.modifiers & Qt.MetaModifier)) {
                        control.submitted(text)
                        event.accepted = true
                    } else {
                        event.accepted = false
                    }
                }
            }
        }

        InputMethodGuard {
            target: editor
            guardEnabled: control.enabled && !control.readOnly
        }

        WheelScrollGuard {
            anchors.fill: parent
            targetFlickable: flickable
            consumeInside: true
        }

        Label {
            style: body
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: control.insetHorizontal
            anchors.rightMargin: control.insetHorizontal
            anchors.topMargin: control.insetVertical
            text: control.placeholderText
            color: control.enabled ? control.placeholderColor : control.placeholderColorDisabled
            opacity: control.placeholderOpacity
            visible: control.empty
            wrapMode: Text.WordWrap
        }

        MouseArea {
            anchors.fill: parent
            enabled: control.enabled
            acceptedButtons: Qt.LeftButton
            cursorShape: control.enabled ? Qt.IBeamCursor : Qt.ArrowCursor
            onPressed: function(mouse) {
                if (control.autoFocusOnPress)
                    control.forceEditorFocus()
                mouse.accepted = false
            }
        }
    }

    Rectangle {
        id: previewPane
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: editArea.bottom
        anchors.topMargin: control.outputSpacing
        visible: control.previewVisible
        height: control.previewHeight
        radius: control.cornerRadius
        color: control.outputBackgroundColor

        Flickable {
            id: previewFlick
            anchors.fill: parent
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(width, previewText.implicitWidth + control.insetHorizontal * 2)
            contentHeight: Math.max(height, previewText.implicitHeight + control.insetVertical * 2)

            Text {
                id: previewText
                x: control.insetHorizontal
                y: control.insetVertical
                width: Math.max(1, previewFlick.width - control.insetHorizontal * 2)
                text: control.renderedOutput
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                color: control.outputTextColor
                font.family: control.fontFamily
                font.pixelSize: control.fontPixelSize
                font.weight: control.fontWeight
                font.styleName: control.fontStyleName
                font.letterSpacing: control.fontLetterSpacing
            }

            ScrollBar.vertical: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
        }

        WheelScrollGuard {
            anchors.fill: parent
            targetFlickable: previewFlick
            consumeInside: true
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TextEditor { mode: plainTextMode; text: "Hello **bold**"; onSubmitted: save(text) }

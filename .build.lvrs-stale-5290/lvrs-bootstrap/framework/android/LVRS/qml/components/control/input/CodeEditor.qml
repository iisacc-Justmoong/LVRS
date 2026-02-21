import QtQuick
import QtQuick.Controls
import LVRS 1.0

FocusScope {
    id: control

    property alias text: editor.text
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

    readonly property int wrapMode: editor.wrapMode
    readonly property int textFormat: editor.textFormat
    property string placeholderText: ""
    property string snippetTitle: "Snippet"
    property string snippetLanguage: ""
    property bool showSnippetHeader: true
    property bool showScrollBar: true
    property bool autoFocusOnPress: true

    property int fieldMinHeight: Theme.controlHeightMd * 4
    property int editorHeight: fieldMinHeight
    property int headerHeight: Theme.controlHeightSm
    property int insetHorizontal: Theme.gap10
    property int insetVertical: Theme.gap8
    property int headerSpacing: Theme.gap4
    property int cornerRadius: Theme.radiusMd

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
    property color headerTextColor: Theme.textTertiary

    property string fontFamily: Qt.platform.os === "osx" ? "Menlo" : "Monospace"
    property int fontPixelSize: Theme.textBody
    property int fontWeight: Theme.textBodyWeight
    property string fontStyleName: Theme.textBodyStyleName
    property real fontLetterSpacing: Theme.textBodyLetterSpacing

    readonly property bool focused: activeFocus || editor.activeFocus
    readonly property bool empty: editor.text.length === 0 && editor.preeditText.length === 0
    readonly property int headerBlockHeight: showSnippetHeader ? (headerHeight + headerSpacing) : 0
    readonly property int topInset: insetVertical + headerBlockHeight
    readonly property int resolvedEditorHeight: Math.max(fieldMinHeight, editorHeight)

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
                       editor.implicitWidth + insetHorizontal * 2
                   )
    implicitHeight: resolvedEditorHeight
    activeFocusOnTab: true

    Rectangle {
        anchors.fill: parent
        radius: control.cornerRadius
        color: control.backgroundColor

        Label {
            id: snippetMeta
            style: caption
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: control.insetHorizontal
            anchors.rightMargin: control.insetHorizontal
            anchors.topMargin: control.insetVertical
            visible: control.showSnippetHeader
            color: control.headerTextColor
            text: control.snippetLanguage.length > 0
                ? control.snippetTitle + " - " + control.snippetLanguage
                : control.snippetTitle
            elide: Text.ElideRight
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
                objectName: "codeTextEdit"
                x: control.insetHorizontal
                y: control.topInset
                width: Math.max(1, flickable.width - control.insetHorizontal * 2)
                height: paintedHeight + 2
                wrapMode: TextEdit.NoWrap
                textFormat: TextEdit.PlainText
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
            style: description
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: control.insetHorizontal
            anchors.rightMargin: control.insetHorizontal
            anchors.topMargin: control.topInset
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
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.CodeEditor { snippetTitle: "init.ts"; snippetLanguage: "TypeScript"; text: "const ready = true" }

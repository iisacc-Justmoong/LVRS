import QtQuick
import QtQuick.Controls
import LVRS 1.0

FocusScope {
    id: control

    // Expose the underlying TextEdit for full platform-native property access.
    readonly property alias editorItem: editor
    readonly property int plainTextMode: 0
    readonly property int markdownMode: 1
    readonly property int richTextMode: 2

    property int mode: plainTextMode
    property bool enforceModeDefaults: true

    property alias text: editor.text
    property string placeholderText: ""
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
    // Compatibility flag: native TextEdit keyboard selection is OS-managed.
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

    property int wrapMode: TextEdit.Wrap
    property int textFormat: TextEdit.PlainText
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
    readonly property bool preferNativeTextInteraction: preferNativeGestures
        && Theme.effectiveRuntimeProfile.ios === true
    property int viewportFlickDeceleration: Theme.mobileTarget ? 1800 : 3200
    property int viewportMaximumFlickVelocity: Theme.mobileTarget ? 12000 : 8000
    readonly property int viewportBoundsBehavior: Flickable.StopAtBounds
    readonly property int viewportBoundsMovement: Flickable.StopAtBounds
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

    readonly property int resolvedWrapMode: TextEdit.Wrap
    readonly property int resolvedTextFormat: TextEdit.PlainText
    readonly property int effectiveWrapMode: enforceModeDefaults ? resolvedWrapMode : wrapMode
    readonly property int effectiveTextFormat: enforceModeDefaults ? resolvedTextFormat : textFormat
    readonly property int textLineBoxHeight: Math.max(1, centeredTextHeight)

    readonly property string normalizedInput: TextMarkup.normalize(editor.text)
    readonly property string renderedOutput: TextMarkup.renderHtml(editor.text)
    readonly property string renderedPlainText: TextMarkup.renderPlainText(editor.text)
    readonly property bool previewVisible: showRenderedOutput
    readonly property int previewHeight: previewVisible
        ? Math.max(outputMinHeight, previewText.implicitHeight + insetVertical * 2)
        : 0
    readonly property int resolvedEditorHeight: Math.max(fieldMinHeight, editorHeight)
    readonly property int centeredTextY: Math.max(
        insetVertical,
        Math.floor((resolvedEditorHeight - textLineBoxHeight) / 2)
    )
    readonly property bool focused: activeFocus || editor.activeFocus
    readonly property bool hovered: interactionArea.enabled && interactionArea.containsMouse
    readonly property bool pressed: interactionArea.enabled && interactionArea.pressed
    readonly property bool empty: editor.text.length === 0 && editor.preeditText.length === 0
    readonly property bool canUndo: editor.canUndo
    readonly property bool canRedo: editor.canRedo
    readonly property color resolvedEditAreaBackgroundColor: !control.enabled
        ? control.backgroundColorDisabled
        : control.pressed
            ? control.backgroundColorPressed
            : control.focused
                ? control.backgroundColorFocused
                : control.hovered
                    ? control.backgroundColorHover
                    : control.backgroundColor

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

    function submit() {
        control.submitted(editor.text)
    }

    function resolvedRectangleRadius(rectWidth, rectHeight, fallbackRadius) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return fallbackRadius
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
            radius: control.resolvedRectangleRadius(width, height, control.cornerRadius)
            color: control.resolvedEditAreaBackgroundColor
        }

        Flickable {
            id: editorFlick
            objectName: "editorViewportFlickable"
            anchors.fill: parent
            clip: true
            enabled: control.enabled
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: control.viewportBoundsBehavior
            boundsMovement: control.viewportBoundsMovement
            flickDeceleration: Math.max(1, control.viewportFlickDeceleration)
            maximumFlickVelocity: Math.max(1, control.viewportMaximumFlickVelocity)
            interactive: contentHeight > height && (!control.preferNativeGestures || !editor.activeFocus)

            ScrollBar.vertical: ScrollBar {
                policy: control.showScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AlwaysOff
            }

            TextArea.flickable: TextArea {
                id: editor
                objectName: "editorTextEdit"
                width: Math.max(1, editorFlick.width)
                wrapMode: control.effectiveWrapMode
                textFormat: control.effectiveTextFormat
                placeholderText: control.placeholderText
                placeholderTextColor: control.enabled ? control.placeholderColor : control.placeholderColorDisabled
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
                activeFocusOnPress: true
                cursorVisible: control.enabled && activeFocus && !readOnly
                leftPadding: control.insetHorizontal
                rightPadding: control.insetHorizontal
                topPadding: control.centeredTextY
                bottomPadding: control.insetVertical
                selectByMouse: true
                persistentSelection: true
                activeFocusOnTab: true
                background: Item {
                    implicitWidth: 0
                    implicitHeight: 0
                }

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

        MouseArea {
            id: interactionArea
            anchors.fill: parent
            enabled: control.enabled && !control.preferNativeGestures
            acceptedButtons: Qt.LeftButton
            hoverEnabled: enabled
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
        radius: control.resolvedRectangleRadius(width, height, control.cornerRadius)
        color: control.outputBackgroundColor

        Flickable {
            id: previewFlick
            objectName: "editorPreviewFlickable"
            anchors.fill: parent
            clip: true
            interactive: contentHeight > height
            boundsBehavior: control.viewportBoundsBehavior
            boundsMovement: control.viewportBoundsMovement
            flickDeceleration: Math.max(1, control.viewportFlickDeceleration)
            maximumFlickVelocity: Math.max(1, control.viewportMaximumFlickVelocity)
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
            enabled: !control.preferNativeGestures
            anchors.fill: parent
            targetFlickable: previewFlick
            consumeInside: true
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TextEditor { mode: plainTextMode; text: "Hello **bold**"; onSubmitted: save(text) }

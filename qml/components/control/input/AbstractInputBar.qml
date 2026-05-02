import QtQuick
import LVRS 1.0

FocusScope {
    id: control

    // Expose the underlying TextInput for full platform-native property access.
    readonly property alias inputItem: inputField
    property alias text: inputField.text
    property alias displayText: inputField.displayText
    property alias preeditText: inputField.preeditText
    property alias length: inputField.length
    property alias placeholderText: placeholderLabel.text
    property alias readOnly: inputField.readOnly
    property alias echoMode: inputField.echoMode
    property alias validator: inputField.validator
    property alias inputMask: inputField.inputMask
    property alias maximumLength: inputField.maximumLength
    property alias acceptableInput: inputField.acceptableInput
    property alias inputMethodHints: inputField.inputMethodHints
    property alias inputMethodComposing: inputField.inputMethodComposing
    property alias activeFocusOnPress: inputField.activeFocusOnPress
    property alias selectByMouse: inputField.selectByMouse
    // TextInput does not expose selectByKeyboard as a writable QML property.
    // Keep this compatibility flag so external callers can set/read it without
    // breaking, while native keyboard selection remains OS-managed.
    property bool selectByKeyboard: true
    property alias mouseSelectionMode: inputField.mouseSelectionMode
    property alias persistentSelection: inputField.persistentSelection
    property alias cursorPosition: inputField.cursorPosition
    property alias cursorRectangle: inputField.cursorRectangle
    property alias selectionStart: inputField.selectionStart
    property alias selectionEnd: inputField.selectionEnd
    property alias selectedText: inputField.selectedText
    property alias autoScroll: inputField.autoScroll
    property alias overwriteMode: inputField.overwriteMode
    property alias canPaste: inputField.canPaste
    property alias canUndo: inputField.canUndo
    property alias canRedo: inputField.canRedo
    property alias cursorDelegate: inputField.cursorDelegate
    property alias renderType: inputField.renderType
    property alias passwordCharacter: inputField.passwordCharacter
    property alias passwordMaskDelay: inputField.passwordMaskDelay
    property alias horizontalAlignment: inputField.horizontalAlignment
    property alias verticalAlignment: inputField.verticalAlignment
    property alias wrapMode: inputField.wrapMode
    property alias contentWidth: inputField.contentWidth
    property alias contentHeight: inputField.contentHeight

    property int fieldMinHeight: Theme.controlHeightMd
    property int insetHorizontal: Theme.gap12
    property int insetVertical: Theme.gap8
    property int sideSpacing: Theme.gap8
    property int centeredTextHeight: Theme.scaleTextMetric(16)
    property bool preferNativeGestures: Theme.mobileTarget
    readonly property bool preferNativeTextInteraction: preferNativeGestures
        && Theme.effectiveRuntimeProfile.ios === true

    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int cornerRadius: Theme.radiusMd

    property color textColor: Theme.textPrimary
    property color textColorDisabled: Theme.textTertiary
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

    property alias leadingItems: leadingCustomSlot.data
    property alias trailingItems: trailingCustomSlot.data
    property alias leadingInternalItems: leadingInternalSlot.data
    property alias trailingInternalItems: trailingInternalSlot.data

    readonly property real leadingWidth: leadingHost.visible ? leadingContent.width : 0
    readonly property real trailingWidth: trailingHost.visible ? trailingContent.width : 0
    readonly property int leftInset: insetHorizontal + leadingWidth + (leadingWidth > 0 ? sideSpacing : 0)
    readonly property int rightInset: insetHorizontal + trailingWidth + (trailingWidth > 0 ? sideSpacing : 0)
    readonly property bool focused: activeFocus || inputField.activeFocus
    readonly property bool hovered: control.enabled && hoverHandler.hovered
    readonly property bool pressed: false
    readonly property int textLineBoxHeight: Math.max(1, centeredTextHeight)
    readonly property int centeredTextY: Math.max(0, Math.floor((height - textLineBoxHeight) / 2))
    readonly property int contentBoxHeight: textLineBoxHeight + insetVertical * 2
    readonly property color resolvedBackgroundColor: !control.enabled
        ? control.backgroundColorDisabled
        : control.pressed
            ? control.backgroundColorPressed
            : control.focused
                ? control.backgroundColorFocused
                : control.hovered
                    ? control.backgroundColorHover
                    : control.backgroundColor
    readonly property real resolvedCornerRadius: shapeStyle === shapeCylinder
        ? Math.max(0, Math.min(width, height) / 2)
        : cornerRadius

    signal accepted(string text)
    signal textEdited(string text)

    function forceInputFocus() {
        inputField.forceActiveFocus()
    }

    function select(start, end) {
        inputField.select(start, end)
    }

    function selectAll() {
        inputField.selectAll()
    }

    function deselect() {
        inputField.deselect()
    }

    function cut() {
        inputField.cut()
    }

    function copy() {
        inputField.copy()
    }

    function paste() {
        inputField.paste()
    }

    function undo() {
        if (inputField.canUndo)
            inputField.undo()
    }

    function redo() {
        if (inputField.canRedo)
            inputField.redo()
    }

    function remove(start, end) {
        inputField.remove(start, end)
    }

    function insert(position, value) {
        inputField.insert(position, String(value))
    }

    implicitHeight: Math.max(fieldMinHeight, contentBoxHeight)
    implicitWidth: Math.max(Theme.inputMinWidth, inputField.implicitWidth + leftInset + rightInset)
    activeFocusOnTab: true

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: control.resolvedCornerRadius
        color: control.resolvedBackgroundColor

        Item {
            id: leadingHost
            anchors.left: parent.left
            anchors.leftMargin: control.insetHorizontal
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: leadingContent.width
            implicitHeight: leadingContent.height
            visible: leadingContent.width > 0 && leadingContent.height > 0

            Row {
                id: leadingContent
                spacing: 0
                anchors.centerIn: parent
                width: childrenRect.width
                height: childrenRect.height

                Item {
                    id: leadingInternalSlot
                    width: childrenRect.width
                    height: childrenRect.height
                }

                Item {
                    id: leadingCustomSlot
                    width: childrenRect.width
                    height: childrenRect.height
                }
            }
        }

        Item {
            id: trailingHost
            anchors.right: parent.right
            anchors.rightMargin: control.insetHorizontal
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: trailingContent.width
            implicitHeight: trailingContent.height
            visible: trailingContent.width > 0 && trailingContent.height > 0

            Row {
                id: trailingContent
                spacing: 0
                anchors.centerIn: parent
                width: childrenRect.width
                height: childrenRect.height

                Item {
                    id: trailingCustomSlot
                    width: childrenRect.width
                    height: childrenRect.height
                }

                Item {
                    id: trailingInternalSlot
                    width: childrenRect.width
                    height: childrenRect.height
                }
            }
        }
    }

    TextInput {
        id: inputField
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: control.leftInset
        anchors.rightMargin: control.rightInset
        y: control.centeredTextY
        height: control.textLineBoxHeight
        color: control.enabled ? control.textColor : control.textColorDisabled
        selectionColor: control.selectionColor
        selectedTextColor: control.selectedTextColor
        cursorVisible: control.focused && control.enabled && !readOnly
        font.family: Theme.fontBody
        font.pixelSize: Theme.textBody
        font.weight: Theme.textBodyWeight
        font.styleName: Theme.textBodyStyleName
        font.letterSpacing: Theme.textBodyLetterSpacing
        font.preferShaping: true
        verticalAlignment: TextInput.AlignVCenter
        renderType: control.preferNativeTextInteraction
            ? TextInput.NativeRendering
            : TextInput.QtRendering
        activeFocusOnPress: true
        activeFocusOnTab: true
        clip: true
        selectByMouse: true
        onTextEdited: control.textEdited(text)
        Keys.onReturnPressed: control.accepted(text)
        Keys.onEnterPressed: control.accepted(text)
    }

    InputMethodGuard {
        target: inputField
        guardEnabled: control.enabled && !control.readOnly
    }

    Label {
        style: body
        id: placeholderLabel
        anchors.left: inputField.left
        anchors.right: inputField.right
        y: control.centeredTextY
        height: control.textLineBoxHeight
        color: control.enabled ? control.placeholderColor : control.placeholderColorDisabled
        opacity: control.placeholderOpacity
        font.family: inputField.font.family
        font.pixelSize: inputField.font.pixelSize
        font.weight: inputField.font.weight
        elide: Text.ElideRight
        visible: inputField.text.length === 0 && inputField.preeditText.length === 0
        verticalAlignment: Text.AlignVCenter
        lineHeightMode: Text.FixedHeight
        lineHeight: control.textLineBoxHeight
        renderType: Text.QtRendering
    }

    HoverHandler {
        id: hoverHandler
        enabled: control.enabled && !control.preferNativeGestures
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.AbstractInputBar { placeholderText: "Search"; leadingItems: LV.Label { text: "⌕"; style: body } }

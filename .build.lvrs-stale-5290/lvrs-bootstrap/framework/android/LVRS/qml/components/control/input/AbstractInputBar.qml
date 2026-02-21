import QtQuick
import LVRS 1.0

FocusScope {
    id: control

    property alias text: inputField.text
    property alias placeholderText: placeholderLabel.text
    property alias readOnly: inputField.readOnly
    property alias echoMode: inputField.echoMode
    property alias validator: inputField.validator
    property alias maximumLength: inputField.maximumLength
    property alias inputMethodHints: inputField.inputMethodHints
    property alias selectByMouse: inputField.selectByMouse
    property alias cursorPosition: inputField.cursorPosition

    property int fieldMinHeight: Theme.controlHeightMd
    property int insetHorizontal: Theme.gap12
    property int insetVertical: Theme.gap8
    property int sideSpacing: Theme.gap8

    property int cornerRadius: Theme.radiusMd

    property color textColor: Theme.textPrimary
    property color textColorDisabled: Theme.textTertiary
    property color placeholderColor: Theme.textTertiary
    property color placeholderColorDisabled: Theme.textOctonary
    property real placeholderOpacity: 1.0
    property color selectionColor: Theme.accent
    property color selectedTextColor: Theme.textPrimary
    property Component cursorDelegate: null

    property color backgroundColor: Theme.subSurface
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

    signal accepted(string text)
    signal textEdited(string text)

    function forceInputFocus() {
        inputField.forceActiveFocus()
    }

    implicitHeight: Math.max(fieldMinHeight, inputField.implicitHeight + insetVertical * 2)
    implicitWidth: Math.max(Theme.inputMinWidth, inputField.implicitWidth + leftInset + rightInset)
    activeFocusOnTab: true

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: control.cornerRadius
        color: control.backgroundColor

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
        anchors.fill: parent
        anchors.leftMargin: control.leftInset
        anchors.rightMargin: control.rightInset
        anchors.topMargin: control.insetVertical
        anchors.bottomMargin: control.insetVertical
        color: control.enabled ? control.textColor : control.textColorDisabled
        selectionColor: control.selectionColor
        selectedTextColor: control.selectedTextColor
        cursorDelegate: control.cursorDelegate
        cursorVisible: control.focused && control.enabled && !readOnly
        font.family: Theme.fontBody
        font.pixelSize: Theme.textBody
        font.weight: Theme.textBodyWeight
        font.preferShaping: true
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
        anchors.verticalCenter: inputField.verticalCenter
        color: control.enabled ? control.placeholderColor : control.placeholderColorDisabled
        opacity: control.placeholderOpacity
        font.family: inputField.font.family
        font.pixelSize: inputField.font.pixelSize
        font.weight: inputField.font.weight
        elide: Text.ElideRight
        visible: inputField.text.length === 0 && inputField.preeditText.length === 0
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill: parent
        enabled: control.enabled
        acceptedButtons: Qt.LeftButton
        cursorShape: control.enabled ? Qt.IBeamCursor : Qt.ArrowCursor
        onPressed: function(mouse) {
            if (!control.enabled)
                return
            control.forceInputFocus()
            mouse.accepted = false
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.AbstractInputBar { placeholderText: "Search"; leadingItems: LV.Label { text: "⌕"; style: body } }

import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    checkable: true
    text: ""
    tone: AbstractButton.Borderless

    property bool state: checked
    property bool available: enabled

    property int indicatorSize: Theme.controlIndicatorSize
    property int dotSize: Theme.gap8

    property color onColor: Theme.accent
    property color offColor: Theme.textPrimary
    property color onColorDisabled: Theme.panelBackground12
    property color offColorDisabled: Theme.panelBackground12
    property color dotColor: Theme.textPrimary
    property color dotColorDisabled: Theme.textSeptenary

    readonly property color indicatorColor: control.enabled
        ? (control.checked ? control.onColor : control.offColor)
        : (control.checked ? control.onColorDisabled : control.offColorDisabled)
    readonly property color indicatorDotColor: control.enabled ? control.dotColor : control.dotColorDisabled

    onStateChanged: {
        if (checked !== state)
            checked = state
    }

    onCheckedChanged: {
        if (state !== checked)
            state = checked
    }

    onAvailableChanged: {
        if (enabled !== available)
            enabled = available
    }

    onEnabledChanged: {
        if (available !== enabled)
            available = enabled
    }

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.gapNone
    backgroundColor: "transparent"
    backgroundColorHover: "transparent"
    backgroundColorPressed: "transparent"
    backgroundColorDisabled: "transparent"
    implicitHeight: contentItem.implicitHeight
    implicitWidth: contentItem.implicitWidth

    background: Item { }

    contentItem: RowLayout {
        spacing: control.text.length > 0 ? Theme.gap8 : Theme.gapNone
        Layout.alignment: Qt.AlignVCenter

        Rectangle {
            implicitWidth: control.indicatorSize
            implicitHeight: control.indicatorSize
            Layout.preferredWidth: control.indicatorSize
            Layout.preferredHeight: control.indicatorSize
            radius: width / 2
            color: control.indicatorColor
            antialiasing: true

            Rectangle {
                width: control.dotSize
                height: control.dotSize
                radius: width / 2
                color: control.indicatorDotColor
                anchors.centerIn: parent
                visible: control.checked
                antialiasing: true
            }
        }

        Label {
            style: body
            text: control.text
            color: control.enabled ? Theme.textPrimary : Theme.textOctonary
            visible: control.text.length > 0
            Layout.alignment: Qt.AlignVCenter
            elide: Text.ElideRight
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.RadioButton { text: "Choice A"; checked: true }

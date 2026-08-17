import QtQuick
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
    readonly property real indicatorRadius: indicatorSize / 2
    readonly property real dotRadius: dotSize / 2

    property color onColor: Theme.accent
    property color offColor: Theme.textPrimary
    property color onColorDisabled: Theme.panelBackground12
    property color offColorDisabled: Theme.panelBackground12
    property color dotColor: Theme.textPrimary
    property color dotColorDisabled: Theme.textSeptenary

    readonly property color indicatorColor: control.checked
        ? (control.enabled ? control.onColor : control.onColorDisabled)
        : (control.enabled ? control.offColor : control.offColorDisabled)
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

    contentItem: Item {
        id: contentLayout

        property real spacing: control.text.length > 0 ? Theme.gap8 : Theme.gapNone
        readonly property real labelWidth: label.visible ? Math.ceil(label.implicitWidth) : 0

        implicitWidth: control.indicatorSize
            + (label.visible ? spacing + labelWidth : 0)
        implicitHeight: Math.max(control.indicatorSize, label.visible ? label.height : 0)

        Rectangle {
            id: indicator
            objectName: control.objectName.length > 0 ? control.objectName + "_indicator" : ""
            x: 0
            y: (contentLayout.height - height) / 2
            width: control.indicatorSize
            height: control.indicatorSize
            implicitWidth: control.indicatorSize
            implicitHeight: control.indicatorSize
            radius: control.indicatorRadius
            color: control.indicatorColor
            antialiasing: true

            Rectangle {
                id: dot
                objectName: control.objectName.length > 0 ? control.objectName + "_dot" : ""
                width: control.dotSize
                height: control.dotSize
                radius: control.dotRadius
                color: control.indicatorDotColor
                anchors.centerIn: parent
                visible: control.checked
                antialiasing: true
            }
        }

        Label {
            id: label
            objectName: control.objectName.length > 0 ? control.objectName + "_label" : ""
            x: indicator.x + indicator.width + contentLayout.spacing
            y: (contentLayout.height - height) / 2
            width: visible ? contentLayout.labelWidth : 0
            height: Theme.textBodyLineHeight
            style: body
            text: control.text
            color: control.enabled ? Theme.textPrimary : Theme.textOctonary
            visible: control.text.length > 0
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.RadioButton { text: "Choice A"; checked: true }

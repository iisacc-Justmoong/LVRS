import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.AbstractButton {
    id: control

    enum ButtonTone {
        Primary,
        Default,
        Borderless,
        Destructive,
        Disabled
    }
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1

    property int tone: AbstractButton.Default
    property int shapeStyle: shapeRoundRect
    property bool effectiveEnabled: enabled && tone !== AbstractButton.Disabled
    property alias method: methodRegistry.method
    property alias methods: methodRegistry.methods
    readonly property alias hasInjectedMethods: methodRegistry.hasInjectedMethods

    readonly property color toneTextColor: {
        if (tone === AbstractButton.Borderless)
            return Theme.primary
        return Theme.textPrimary
    }
    readonly property color toneBackgroundColor: {
        if (tone === AbstractButton.Primary)
            return Theme.primary
        if (tone === AbstractButton.Destructive)
            return Theme.danger
        if (tone === AbstractButton.Borderless)
            return "transparent"
        if (tone === AbstractButton.Disabled)
            return Theme.panelBackground04
        return Theme.panelBackground12
    }
    readonly property color toneBackgroundColorHover: {
        if (tone === AbstractButton.Primary)
            return Qt.darker(Theme.primary, 1.12)
        if (tone === AbstractButton.Destructive)
            return Qt.darker(Theme.danger, 1.12)
        if (tone === AbstractButton.Borderless)
            return Theme.surfaceAlt
        return Theme.surfaceAlt
    }
    readonly property color toneBackgroundColorPressed: {
        if (tone === AbstractButton.Primary)
            return Qt.darker(Theme.primary, 1.2)
        if (tone === AbstractButton.Destructive)
            return Qt.darker(Theme.danger, 1.2)
        if (tone === AbstractButton.Borderless)
            return Theme.accentBlueMuted
        return Theme.accentBlueMuted
    }
    horizontalPadding: Theme.gap14
    verticalPadding: Theme.gap10
    property int cornerRadius: Theme.radiusMd
    readonly property real resolvedCornerRadius: shapeStyle === shapeCylinder
        ? Math.max(0, Math.min(width, height) / 2)
        : cornerRadius

    property color textColor: control.toneTextColor
    property color textColorDisabled: Theme.textOctonary

    property color backgroundColor: control.toneBackgroundColor
    property color backgroundColorHover: control.toneBackgroundColorHover
    property color backgroundColorPressed: control.toneBackgroundColorPressed
    property color backgroundColorDisabled: Theme.panelBackground04

    hoverEnabled: control.effectiveEnabled
    focusPolicy: control.effectiveEnabled ? Qt.StrongFocus : Qt.NoFocus
    activeFocusOnTab: control.effectiveEnabled

    leftPadding: horizontalPadding
    rightPadding: horizontalPadding
    topPadding: verticalPadding
    bottomPadding: verticalPadding
    spacing: Theme.gap8

    implicitHeight: Math.max(Theme.controlHeightMd, contentItem.implicitHeight + topPadding + bottomPadding)
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding

    onEffectiveEnabledChanged: {
        if (!control.effectiveEnabled && control.activeFocus)
            control.focus = false
    }

    function createMethodEvent(triggerName) {
        return methodRegistry.createEvent(triggerName)
    }

    function invokeMethod(candidate, eventData) {
        return methodRegistry.invokeMethod(candidate, eventData)
    }

    function invokeMethods(eventData) {
        return methodRegistry.invokeMethods(eventData)
    }

    ButtonMethodRegistry {
        id: methodRegistry
        owner: control
        defaultTrigger: "clicked"
    }

    Connections {
        target: control
        function onClicked() {
            control.invokeMethods(control.createMethodEvent("clicked"))
        }
    }

    contentItem: Label {
        style: body
        text: control.text
        color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: control.resolvedCornerRadius
        antialiasing: true
        color: !control.effectiveEnabled
            ? control.backgroundColorDisabled
            : control.down
                ? control.backgroundColorPressed
                : control.hovered
                    ? control.backgroundColorHover
                    : control.backgroundColor
    }

    MouseArea {
        anchors.fill: parent
        enabled: !control.effectiveEnabled
        acceptedButtons: Qt.AllButtons
        hoverEnabled: enabled
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.AbstractButton { text: "Action"; tone: LV.AbstractButton.Primary; method: function(eventData) { ... } }

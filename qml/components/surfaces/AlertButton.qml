import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    // Opt in only from Alert; Modal and standalone buttons retain their baseline.
    property bool dialogStyle: false
    tone: AbstractButton.Default
    horizontalPadding: dialogStyle ? Theme.gap16 : Theme.gap8
    verticalPadding: Theme.gap4
    spacing: Theme.gapNone
    cornerRadius: dialogStyle ? Theme.radiusXl : Theme.radiusSm
    implicitHeight: Theme.gap20
    height: Theme.gap20
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    readonly property bool isPrimaryTone: tone === AbstractButton.Primary
    readonly property bool isDefaultTone: tone === AbstractButton.Default
    textColor: dialogStyle ? Theme.alertTitleColor : toneTextColor

    // Dialog actions use the Figma primary fill or a transparent outlined surface.
    backgroundColor: isPrimaryTone
        ? (dialogStyle ? Theme.alertActionPrimary : Theme.accent)
        : isDefaultTone
            ? (dialogStyle ? "transparent" : Theme.panelBackground12)
            : toneBackgroundColor
    backgroundColorHover: isPrimaryTone
        ? Qt.darker(dialogStyle ? Theme.alertActionPrimary : Theme.accent, 1.12)
        : isDefaultTone
            ? (dialogStyle ? Qt.rgba(1, 1, 1, 0.05) : Qt.lighter(Theme.panelBackground12, 1.08))
            : toneBackgroundColorHover
    backgroundColorPressed: isPrimaryTone
        ? Qt.darker(dialogStyle ? Theme.alertActionPrimary : Theme.accent, 1.2)
        : isDefaultTone
            ? (dialogStyle ? Qt.rgba(1, 1, 1, 0.09) : Theme.panelBackground10)
            : toneBackgroundColorPressed
    backgroundColorDisabled: Theme.panelBackground09

    background: Rectangle {
        radius: control.resolvedCornerRadius
        antialiasing: true
        color: !control.effectiveEnabled ? control.backgroundColorDisabled
               : control.down ? control.backgroundColorPressed
               : control.hovered ? control.backgroundColorHover
               : control.backgroundColor
        border.width: control.dialogStyle && control.isDefaultTone ? Theme.scaleRealMetric(1) : 0
        border.color: Theme.alertActionBorder
    }

    contentItem: Label {
        style: body
        text: control.text
        color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.AlertButton { text: "Button"; tone: LV.AbstractButton.Primary }

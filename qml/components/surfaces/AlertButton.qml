import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Default
    horizontalPadding: Theme.gap8
    verticalPadding: Theme.gap4
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm
    implicitHeight: Theme.gap20
    height: Theme.gap20
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    readonly property bool isPrimaryTone: tone === AbstractButton.Primary
    readonly property bool isDefaultTone: tone === AbstractButton.Default

    // Alert default actions use a brighter neutral surface per Figma spec.
    backgroundColor: isPrimaryTone
        ? Theme.accent
        : isDefaultTone
            ? Theme.panelBackground12
            : toneBackgroundColor
    backgroundColorHover: isPrimaryTone
        ? Qt.darker(Theme.accent, 1.12)
        : isDefaultTone
            ? Qt.lighter(Theme.panelBackground12, 1.08)
            : toneBackgroundColorHover
    backgroundColorPressed: isPrimaryTone
        ? Qt.darker(Theme.accent, 1.2)
        : isDefaultTone
            ? Theme.panelBackground10
            : toneBackgroundColorPressed
    backgroundColorDisabled: Theme.panelBackground09

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

import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.gap20
    horizontalPadding: Theme.gap8
    verticalPadding: Theme.gap4
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding

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
// LV.LabelButton { text: "Button"; tone: LV.AbstractButton.Primary }

import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.iconSm + (Theme.gap2 * 2)
    horizontalPadding: Theme.gap8
    verticalPadding: Math.max(
        0,
        (control.figmaButtonHeight - Theme.textBodyLineHeight) / 2)
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: Math.ceil(contentItem.implicitWidth) + leftPadding + rightPadding

    contentItem: Label {
        objectName: "labelButton_label"
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
// LV.LabelButton { text: "Button" }

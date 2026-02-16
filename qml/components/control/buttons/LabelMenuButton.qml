import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.gap20
    readonly property string indicatorNameDefault: "panDownSymbolicDefault"
    readonly property string indicatorNameBorderless: "panDownSymbolicBorderless"
    readonly property string indicatorNameAccent: "panDownSymbolicAccent"
    readonly property string indicatorNameDisabled: "panDownSymbolicDisabled"
    readonly property string resolvedIndicatorName: !control.effectiveEnabled
        ? control.indicatorNameDisabled
        : control.tone === AbstractButton.Borderless
            ? control.indicatorNameBorderless
            : control.tone === AbstractButton.Primary || control.tone === AbstractButton.Destructive
                ? control.indicatorNameAccent
                : control.indicatorNameDefault
    readonly property int iconRevision: SvgManager.revision
    readonly property string renderedIndicatorSource: {
        control.iconRevision
        return SvgManager.icon(
                    Theme.iconPath(control.resolvedIndicatorName),
                    Theme.iconSm)
    }

    horizontalPadding: Theme.gap8
    verticalPadding: Theme.gap2
    spacing: Theme.gap2
    cornerRadius: Theme.radiusSm
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    clip: true

    contentItem: RowLayout {
        spacing: Theme.gap2
        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

        Label {
            style: body
            text: control.text
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            elide: Text.ElideRight
            Layout.alignment: Qt.AlignVCenter
        }

        Image {
            source: control.renderedIndicatorSource
            sourceSize.width: Theme.iconSm
            sourceSize.height: Theme.iconSm
            fillMode: Image.PreserveAspectFit
            smooth: true
            Layout.preferredWidth: Theme.iconSm
            Layout.preferredHeight: Theme.iconSm
            Layout.minimumWidth: Theme.iconSm
            Layout.minimumHeight: Theme.iconSm
            Layout.maximumWidth: Theme.iconSm
            Layout.maximumHeight: Theme.iconSm
            Layout.alignment: Qt.AlignVCenter
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.LabelMenuButton { text: "Open"; tone: LV.AbstractButton.Default }

import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.gap20
    readonly property string indicatorNameDefault: "generalchevronDown"
    readonly property string indicatorNameBorderless: "generalchevronDownBorderless"
    readonly property string indicatorNameAccent: "generalchevronDownAccent"
    readonly property string indicatorNameDisabled: "generalchevronDownDisabled"
    readonly property string resolvedIndicatorName: !control.effectiveEnabled
        ? control.indicatorNameDisabled
        : control.tone === AbstractButton.Borderless
            ? control.indicatorNameBorderless
            : control.tone === AbstractButton.Primary || control.tone === AbstractButton.Destructive
                ? control.indicatorNameAccent
                : control.indicatorNameDefault
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int indicatorSourceSize: Math.max(1, Math.round(Theme.iconSm * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property url renderedIndicatorSource: Theme.iconPath(control.resolvedIndicatorName)

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
            source: RenderQuality.resolveTextureSource(control.renderedIndicatorSource)
            sourceSize.width: control.indicatorSourceSize
            sourceSize.height: control.indicatorSourceSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: RenderQuality.mipmapEnabled
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

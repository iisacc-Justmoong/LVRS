import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.iconSm + (Theme.gap2 * 2)
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
    readonly property int indicatorSize: Theme.iconSm
    readonly property int indicatorSourceSize: Math.max(1, Math.round(control.indicatorSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property url renderedIndicatorSource: Theme.iconPath(control.resolvedIndicatorName)

    horizontalPadding: Theme.gap8
    verticalPadding: Theme.gap2
    spacing: -Theme.gap2
    cornerRadius: Theme.radiusSm
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: Math.ceil(contentItem.implicitWidth) + leftPadding + rightPadding
    clip: true

    contentItem: Item {
        id: contentRoot
        objectName: "labelMenuButton_content"
        readonly property int naturalLabelWidth: Math.ceil(labelItem.implicitWidth)
        implicitWidth: naturalLabelWidth + control.spacing + indicatorImage.width
        implicitHeight: control.indicatorSize

        Label {
            id: labelItem
            objectName: "labelMenuButton_label"
            x: 0
            y: (parent.height - height) / 2
            width: Math.min(
                contentRoot.naturalLabelWidth,
                Math.max(0, parent.width - control.spacing - indicatorImage.width))
            height: Theme.textBodyLineHeight
            style: body
            text: control.text
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image {
            id: indicatorImage
            objectName: "labelMenuButton_indicator"
            x: labelItem.x + labelItem.width + control.spacing
            y: 0
            width: control.indicatorSize
            height: control.indicatorSize
            source: RenderQuality.resolveTextureSource(control.renderedIndicatorSource)
            sourceSize.width: control.indicatorSourceSize
            sourceSize.height: control.indicatorSourceSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: RenderQuality.mipmapEnabled
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.LabelMenuButton { text: "Open" }

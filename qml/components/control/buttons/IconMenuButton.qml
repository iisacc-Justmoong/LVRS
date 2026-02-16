import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.gap20
    readonly property string iconNameDefault: "viewMoreSymbolicDefault"
    readonly property string iconNameBorderless: "viewMoreSymbolicBorderless"
    readonly property string iconNameDisabled: "viewMoreSymbolicDisabled"
    readonly property url iconSourceDefault: Theme.iconPath(control.iconNameDefault)
    readonly property url iconSourceBorderless: Theme.iconPath(control.iconNameBorderless)
    readonly property url iconSourceDisabled: Theme.iconPath(control.iconNameDisabled)

    property url url: ""
    property alias iconSource: control.url
    property string iconName: ""
    property string iconGlyph: ""
    property int iconSize: Theme.iconSm
    readonly property string resolvedIconName: {
        const explicitIconName = control.iconName === undefined || control.iconName === null
            ? ""
            : String(control.iconName).trim()
        if (explicitIconName.length > 0)
            return explicitIconName
        const groupedIconName = control.icon && control.icon.name !== undefined && control.icon.name !== null
            ? String(control.icon.name).trim()
            : ""
        if (groupedIconName.length > 0)
            return groupedIconName
        return ""
    }
    readonly property url resolvedIconSource: control.url.toString().length > 0
        ? control.url
        : control.resolvedIconName.length > 0
            ? Theme.iconPath(control.resolvedIconName)
            : !control.effectiveEnabled
                ? control.iconSourceDisabled
                : control.tone === AbstractButton.Borderless
                    ? control.iconSourceBorderless
                    : control.iconSourceDefault
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale))
    readonly property int indicatorSourceSize: Math.max(1, Math.round(Theme.iconSm * control.iconSupersampleScale))
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
    readonly property url renderedIndicatorSource: Theme.iconPath(control.resolvedIndicatorName)

    horizontalPadding: Theme.gap2
    verticalPadding: Theme.gap2
    spacing: Theme.gap4
    cornerRadius: Theme.radiusSm
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    clip: true

    contentItem: RowLayout {
        spacing: Theme.gap4
        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

        Image {
            visible: control.iconGlyph.length === 0
            source: control.resolvedIconSource
            sourceSize.width: control.iconSourceSize
            sourceSize.height: control.iconSourceSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            Layout.preferredWidth: control.iconSize
            Layout.preferredHeight: control.iconSize
            Layout.minimumWidth: control.iconSize
            Layout.minimumHeight: control.iconSize
            Layout.maximumWidth: control.iconSize
            Layout.maximumHeight: control.iconSize
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            style: body
            visible: control.iconGlyph.length > 0
            text: control.iconGlyph
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            font.pixelSize: control.iconSize
            font.weight: Font.Normal
            lineHeight: control.iconSize
            lineHeightMode: Text.FixedHeight
            Layout.alignment: Qt.AlignVCenter
        }

        Image {
            source: control.renderedIndicatorSource
            sourceSize.width: control.indicatorSourceSize
            sourceSize.height: control.indicatorSourceSize
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
// LV.IconMenuButton { tone: LV.AbstractButton.Default; iconName: "viewMoreSymbolicDefault" }

import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.gap20
    readonly property string fallbackIconName: "projectStructure"
    readonly property url fallbackIconSource: Theme.iconPath(control.fallbackIconName)

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
            : control.fallbackIconSource
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale))

    horizontalPadding: Theme.gap2
    verticalPadding: Theme.gap2
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding

    contentItem: RowLayout {
        spacing: control.text.length > 0 ? Theme.gap4 : Theme.gapNone
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

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

        Label {
            style: body
            text: control.text
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            elide: Text.ElideRight
            visible: control.text.length > 0
            Layout.alignment: Qt.AlignVCenter
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.IconButton { tone: LV.AbstractButton.Primary; iconName: "add" }

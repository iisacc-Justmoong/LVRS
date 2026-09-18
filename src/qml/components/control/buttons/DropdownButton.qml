import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    // Figma Type: false = LabelMenuButton, true = IconMenuButton.
    property bool iconMode: false

    tone: AbstractButton.Primary
    readonly property int figmaButtonHeight: Theme.iconSm + (Theme.gap2 * 2)
    readonly property string fallbackIconName: "projectStructure"
    readonly property url fallbackIconSource: Theme.iconPath(control.fallbackIconName)

    property url url: ""
    property alias iconSource: control.url
    property string iconName: ""
    property string iconGlyph: ""
    property int iconSize: Theme.iconSm
    readonly property int indicatorSize: Theme.iconSm
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
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property int indicatorSourceSize: Math.max(1, Math.round(control.indicatorSize * control.iconSupersampleScale * control.iconHiDpiScale))
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
    readonly property url renderedIndicatorSource: Theme.iconPath(control.resolvedIndicatorName)

    horizontalPadding: iconMode ? Theme.gap4 : Theme.gap8
    rightPadding: Theme.gap2
    verticalPadding: Theme.gap2
    spacing: iconMode ? -Theme.gap2 : Theme.gapNone
    cornerRadius: Theme.radiusMd
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: Math.ceil(contentItem.implicitWidth) + leftPadding + rightPadding
    clip: true

    contentItem: Item {
        id: contentRoot
        objectName: control.iconMode ? "iconMenuButton_content" : "labelMenuButton_content"
        readonly property int naturalLabelWidth: Math.ceil(labelItem.implicitWidth)
        implicitWidth: (control.iconMode ? control.iconSize : naturalLabelWidth)
                       + control.spacing + control.indicatorSize
        implicitHeight: Math.max(control.iconMode ? control.iconSize : Theme.textBodyLineHeight,
                                 control.indicatorSize)

        Label {
            id: labelItem
            objectName: "labelMenuButton_label"
            visible: !control.iconMode
            width: Math.min(contentRoot.naturalLabelWidth,
                            Math.max(0, parent.width - control.spacing - control.indicatorSize))
            height: Theme.textBodyLineHeight
            y: (parent.height - height) / 2
            style: body
            text: control.text
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image {
            objectName: "iconMenuButton_icon"
            visible: control.iconMode && control.iconGlyph.length === 0
            width: control.iconSize
            height: control.iconSize
            y: (parent.height - height) / 2
            source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
            sourceSize.width: control.iconSourceSize
            sourceSize.height: control.iconSourceSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: RenderQuality.mipmapEnabled
        }

        Label {
            visible: control.iconMode && control.iconGlyph.length > 0
            width: control.iconSize
            height: control.iconSize
            y: (parent.height - height) / 2
            style: body
            text: control.iconGlyph
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            font.pixelSize: control.iconSize
            fontSizeMode: Text.Fit
            minimumPixelSize: 1
            font.weight: Font.Normal
            lineHeight: control.iconSize
            lineHeightMode: Text.FixedHeight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            clip: true
        }

        Image {
            objectName: control.iconMode ? "iconMenuButton_indicator" : "labelMenuButton_indicator"
            x: (control.iconMode ? control.iconSize : labelItem.width) + control.spacing
            y: (parent.height - height) / 2
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
// LV.DropdownButton { text: "Open"; method: function(eventData) { menu.open() } }
// LV.DropdownButton { iconMode: true; iconName: "projectStructure" }

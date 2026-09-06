import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    // Figma Type: false = LabelButton, true = IconButton.
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

    horizontalPadding: iconMode ? Theme.gap2 : Theme.gap8
    verticalPadding: iconMode ? Theme.gap2
        : Math.max(0, (figmaButtonHeight - Theme.textBodyLineHeight) / 2)
    spacing: iconMode ? (tone === AbstractButton.Primary ? Theme.gapNone : Theme.gap7) : Theme.gap10
    cornerRadius: Theme.radiusMd
    height: figmaButtonHeight
    implicitHeight: figmaButtonHeight
    implicitWidth: Math.ceil(Math.max(contentItem.implicitWidth, control.iconMode ? control.iconSize : 0))
        + leftPadding + rightPadding

    contentItem: RowLayout {
        objectName: control.iconMode ? "iconButton_content" : "pushButton_content"
        spacing: control.spacing
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

        Image {
            objectName: "iconButton_icon"
            visible: control.iconMode && control.iconGlyph.length === 0
            source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
            sourceSize.width: control.iconSourceSize
            sourceSize.height: control.iconSourceSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: RenderQuality.mipmapEnabled
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
            visible: control.iconMode && control.iconGlyph.length > 0
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
            Layout.preferredWidth: control.iconSize
            Layout.preferredHeight: control.iconSize
            Layout.minimumWidth: control.iconSize
            Layout.minimumHeight: control.iconSize
            Layout.maximumWidth: control.iconSize
            Layout.maximumHeight: control.iconSize
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            objectName: "labelButton_label"
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.textBodyLineHeight
            Layout.minimumHeight: Theme.textBodyLineHeight
            Layout.maximumHeight: Theme.textBodyLineHeight
            style: body
            text: control.text
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            visible: control.text.length > 0
            Layout.alignment: Qt.AlignVCenter
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.PushButton { text: "Button"; method: function(eventData) { save() } }
// LV.PushButton { iconMode: true; iconName: "add" }

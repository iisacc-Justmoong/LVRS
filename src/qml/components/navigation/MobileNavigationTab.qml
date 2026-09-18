pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import LVRS 1.0

Tab {
    id: control
    // Figma Tab/LVRS Mobile, 1073:1291.
    property bool showLabel: text.length > 0
    property bool drawSelection: true
    readonly property color foreground: !effectiveEnabled ? Theme.disabledColor
        : effectiveSelected ? Theme.primary : Theme.bodyColor
    iconSize: Theme.mobileNavigationIconSize
    implicitWidth: Theme.mobileNavigationTabWidth
    implicitHeight: Theme.mobileNavigationTouchSize
    horizontalPadding: 0
    verticalPadding: 0
    bottomPadding: 0
    cornerRadius: Theme.mobileNavigationRadiusIOS - Theme.gap4
    motionStrength: 0
    tabStyle: Tab.Surface

    contentItem: Item {
        Column {
            anchors.centerIn: parent
            width: parent.width
            spacing: Theme.gap2
            Image {
                objectName: "mobileNavigationIcon"
                anchors.horizontalCenter: parent.horizontalCenter
                width: control.iconSize
                height: control.iconSize
                source: RenderQuality.resolveTextureSource(control.iconSource)
                sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
                fillMode: Image.PreserveAspectFit
                opacity: control.foreground.a
                layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
                layer.effect: MultiEffect {
                    // Normalize luminance before tinting: the Figma icon uses
                    // one solid foreground, regardless of SVG source colors.
                    contrast: -1
                    brightness: 0.5
                    colorization: 1
                    colorizationColor: Qt.rgba(control.foreground.r, control.foreground.g, control.foreground.b, 1)
                }
                Accessible.ignored: true
            }
            Label {
                objectName: control.labelObjectName
                visible: control.showLabel
                width: parent.width
                style: caption
                text: control.text
                color: !control.effectiveEnabled ? Theme.disabledColor
                    : control.effectiveSelected ? Theme.titleHeaderColor : Theme.bodyColor
                horizontalAlignment: Text.AlignHCenter
                lineHeight: Theme.textCaptionLineHeight
                lineHeightMode: Text.FixedHeight
                elide: Text.ElideRight
                Accessible.ignored: true
            }
        }
    }
    background: Rectangle {
        radius: control.resolvedCornerRadius
        color: control.drawSelection && control.effectiveSelected ? Theme.panelBackground10 : "transparent"
        antialiasing: true
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Theme.bodyColor
            opacity: !control.effectiveEnabled ? 0 : control.effectivePressed ? 0.12 : control.effectiveHovered ? 0.06 : 0
            Behavior on opacity { NumberAnimation { duration: control.motionEnabled ? Motion.duration(Motion.hoverDuration) : 0 } }
        }
    }
}

// LV.MobileNavigationTab { iconName: "home-1"; Accessible.name: "Home"; text: ""; selected: true }

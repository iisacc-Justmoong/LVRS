pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import LVRS 1.0

Tab {
    id: control
    enum PlatformStyle { Automatic, IOS, Android }
    property int platformStyle: MobileTab.Automatic
    readonly property int resolvedPlatformStyle: platformStyle === MobileTab.Automatic
        ? (Theme.effectiveTarget === "android" ? MobileTab.Android : MobileTab.IOS) : platformStyle
    property bool showLabel: true
    property bool badgeDot: false
    property real leadingInset: 0
    readonly property bool iosStyle: resolvedPlatformStyle === MobileTab.IOS
    readonly property color foreground: !effectiveEnabled ? Theme.disabledColor
        : iosStyle ? (effectiveSelected ? Theme.primary : Theme.titleHeaderColor)
        : effectiveSelected ? Theme.mobileTabAndroidSelectedText : Theme.mobileTabAndroidText
    iconSize: iosStyle ? 28 : 24
    implicitHeight: iosStyle ? 54 : 64
    implicitWidth: iosStyle ? 76 : 80
    horizontalPadding: 2
    leftPadding: horizontalPadding + leadingInset
    verticalPadding: 0
    bottomPadding: 0
    cornerRadius: iosStyle ? height / 2 : 16
    showIcon: true

    contentItem: Item {
        Image {
            id: symbol
            objectName: "mobileTabIcon"
            width: control.iconSize
            height: control.iconSize
            anchors.horizontalCenter: parent.horizontalCenter
            y: control.showLabel ? (control.iosStyle ? 4 : 10) : (parent.height - height) / 2
            source: RenderQuality.resolveTextureSource(control.iconSource)
            sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
            fillMode: Image.PreserveAspectFit
            opacity: control.effectiveEnabled ? 1 : 0.3
            layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
            layer.effect: MultiEffect { colorization: 1; colorizationColor: control.foreground }
            Accessible.ignored: true
        }
        Label {
            objectName: control.labelObjectName
            anchors.bottom: parent.bottom
            anchors.bottomMargin: control.iosStyle ? 4 : 6
            width: parent.width
            visible: control.showLabel
            style: caption
            text: control.text
            font.family: control.iosStyle ? FontPolicy.systemFamily : FontPolicy.resolveFamily("Roboto")
            font.styleName: ""
            font.pixelSize: control.iosStyle ? 10 : 12
            font.weight: control.iosStyle ? Font.DemiBold : Font.Medium
            lineHeight: control.iosStyle ? 12 : 16
            lineHeightMode: Text.FixedHeight
            color: control.foreground
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            Accessible.ignored: true
        }
        Rectangle {
            objectName: "mobileTabBadge"
            visible: control.badgeDot || control.showBadge
            x: symbol.x + symbol.width - 2
            y: Math.max(0, symbol.y - 2)
            width: control.badgeDot ? 6 : Math.max(16, badgeText.implicitWidth + 8)
            height: control.badgeDot ? 6 : 16
            radius: height / 2
            color: control.iosStyle ? Theme.danger : Theme.mobileTabAndroidBadge
            Label {
                id: badgeText
                anchors.centerIn: parent
                visible: !control.badgeDot
                text: control.badge
                style: caption
                color: control.iosStyle ? "white" : "#601410"
                Accessible.ignored: true
            }
        }
    }
    background: Item {
        Rectangle {
            objectName: "mobileTabIndicator"
            x: control.leadingInset + (parent.width - control.leadingInset - width) / 2
            y: control.iosStyle ? 0 : 6
            width: control.iosStyle ? parent.width - control.leadingInset : Math.min(56, parent.width)
            height: control.iosStyle ? parent.height : 32
            radius: height / 2
            color: control.effectiveSelected
                ? (control.iosStyle ? Theme.panelBackground10 : Theme.mobileTabAndroidIndicator)
                : "transparent"
            StateColorBehavior on color { motionEnabled: control.motionEnabled }
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: control.foreground
                opacity: control.effectivePressed ? 0.12 : control.effectiveHovered ? 0.08 : 0
            }
        }
    }
}

// LV.MobileTab { text: "Library"; iconName: "nodesfolder"; selected: true; platformStyle: LV.MobileTab.IOS }

import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    readonly property int defaultState: 0
    readonly property int selectedState: 1
    readonly property int inactiveState: 2
    readonly property int directionRight: 0
    readonly property int directionLeft: 1
    readonly property int directionUp: 2
    readonly property int directionDown: 3

    property int state: defaultState
    property string label: "Label"
    property alias key: shortcutLabel.text
    property alias shortcut: shortcutLabel.text
    property bool showChevron: true
    // Supports int enum or string: right|left|up|down
    property var selectionDirection: directionRight
    property int itemWidth: 161
    property int itemHeight: 22
    property int iconSize: 16
    property int chevronSize: 16
    property string iconName: ""
    property url iconSource: ""
    property color iconPlaceholderColor: Theme.darkGrey10
    property color chevronColor: Theme.descriptionColor
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property int chevronSourceSize: Math.max(1, Math.round(control.chevronSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property string chevronIconName: "generalchevronDown"
    readonly property url chevronIconSource: Theme.iconPath(control.chevronIconName)

    readonly property bool isSelected: state === selectedState
    readonly property bool isInactive: state === inactiveState
    readonly property string resolvedIconName: {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        return rawName.trim()
    }
    readonly property url resolvedIconSource: iconSource.toString().length > 0
        ? iconSource
        : resolvedIconName.length > 0
            ? Theme.iconPath(resolvedIconName)
            : ""
    readonly property int resolvedSelectionDirection: {
        const raw = selectionDirection
        if (typeof raw === "number")
            return Math.max(directionRight, Math.min(directionDown, Math.round(raw)))

        const normalized = raw === undefined || raw === null
            ? ""
            : String(raw).trim().toLowerCase()
        if (normalized === "left")
            return directionLeft
        if (normalized === "up")
            return directionUp
        if (normalized === "down")
            return directionDown
        return directionRight
    }
    readonly property color resolvedBackgroundColor: isSelected
        ? Theme.contextMenuItemSelectedBackground
        : isInactive
            ? Theme.contextMenuItemInactiveBackground
            : "transparent"
    readonly property real resolvedChevronRotation: {
        if (control.resolvedSelectionDirection === control.directionLeft)
            return 90
        if (control.resolvedSelectionDirection === control.directionUp)
            return 180
        if (control.resolvedSelectionDirection === control.directionDown)
            return 0
        return -90
    }

    tone: AbstractButton.Borderless
    horizontalPadding: Theme.gap4
    verticalPadding: Theme.gap3
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm

    implicitWidth: itemWidth
    implicitHeight: itemHeight

    textColor: Theme.titleHeaderColor
    textColorDisabled: Theme.disabledColor
    backgroundColor: resolvedBackgroundColor
    backgroundColorHover: resolvedBackgroundColor
    backgroundColorPressed: resolvedBackgroundColor
    backgroundColorDisabled: resolvedBackgroundColor

    contentItem: Item {
        id: contentRoot
        implicitWidth: Math.max(
                           control.itemWidth - control.leftPadding - control.rightPadding,
                           leftGroup.implicitWidth + Theme.gap8 + rightGroup.implicitWidth)
        implicitHeight: Math.max(leftGroup.implicitHeight, rightGroup.implicitHeight)

        RowLayout {
            id: leftGroup
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.gap8

            Item {
                Layout.preferredWidth: control.iconSize
                Layout.preferredHeight: control.iconSize
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: control.iconSize
                implicitHeight: control.iconSize

                Image {
                    id: iconImage
                    visible: control.resolvedIconSource.toString().length > 0
                    anchors.centerIn: parent
                    width: control.iconSize
                    height: control.iconSize
                    source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
                    sourceSize.width: control.iconSourceSize
                    sourceSize.height: control.iconSourceSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: RenderQuality.mipmapEnabled
                }

                Item {
                    anchors.fill: parent
                    visible: !iconImage.visible

                    Rectangle {
                        width: 12
                        height: 12
                        radius: Theme.gap3
                        color: control.iconPlaceholderColor
                        anchors.centerIn: parent
                        antialiasing: true
                    }
                }
            }

            Label {
                id: labelNode
                style: body
                text: control.label
                color: control.isInactive ? Theme.titleHeaderColor
                                          : (control.effectiveEnabled ? Theme.titleHeaderColor : Theme.disabledColor)
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight
                lineHeight: 12
                lineHeightMode: Text.FixedHeight
            }
        }

        RowLayout {
            id: rightGroup
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.gap8

            Label {
                id: shortcutLabel
                style: body
                visible: text.length > 0
                text: "key"
                color: control.isInactive ? Theme.descriptionColor
                                          : (control.effectiveEnabled ? Theme.descriptionColor : Theme.disabledColor)
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight
                lineHeight: 12
                lineHeightMode: Text.FixedHeight
            }

            Image {
                id: chevronIcon
                visible: control.showChevron
                Layout.preferredWidth: control.chevronSize
                Layout.preferredHeight: control.chevronSize
                Layout.alignment: Qt.AlignVCenter
                source: RenderQuality.resolveTextureSource(control.chevronIconSource)
                sourceSize.width: control.chevronSourceSize
                sourceSize.height: control.chevronSourceSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: RenderQuality.mipmapEnabled
                rotation: control.resolvedChevronRotation
                transformOrigin: Item.Center
                opacity: control.isInactive ? 1.0 : (control.effectiveEnabled ? 1.0 : 0.45)
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.MenuItem { iconName: "iconname"; state: selectedState; label: "Label"; key: "key"; selectionDirection: "right" }

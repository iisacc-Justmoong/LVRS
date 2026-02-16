import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    readonly property int defaultState: 0
    readonly property int selectedState: 1
    readonly property int inactiveState: 2

    property int state: defaultState
    property string label: "Label"
    property alias key: shortcutLabel.text
    property alias shortcut: shortcutLabel.text
    property bool showChevron: true
    property int itemWidth: 161
    property int itemHeight: 22
    property int iconSize: 16
    property int chevronSize: 16
    property string iconName: ""
    property url iconSource: ""
    property color iconPlaceholderColor: Theme.darkGrey10
    property color iconPlaceholderAccentColor: Qt.lighter(Theme.darkGrey10, 1.2)
    property color chevronColor: Theme.descriptionColor
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale))

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
    readonly property color resolvedBackgroundColor: isSelected
        ? Theme.contextMenuItemSelectedBackground
        : isInactive
            ? Theme.contextMenuItemInactiveBackground
            : "transparent"

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
                    source: control.resolvedIconSource
                    sourceSize.width: control.iconSourceSize
                    sourceSize.height: control.iconSourceSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
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

                    Rectangle {
                        width: 5
                        height: 5
                        radius: Theme.gap2
                        color: control.iconPlaceholderAccentColor
                        anchors.left: parent.horizontalCenter
                        anchors.top: parent.verticalCenter
                        anchors.leftMargin: -5
                        anchors.topMargin: -5
                        antialiasing: true
                    }
                }
            }

            Label {
                style: body
                text: control.label
                color: control.isInactive ? Theme.titleHeaderColor
                                          : (control.effectiveEnabled ? Theme.titleHeaderColor : Theme.disabledColor)
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight
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
            }

            Canvas {
                id: chevronCanvas
                visible: control.showChevron
                Layout.preferredWidth: control.chevronSize
                Layout.preferredHeight: control.chevronSize
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: control.chevronSize
                implicitHeight: control.chevronSize
                antialiasing: true

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (!control.showChevron)
                        return

                    ctx.beginPath()
                    ctx.moveTo(width * 0.38, height * 0.28)
                    ctx.lineTo(width * 0.58, height * 0.5)
                    ctx.lineTo(width * 0.38, height * 0.72)
                    ctx.lineWidth = 1.6
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.strokeStyle = control.isInactive
                        ? control.chevronColor
                        : (control.effectiveEnabled ? control.chevronColor : Theme.disabledColor)
                    ctx.stroke()
                }
            }
        }
    }

    onChevronColorChanged: chevronCanvas.requestPaint()
    onShowChevronChanged: chevronCanvas.requestPaint()
    onEnabledChanged: chevronCanvas.requestPaint()
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.MenuItem { iconName: "iconname"; state: selectedState; label: "Label"; key: "key" }

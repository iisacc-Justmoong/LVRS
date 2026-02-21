pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property string icon1: ""
    property string icon2: ""
    property string icon3: ""
    property int iconSize: Theme.iconSm
    property int buttonSize: Theme.controlHeightSm
    property int horizontalPadding: Theme.gap8
    property int verticalPadding: Theme.gap4
    property int spacing: Theme.gap4
    property color backgroundColor: Theme.subSurface
    property bool interactive: true
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale))

    signal iconClicked(int index, string source)

    function resolvedIconSource(value) {
        const raw = value === undefined || value === null ? "" : String(value).trim()
        if (raw.length === 0)
            return ""
        return Theme.iconPath(raw)
    }

    readonly property string resolvedIcon1: resolvedIconSource(icon1)
    readonly property string resolvedIcon2: resolvedIconSource(icon2)
    readonly property string resolvedIcon3: resolvedIconSource(icon3)

    function iconAt(index) {
        if (index === 0)
            return resolvedIcon1
        if (index === 1)
            return resolvedIcon2
        if (index === 2)
            return resolvedIcon3
        return ""
    }

    implicitWidth: rowLayout.implicitWidth + (horizontalPadding * 2)
    implicitHeight: Math.max(buttonSize, rowLayout.implicitHeight) + (verticalPadding * 2)

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMd
        color: control.backgroundColor
        antialiasing: true
    }

        RowLayout {
            id: rowLayout
            anchors.fill: parent
            anchors.leftMargin: control.horizontalPadding
            anchors.rightMargin: control.horizontalPadding
            anchors.topMargin: control.verticalPadding
            anchors.bottomMargin: control.verticalPadding
            spacing: control.spacing

        Repeater {
            model: 3

            delegate: AbstractButton {
                id: slotButton
                required property int index
                readonly property string slotIconSource: control.iconAt(index)

                Layout.preferredWidth: control.buttonSize
                Layout.preferredHeight: control.buttonSize
                Layout.alignment: Qt.AlignVCenter

                tone: AbstractButton.Borderless
                enabled: control.interactive && slotIconSource.length > 0
                horizontalPadding: Theme.gap4
                verticalPadding: Theme.gap4
                backgroundColor: "transparent"
                backgroundColorDisabled: "transparent"
                backgroundColorHover: Theme.surfaceAlt
                backgroundColorPressed: Theme.surfaceAlt

                contentItem: Item {
                    implicitWidth: control.iconSize
                    implicitHeight: control.iconSize

                    Image {
                        anchors.centerIn: parent
                        visible: slotButton.slotIconSource.length > 0
                        source: RenderQuality.resolveTextureSource(slotButton.slotIconSource)
                        sourceSize.width: control.iconSourceSize
                        sourceSize.height: control.iconSourceSize
                        width: control.iconSize
                        height: control.iconSize
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: RenderQuality.mipmapEnabled
                    }
                }

                onClicked: control.iconClicked(index, slotIconSource)
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ListToolbar { icon1: "iconname"; icon2: "qrc:/.../icon.svg"; icon3: "iconname" }

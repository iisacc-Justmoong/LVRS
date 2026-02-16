import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    property string label: ""
    property string detail: ""
    property string iconName: ""
    property bool selected: false
    property bool showChevron: false

    property int rowHeight: Theme.controlHeightMd
    property int rowHorizontalPadding: Theme.gap8
    property int rowVerticalPadding: Theme.gap6
    property int contentSpacing: Theme.gap8
    property int iconSize: Theme.iconSm
    property color listBackgroundColor: Theme.surfaceGhost

    readonly property string resolvedIconSource: {
        const raw = iconName === undefined || iconName === null ? "" : String(iconName).trim()
        if (raw.length === 0)
            return ""
        return Theme.iconPath(raw)
    }

    tone: AbstractButton.Borderless
    horizontalPadding: control.rowHorizontalPadding
    verticalPadding: control.rowVerticalPadding
    spacing: control.contentSpacing
    cornerRadius: Theme.radiusSm

    implicitHeight: control.rowHeight
    implicitWidth: Math.max(Theme.inputMinWidth, contentLayout.implicitWidth + leftPadding + rightPadding)

    backgroundColor: control.selected ? Theme.accentOverlay : listBackgroundColor
    backgroundColorHover: control.selected ? Theme.accentOverlay : Theme.subSurface
    backgroundColorPressed: control.selected ? Theme.accentOverlay : Theme.surfaceAlt
    backgroundColorDisabled: listBackgroundColor
    textColor: Theme.titleHeaderColor
    textColorDisabled: Theme.disabledColor

    contentItem: RowLayout {
        id: contentLayout
        spacing: control.contentSpacing

        Image {
            id: iconImage
            visible: control.resolvedIconSource.length > 0
            source: control.resolvedIconSource
            sourceSize.width: control.iconSize
            sourceSize.height: control.iconSize
            implicitWidth: control.iconSize
            implicitHeight: control.iconSize
            Layout.preferredWidth: control.iconSize
            Layout.preferredHeight: control.iconSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            id: labelNode
            style: body
            text: control.label
            color: control.effectiveEnabled ? Theme.bodyColor : Theme.disabledColor
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            elide: Text.ElideRight
        }

        Label {
            id: detailNode
            visible: text.length > 0
            style: description
            text: control.detail
            color: control.effectiveEnabled ? Theme.descriptionColor : Theme.disabledColor
            Layout.alignment: Qt.AlignVCenter
            elide: Text.ElideRight
        }

        Canvas {
            id: chevronNode
            visible: control.showChevron
            Layout.preferredWidth: Theme.iconSm
            Layout.preferredHeight: Theme.iconSm
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: Theme.iconSm
            implicitHeight: Theme.iconSm
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!control.showChevron)
                    return

                ctx.beginPath()
                ctx.moveTo(width * 0.32, height * 0.24)
                ctx.lineTo(width * 0.6, height * 0.5)
                ctx.lineTo(width * 0.32, height * 0.76)
                ctx.lineWidth = Theme.strokeRegular
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.strokeStyle = control.effectiveEnabled ? Theme.descriptionColor : Theme.disabledColor
                ctx.stroke()
            }
        }
    }

    onShowChevronChanged: chevronNode.requestPaint()
    onEnabledChanged: chevronNode.requestPaint()
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ListItem { label: "Label"; detail: "key"; iconName: "iconname"; showChevron: true }

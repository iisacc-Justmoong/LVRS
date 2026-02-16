import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    property string label: ""
    // Legacy compatibility fields kept for existing callers.
    property string detail: ""
    property string iconName: ""
    property bool selected: false
    property bool showChevron: false

    property int rowHorizontalPadding: Theme.gap4
    property int rowVerticalPadding: Theme.gap2
    property int separatorHeight: 1
    property int separatorTopSpacing: 1
    property int minItemWidth: 170
    property color listBackgroundColor: "transparent"
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5

    tone: AbstractButton.Borderless
    horizontalPadding: control.rowHorizontalPadding
    verticalPadding: control.rowVerticalPadding
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm

    implicitHeight: contentColumn.implicitHeight + topPadding + bottomPadding
    implicitWidth: Math.max(control.minItemWidth, contentColumn.implicitWidth + leftPadding + rightPadding)

    backgroundColor: control.selected ? Theme.accentOverlay : control.listBackgroundColor
    backgroundColorHover: control.selected ? Theme.accentOverlay : control.listBackgroundColor
    backgroundColorPressed: control.selected ? Theme.accentOverlay : control.listBackgroundColor
    backgroundColorDisabled: listBackgroundColor
    textColor: Theme.bodyColor
    textColorDisabled: Theme.disabledColor

    contentItem: Column {
        id: contentColumn
        spacing: control.separatorTopSpacing

        Label {
            id: labelItem
            style: body
            text: control.label
            color: control.effectiveEnabled ? Theme.bodyColor : Theme.disabledColor
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Rectangle {
            width: parent.width
            height: control.separatorHeight
            color: control.separatorColor
            opacity: control.separatorOpacity
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ListItem { label: "Label" }

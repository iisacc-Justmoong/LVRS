pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    implicitHeight: content.implicitHeight

    function showExample(index) {
        const item = corners.itemAt(index);
        if (item)
            bubble.openAt(item, Qt.point(item.width / 2, item.height / 2), index < 2 ? helpView : statusView);
    }

    Component {
        id: helpView
        Item {
            implicitWidth: 232
            implicitHeight: helpColumn.implicitHeight
            Column {
                id: helpColumn
                width: parent.width
                spacing: LV.Theme.gap8
                LV.Label { width: parent.width; style: header2; text: "Keep your work safe" }
                LV.Label {
                    width: parent.width; style: body; sizeToContentHeight: true
                    wrapMode: Text.WordWrap
                    text: "Changes are saved automatically. You can return to any earlier version from History."
                }
                LV.Label { style: caption; text: "⌘ S  ·  Save now" }
            }
        }
    }
    Component {
        id: statusView
        Item {
            implicitWidth: 248
            implicitHeight: statusColumn.implicitHeight
            Column {
                id: statusColumn
                width: parent.width
                spacing: LV.Theme.gap12
                LV.Label { style: header2; text: "Workspace sync" }
                LV.ProgressBar { width: parent.width; currentValue: 72 }
                LV.Label { style: body; text: "18 of 25 files uploaded" }
                LV.LabelButton { objectName: "tooltipAction"; text: "Got it"; onClicked: bubble.close() }
            }
        }
    }

    Column {
        id: content
        width: parent.width
        spacing: LV.Theme.gap16

        LV.Label {
            width: parent.width; style: body; wrapMode: Text.WordWrap; sizeToContentHeight: true
            text: "Choose a corner to preview placement. Drag the center point to watch the tail follow its origin."
        }

        Rectangle {
            id: displayFrame
            width: parent.width
            height: 352
            radius: LV.Theme.radiusXl
            color: LV.Theme.panelBackground03
            border.color: LV.Theme.panelBackground12

            LV.Label {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 42
                style: caption
                text: "Display boundary · content stays inside"
            }
            Repeater {
                id: corners
                model: ["Top left", "Top right", "Bottom left", "Bottom right"]
                LV.LabelButton {
                    required property int index
                    required property string modelData
                    text: modelData
                    x: index % 2 === 0 ? 16 : displayFrame.width - width - 16
                    y: index < 2 ? 16 : displayFrame.height - height - 16
                    onClicked: root.showExample(index)
                }
            }
            Rectangle {
                id: originPoint
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                width: 18; height: 18; radius: 9
                color: LV.Theme.accent
                DragHandler {
                    id: drag
                    xAxis.minimum: 12; xAxis.maximum: displayFrame.width - 30
                    yAxis.minimum: 12; yAxis.maximum: displayFrame.height - 30
                    onActiveChanged: {
                        if (active) {
                            bubble.close();
                            movingTip.open();
                        } else movingTip.close();
                    }
                }
            }
        }
        LV.LabelButton { id: hoverTarget; text: "Hover, focus, or hold for help" }
        LV.Tooltip {
            target: hoverTarget
            text: "Tooltips also accept plain text. Press Escape to dismiss."
        }
    }

    LV.Tooltip {
        id: bubble
        objectName: "galleryTooltip"
        automatic: false
        delay: 0
        timeout: -1
        availableRect: parent ? displayFrame.mapToItem(parent, 0, 0, displayFrame.width, displayFrame.height) : Qt.rect(0, 0, 0, 0)
    }
    LV.Tooltip {
        id: movingTip
        target: originPoint
        automatic: false
        delay: 0
        timeout: -1
        contentComponent: helpView
        availableRect: parent ? displayFrame.mapToItem(parent, 0, 0, displayFrame.width, displayFrame.height) : Qt.rect(0, 0, 0, 0)
    }
}

// API usage (external): TooltipGallery { width: parent.width }

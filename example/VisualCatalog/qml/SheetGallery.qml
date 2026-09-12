pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    property real deviceRadius: 55
    property int exampleIndex: 0
    property string feedback: "Choose an example. Radius values below are illustrative measurements."
    readonly property bool desktopExample: exampleIndex === 3
    implicitWidth: 900
    implicitHeight: layout.implicitHeight
    height: implicitHeight

    function showExample(index) {
        sheet.close();
        exampleIndex = index;
        Qt.callLater(function () {
            sheet.open();
        });
    }

    Column {
        id: layout
        width: parent.width
        spacing: LV.Theme.gap16
        LV.Label {
            style: header
            text: "Sheet · one content view, two presentations"
        }
        LV.Label {
            width: parent.width
            text: root.feedback
            wrapMode: Text.WordWrap
            sizeToContentHeight: true
        }
        Flow {
            width: parent.width
            spacing: LV.Theme.gap8
            Repeater {
                model: ["Share · Fit", "Export · Medium", "Folders · Large", "Export · Desktop", "Inline frame"]
                LV.LabelButton {
                    required property int index
                    required property string modelData
                    text: modelData
                    onClicked: root.showExample(index)
                }
            }
        }
        Flow {
            width: parent.width
            spacing: LV.Theme.gap8
            LV.Label {
                text: "Device radius (logical px): " + root.deviceRadius
            }
            LV.LabelButton {
                text: "28"
                onClicked: root.deviceRadius = 28
            }
            LV.LabelButton {
                text: "55"
                onClicked: root.deviceRadius = 55
            }
            LV.LabelButton {
                text: "Square · 0"
                onClicked: root.deviceRadius = 0
            }
        }
        Rectangle {
            id: stage
            objectName: "gallery_sheetStage"
            width: Math.min(parent.width, root.desktopExample ? 800 : 390)
            height: root.desktopExample ? 600 : 844
            radius: root.desktopExample ? 16 : root.deviceRadius
            color: LV.Theme.panelBackground01
            border.color: LV.Theme.panelBackground12
            Column {
                x: 20
                y: 60
                width: parent.width - 40
                spacing: LV.Theme.gap16
                LV.Label {
                    style: title2
                    text: "Projects"
                }
                LV.Label {
                    text: "Your creative workspace"
                }
                Repeater {
                    model: ["Brand refresh", "Product launch", "Project overview"]
                    Rectangle {
                        required property string modelData
                        width: parent.width
                        height: 64
                        radius: LV.Theme.radiusSm
                        color: LV.Theme.panelBackground05
                        LV.Label {
                            anchors.centerIn: parent
                            text: parent.modelData
                        }
                    }
                }
                LV.LabelButton {
                    text: "Open sheet"
                    onClicked: sheet.open()
                }
            }
        }
    }

    Component {
        id: shareView
        Column {
            spacing: LV.Theme.gap16
            LV.Label {
                text: "Invite people to this project."
            }
            LV.Label {
                text: "Project access · Only invited"
            }
            LV.LabelButton {
                width: parent.width
                height: 44
                text: "Copy link"
                textColor: LV.Theme.panelBackground01
                onClicked: {
                    root.feedback = "Copy link requested by the content view.";
                    sheet.close();
                }
            }
        }
    }
    Component {
        id: exportView
        Column {
            spacing: LV.Theme.gap16
            Rectangle {
                width: parent.width
                height: 72
                radius: LV.Theme.radiusSm
                color: LV.Theme.panelBackground09
                Column {
                    anchors.centerIn: parent
                    spacing: LV.Theme.gap8
                    LV.Label {
                        style: header2
                        text: "Project overview.png"
                    }
                    LV.Label {
                        text: "2048 × 1536 px · Ready to export"
                    }
                }
            }
            Repeater {
                model: [
                    {
                        label: "Format",
                        value: "PNG"
                    },
                    {
                        label: "Scale",
                        value: "2×"
                    },
                    {
                        label: "Background",
                        value: "Transparent"
                    }
                ]
                Rectangle {
                    required property var modelData
                    width: parent.width
                    height: 36
                    radius: LV.Theme.radiusSm
                    color: LV.Theme.panelBackground09
                    LV.Label {
                        x: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.modelData.label
                    }
                    LV.Label {
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.modelData.value
                    }
                }
            }
            LV.LabelButton {
                width: parent.width
                height: 44
                text: "Export image"
                textColor: LV.Theme.panelBackground01
                onClicked: {
                    root.feedback = "Export requested from the same Component on mobile and desktop.";
                    sheet.close();
                }
            }
        }
    }
    Component {
        id: folderView
        Item {
            implicitHeight: 618
            ColumnLayout {
                anchors.fill: parent
                spacing: LV.Theme.gap16
                LV.Label {
                    text: "Society / Projects"
                }
                ListView {
                    objectName: "gallery_sheetFolders"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: LV.Theme.gap8
                    model: ["Brand assets", "Design systems", "Dreamscapes", "Marketing", "References", "Society", "Vincent", "Web", "Campaigns", "Archive", "Shared library", "Templates"]
                    delegate: LV.LabelButton {
                        required property string modelData
                        width: ListView.view.width
                        height: 44
                        text: modelData
                        tone: LV.AbstractButton.Default
                        onClicked: root.feedback = "Selected folder: " + modelData
                    }
                }
                LV.LabelButton {
                    objectName: "gallery_sheetSave"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    Layout.minimumHeight: 44
                    Layout.maximumHeight: 44
                    text: "Save here"
                    textColor: LV.Theme.panelBackground01
                    onClicked: sheet.close()
                }
            }
        }
    }

    LV.Sheet {
        id: sheet
        objectName: "gallery_sheet"
        parent: stage
        presentation: root.desktopExample ? LV.Sheet.Desktop : LV.Sheet.Mobile
        detent: root.exampleIndex === 2 ? LV.Sheet.Large : root.exampleIndex === 1 ? LV.Sheet.Medium : LV.Sheet.Fit
        cornerRadius: root.desktopExample ? 16 : root.deviceRadius
        topSafeInset: root.desktopExample ? 0 : 44
        bottomSafeInset: root.desktopExample ? 0 : 34
        title: root.exampleIndex === 0 ? "Share project" : root.exampleIndex === 2 ? "Save to Society" : root.exampleIndex === 4 ? "Project brief" : "Export image"
        description: root.exampleIndex === 2 ? "Select a destination folder." : "Brand refresh"
        scrollContent: root.exampleIndex !== 2
        contentComponent: root.exampleIndex === 4 ? null : root.exampleIndex === 0 ? shareView : root.exampleIndex === 2 ? folderView : exportView
        Column {
            width: parent.width
            spacing: LV.Theme.gap16
            LV.Label {
                style: header2
                text: "A custom frame, placed directly."
            }
            LV.Label {
                width: parent.width
                text: "This ordinary Column is inline Sheet content. It owns its inputs and actions."
                wrapMode: Text.WordWrap
                sizeToContentHeight: true
            }
            LV.LabelButton {
                text: "Done"
                onClicked: sheet.close()
            }
        }
    }
}

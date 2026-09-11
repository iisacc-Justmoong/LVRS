pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    property int displayState: LV.Card.Automatic
    property string feedback: "Select a card, open its menu, or use its action."
    readonly property url architecture: Qt.resolvedUrl("../assets/cards/architecture.png")
    readonly property url landscape: Qt.resolvedUrl("../assets/cards/landscape.png")
    readonly property url document: Qt.resolvedUrl("../assets/cards/document.png")
    implicitWidth: 1032
    implicitHeight: content.implicitHeight
    height: implicitHeight

    Column {
        id: content
        width: parent.width
        spacing: 24
        LV.Label {
            style: header
            text: "File · full preview with bottom caption"
        }
        Flow {
            width: parent.width
            spacing: 24
            Repeater {
                model: 6
                delegate: LV.Card {
                    required property int index
                    objectName: "gallery_file_" + index
                    size: index % 3
                    detail: index >= 3 ? LV.Card.Detailed : LV.Card.Brief
                    displayState: root.displayState
                    previewSource: index % 3 === 0 ? root.landscape : index % 3 === 1 ? root.document : root.architecture
                    title: index % 3 === 0 ? "Alpine lake.png" : index % 3 === 1 ? "Spaces in balance.pdf" : "Coastal house.png"
                    metadata: index % 3 === 1 ? "PDF · 24 pages · 8.2 MB" : "PNG · 1536 × 1024"
                    details: index % 3 === 0 ? "A still alpine lake below the morning peaks." : index % 3 === 1 ? "An architectural journal exploring light, space and materials." : "Warm limestone, curved walls and a quiet view of the sea."
                    showMenu: true
                    onMenuRequested: root.feedback = "Menu: " + title
                    onClicked: root.feedback = (selected ? "Selected: " : "Deselected: ") + title
                }
            }
        }
        LV.Label {
            style: header
            text: "FilePreview · Architecture / Landscape / Document"
        }
        Flow {
            width: parent.width
            spacing: 24
            Repeater {
                model: [root.architecture, root.landscape, root.document]
                delegate: LV.Card {
                    required property url modelData
                    type: LV.Card.FilePreview
                    previewSource: modelData
                    width: 320
                    height: 213.333
                }
            }
        }
        LV.Label {
            style: header
            text: "Folder / Project / Device / Model / Member / Link"
        }
        Flow {
            width: parent.width
            spacing: 24
            Repeater {
                model: [
                    {
                        type: LV.Card.Folder,
                        title: "Brand assets",
                        description: "Everything for the next release",
                        status: "Shared",
                        summary: "24 items · 1.8 GB",
                        footnote: "Updated today",
                        rows: [
                            {
                                label: "Guidelines",
                                value: "12 files"
                            },
                            {
                                label: "Photography",
                                value: "8 files"
                            },
                            {
                                label: "Templates",
                                value: "4 files"
                            }
                        ]
                    },
                    {
                        type: LV.Card.Project,
                        title: "Website launch",
                        description: "Marketing · Q3 release",
                        status: "In progress",
                        summary: "3 collaborators · Due Sep 24",
                        footnote: "75% complete",
                        progressText: "6 of 8",
                        progress: 0.75,
                        rows: [
                            {
                                label: "Next milestone",
                                value: "Review"
                            }
                        ]
                    },
                    {
                        type: LV.Card.Device,
                        title: "Studio Mac",
                        description: "macOS · Desktop host",
                        status: "Online",
                        summary: "Last synced just now",
                        footnote: "384 GB available",
                        progressText: "128 / 512 GB",
                        progress: 0.25,
                        rows: [
                            {
                                label: "Connection",
                                value: "Local network"
                            }
                        ]
                    },
                    {
                        type: LV.Card.Model,
                        title: "Studio Portrait",
                        description: "Image generation · Checkpoint",
                        status: "Installed",
                        summary: "Ready for Dreamscapes",
                        footnote: "6.4 GB on device",
                        rows: [
                            {
                                label: "Architecture",
                                value: "SDXL"
                            },
                            {
                                label: "Format",
                                value: "Safetensors"
                            },
                            {
                                label: "Precision",
                                value: "FP16"
                            }
                        ]
                    },
                    {
                        type: LV.Card.Member,
                        title: "Maya Chen",
                        description: "Product designer",
                        status: "Available",
                        summary: "Seoul · UTC+9",
                        footnote: "Joined Sep 2026",
                        rows: [
                            {
                                label: "Workspace role",
                                value: "Editor"
                            },
                            {
                                label: "Projects",
                                value: "3 active"
                            },
                            {
                                label: "Team",
                                value: "Design"
                            }
                        ]
                    },
                    {
                        type: LV.Card.Link,
                        title: "iisacc — Creative tools",
                        description: "A connected suite for creative work.",
                        status: "Saved",
                        footnote: "Saved today",
                        domain: "iisacc.com",
                        previewTitle: "Tools for independent creators"
                    }
                ]
                delegate: LV.Card {
                    required property var modelData
                    type: modelData.type
                    displayState: root.displayState
                    title: modelData.title
                    description: modelData.description
                    statusText: modelData.status
                    summary: modelData.summary || ""
                    footnote: modelData.footnote
                    rows: modelData.rows || []
                    progress: modelData.progress || 0
                    progressText: modelData.progressText || ""
                    domain: modelData.domain || ""
                    previewTitle: modelData.previewTitle || ""
                    onMenuRequested: root.feedback = "Menu: " + title
                    onActionTriggered: root.feedback = actionText + ": " + title
                    onClicked: root.feedback = (selected ? "Selected: " : "Deselected: ") + title
                }
            }
        }
        LV.Label {
            style: body
            text: root.feedback
            width: parent.width
        }
    }
}

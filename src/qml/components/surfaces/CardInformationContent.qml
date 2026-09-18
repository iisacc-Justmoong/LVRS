import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: root
    required property var card
    implicitHeight: layout.implicitHeight + card.contentInset * 2

    component DetailRow: Item {
        id: row
        property string label: ""
        property string value: ""
        property url iconSource: ""
        implicitHeight: Theme.scaleMetric(22)
        Image {
            id: rowIcon
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: visible ? Theme.iconSm : 0
            height: Theme.iconSm
            visible: row.iconSource.toString().length > 0
            source: row.iconSource
        }
        Label {
            id: rowValue
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, parent.width * 0.5)
            height: Theme.textDescriptionLineHeight
            style: description
            text: row.value
            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignRight
            maximumLineCount: 1
        }
        Label {
            anchors.left: rowIcon.right
            anchors.leftMargin: rowIcon.visible ? Theme.gap6 : 0
            anchors.right: rowValue.left
            anchors.rightMargin: Theme.gap6
            anchors.verticalCenter: parent.verticalCenter
            height: Theme.textBodyLineHeight
            style: body
            text: row.label
            textFormat: Text.PlainText
            maximumLineCount: 1
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: root.card.contentInset
        spacing: Theme.gap10

        RowLayout {
            objectName: "card_header"
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.scaleMetric(36)
            Layout.minimumHeight: Theme.scaleMetric(36)
            Layout.maximumHeight: Theme.scaleMetric(36)
            spacing: Theme.gap8
            Rectangle {
                Layout.preferredWidth: Theme.scaleMetric(36)
                Layout.preferredHeight: Theme.scaleMetric(36)
                radius: Theme.scaleMetric(6)
                color: Theme.panelBackground03
                Image {
                    anchors.centerIn: parent
                    width: Theme.iconSm
                    height: width
                    source: root.card.resolvedIconSource
                    visible: !iconLoader.active
                }
                Loader {
                    id: iconLoader
                    anchors.centerIn: parent
                    width: Theme.iconSm
                    height: width
                    active: root.card.iconComponent !== null
                    sourceComponent: root.card.iconComponent
                }
            }
            Image {
                objectName: "card_selectionMarker"
                Layout.preferredWidth: Theme.iconSm
                Layout.preferredHeight: Theme.iconSm
                visible: root.card.effectiveSelected
                source: root.card.cardIcon("check")
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                objectName: "card_status"
                Layout.maximumWidth: Math.max(0, layout.width - Theme.scaleMetric(root.card.effectiveSelected ? 104 : 78))
                Layout.preferredHeight: Theme.textDescriptionLineHeight
                style: description
                text: root.card.statusText
                color: root.card.statusColor
                visible: root.card.showStatus && text.length > 0
                textFormat: Text.PlainText
                maximumLineCount: 1
            }
            IconButton {
                objectName: "card_menu"
                visible: root.card.showMenu
                enabled: root.card.effectiveEnabled
                tone: AbstractButton.Borderless
                iconSource: root.card.cardIcon("more")
                Accessible.name: root.card.menuAccessibleName
                onClicked: root.card.menuRequested()
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.gap12

            Rectangle {
                visible: root.card.type === Card.Link
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.scaleMetric(61)
                radius: Theme.scaleMetric(6)
                color: Theme.panelBackground03
                Column {
                    anchors.fill: parent
                    anchors.margins: Theme.gap12
                    spacing: Theme.gap6
                    Row {
                        width: parent.width
                        height: Theme.iconSm
                        spacing: Theme.gap6
                        Image {
                            width: Theme.iconSm
                            height: width
                            source: root.card.cardIcon("globe")
                        }
                        Label {
                            width: Math.max(0, parent.width - Theme.iconSm - Theme.gap6)
                            anchors.verticalCenter: parent.verticalCenter
                            height: Theme.textDescriptionLineHeight
                            text: root.card.domain
                            style: description
                            textFormat: Text.PlainText
                            maximumLineCount: 1
                        }
                    }
                    Label {
                        width: parent.width
                        height: Theme.textBodyLineHeight
                        text: root.card.previewTitle
                        style: body
                        textFormat: Text.PlainText
                        maximumLineCount: 1
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.gap4
                Label {
                    objectName: "card_title"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.textHeader2LineHeight
                    text: root.card.title
                    style: header2
                    textFormat: Text.PlainText
                    maximumLineCount: 1
                }
                Label {
                    objectName: "card_description"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.textDescriptionLineHeight
                    visible: root.card.showDescription && text.length > 0
                    text: root.card.description
                    style: description
                    textFormat: Text.PlainText
                    maximumLineCount: 1
                }
            }
            Column {
                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                visible: root.card.type !== Card.Link && (root.card.progressCard || metrics.count > 0)
                spacing: root.card.progressCard ? Theme.gap12 : Theme.gap2
                Column {
                    width: parent.width
                    visible: root.card.progressCard
                    spacing: Theme.gap8
                    DetailRow {
                        width: parent.width
                        height: implicitHeight
                        label: root.card.progressLabel
                        value: root.card.progressText
                    }
                    ProgressBar {
                        objectName: "card_progress"
                        width: parent.width
                        height: Theme.scaleMetric(4)
                        largeHeight: Theme.scaleMetric(4)
                        maximumValue: 1
                        currentValue: root.card.progress
                        fillColor: root.card.progressColor
                        trackColor: Theme.panelBackground12
                        cornerRadius: Theme.scaleMetric(2)
                    }
                }
                Repeater {
                    id: metrics
                    model: root.card.rows
                    delegate: DetailRow {
                        required property var modelData
                        required property int index
                        objectName: "card_row_" + index
                        width: parent.width
                        height: implicitHeight
                        label: modelData.label || ""
                        value: modelData.value || ""
                        iconSource: modelData.iconSource || (modelData.iconName ? Theme.iconPath(modelData.iconName) : root.card.type === Card.Folder ? root.card.cardIcon(["folder", "image", "text"][index] || "text") : "")
                    }
                }
            }
            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 1
            }
            Label {
                objectName: "card_summary"
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.textDescriptionLineHeight
                visible: root.card.type !== Card.Link && text.length > 0
                text: root.card.summary
                style: description
                textFormat: Text.PlainText
                maximumLineCount: 1
            }
        }

        RowLayout {
            objectName: "card_footer"
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.scaleMetric(22)
            Layout.minimumHeight: Theme.scaleMetric(22)
            Layout.maximumHeight: Theme.scaleMetric(22)
            spacing: Theme.gap8
            Label {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.textDescriptionLineHeight
                text: root.card.footnote
                style: description
                textFormat: Text.PlainText
                maximumLineCount: 1
            }
            LabelButton {
                objectName: "card_action"
                Layout.maximumWidth: layout.width * 0.5
                visible: root.card.showAction
                enabled: root.card.effectiveEnabled
                tone: AbstractButton.Default
                text: root.card.actionText
                onClicked: root.card.actionTriggered()
            }
        }
    }
}

import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root

    property string bootstrapTitle: "Android Hello"
    property string bootstrapSubtitle: "LVRS Example"
    property string bootstrapMessage: "This app verifies LVRS Android bootstrap."
    property string platformLabel: "Android"

    width: 780
    height: 540
    visible: true
    title: bootstrapTitle
    subtitle: bootstrapSubtitle
    pageRoutes: [
        { path: "/", component: helloRoute }
    ]

    Component {
        id: helloRoute

        Item {
            anchors.fill: parent

            LV.AppCard {
                anchors.centerIn: parent
                width: Math.min(parent.width * 0.82, 560)
                title: platformLabel + " Demo"
                subtitle: "Single route host"

                ColumnLayout {
                    width: parent.width - (LV.Theme.gap24 * 2)
                    spacing: LV.Theme.gap8

                    LV.Label {
                        Layout.fillWidth: true
                        style: header
                        text: "Hello World!"
                    }

                    LV.Label {
                        Layout.fillWidth: true
                        style: body
                        wrapMode: Text.WordWrap
                        text: bootstrapMessage
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root

    property string bootstrapTitle: "iOS Hello"
    property string bootstrapSubtitle: "LVRS Example"
    property string bootstrapMessage: "This app verifies LVRS iOS bootstrap."
    property string platformLabel: "iOS"
    readonly property rect safeAreaViewport: mobileSystemSafeAreaBounds
    readonly property real heroHeight: mobileSystemSafeTopInset + LV.Theme.scaleMetric(108)
    readonly property real viewportSideInset: LV.Theme.gap16
    property bool exampleContractReady: mobileSystemSafeAreaResolved
        && safeAreaViewport.width >= 0
        && safeAreaViewport.height >= 0

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

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#091621" }
                    GradientStop { position: 0.46; color: "#154767" }
                    GradientStop { position: 1.0; color: "#f3efe4" }
                }
            }

            Rectangle {
                x: 0
                y: 0
                width: parent.width
                height: root.heroHeight
                color: "#0f4b6e"
                opacity: 0.94
            }

            Column {
                x: root.safeAreaViewport.x + root.viewportSideInset
                y: root.mobileSystemSafeTopInset + LV.Theme.gap12
                width: Math.max(1, root.safeAreaViewport.width - (root.viewportSideInset * 2))
                spacing: LV.Theme.gap6

                LV.Label {
                    width: parent.width
                    style: caption
                    color: "#d7ecff"
                    text: "Full-bleed iOS root"
                }

                LV.Label {
                    width: parent.width
                    style: title2
                    color: "#ffffff"
                    wrapMode: Text.WordWrap
                    text: platformLabel + " status-bar and notch area are app-controlled."
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: "#d7ecff"
                    wrapMode: Text.WordWrap
                    text: "top=" + Math.round(root.mobileSystemSafeTopInset)
                        + " left=" + Math.round(root.mobileSystemSafeLeftInset)
                        + " right=" + Math.round(root.mobileSystemSafeRightInset)
                        + " bottom=" + Math.round(root.mobileSystemSafeBottomInset)
                }
            }

            LV.AppCard {
                x: root.safeAreaViewport.x + root.viewportSideInset
                y: root.heroHeight - LV.Theme.gap8
                width: Math.max(1, Math.min(root.safeAreaViewport.width - (root.viewportSideInset * 2), 560))
                title: platformLabel + " Demo"
                subtitle: "Single route host with explicit safe-area reservation"

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

            Rectangle {
                x: root.safeAreaViewport.x + root.viewportSideInset
                y: root.safeAreaViewport.y + root.safeAreaViewport.height - height - LV.Theme.gap12
                width: Math.max(1, Math.min(root.safeAreaViewport.width - (root.viewportSideInset * 2), 320))
                height: 42
                radius: LV.Theme.radiusLg
                color: "#1b2730"
                opacity: 0.92

                LV.Label {
                    anchors.centerIn: parent
                    style: caption
                    color: "#f8fafc"
                    text: "Home indicator reserved explicitly"
                }
            }
        }
    }
}

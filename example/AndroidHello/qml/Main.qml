import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root

    property string bootstrapTitle: "Android Hello"
    property string bootstrapSubtitle: "LVRS Example"
    property string bootstrapMessage: "This app verifies LVRS Android bootstrap."
    property string platformLabel: "Android"
    readonly property rect safeAreaViewport: mobileSystemSafeAreaBounds
    readonly property real heroHeight: mobileSystemSafeTopInset + LV.Theme.scaleMetric(92)
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
                    GradientStop { position: 0.0; color: "#0b1b12" }
                    GradientStop { position: 0.42; color: "#1c5a39" }
                    GradientStop { position: 1.0; color: "#eef4e7" }
                }
            }

            Rectangle {
                x: 0
                y: 0
                width: parent.width
                height: root.heroHeight
                color: "#174c31"
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
                    color: "#d7ffe5"
                    text: "Edge-to-edge Android root"
                }

                LV.Label {
                    width: parent.width
                    style: title2
                    color: "#ffffff"
                    wrapMode: Text.WordWrap
                    text: platformLabel + " root chooses its own inset policy explicitly."
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: "#d7ffe5"
                    wrapMode: Text.WordWrap
                    text: "safe area " + Math.round(root.safeAreaViewport.width)
                        + " x " + Math.round(root.safeAreaViewport.height)
                }
            }

            LV.AppCard {
                x: root.safeAreaViewport.x + root.viewportSideInset
                y: root.heroHeight - LV.Theme.gap8
                width: Math.max(1, Math.min(root.safeAreaViewport.width - (root.viewportSideInset * 2), 560))
                title: platformLabel + " Demo"
                subtitle: "Explicit safe-area viewport inside a full-bleed host"

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
                width: Math.max(1, Math.min(root.safeAreaViewport.width - (root.viewportSideInset * 2), 340))
                height: 42
                radius: LV.Theme.radiusLg
                color: "#102119"
                opacity: 0.92

                LV.Label {
                    anchors.centerIn: parent
                    style: caption
                    color: "#f4fff7"
                    text: "Insets come from the platform, not automatic shell padding"
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Item {
    id: root

    property bool open: false
    property string title: "Alert Dialog"
    property string message: "It can have 2 or 3 actions depending on your needs."
    // 0 = auto(legacy by text presence), 2 or 3 = explicit Figma variant selection.
    property int buttonCount: 0
    property string primaryText: "Button"
    property string secondaryText: "Button"
    property string tertiaryText: ""
    property bool primaryEnabled: true
    property bool secondaryEnabled: true
    property bool tertiaryEnabled: true
    property bool dismissOnBackground: false
    property bool useOverlayLayer: true
    property int maxWidth: Theme.dialogMaxWidth
    property int minWidth: Theme.dialogMinWidth
    property color backdropColor: Theme.overlayBackdrop
    property color cardBackgroundColor: root.useVerticalActionLayout ? Theme.panelBackground07 : Theme.panelBackground08
    property color appIconBackgroundColor: "#C9D4DB"
    property color appIconFrameColor: "#E8F0F5"
    property color appIconInnerColor: "#D8E0E6"

    readonly property int preferredWidth: 328
    readonly property int sidePadding: Theme.gap24
    readonly property int resolvedButtonCount: {
        if (root.buttonCount === 2 || root.buttonCount === 3)
            return root.buttonCount
        if (root.tertiaryText.length > 0)
            return 3
        if (root.secondaryText.length > 0)
            return 2
        return 1
    }
    readonly property string resolvedPrimaryText: root.primaryText.length > 0 ? root.primaryText : "Button"
    readonly property string resolvedSecondaryText: root.secondaryText.length > 0 ? root.secondaryText : "Button"
    readonly property string resolvedTertiaryText: root.tertiaryText.length > 0 ? root.tertiaryText : "Button"
    readonly property bool hasSecondaryAction: root.resolvedButtonCount >= 2
    readonly property bool hasTertiaryAction: root.resolvedButtonCount >= 3
    readonly property bool useVerticalActionLayout: root.hasTertiaryAction

    signal primaryClicked()
    signal secondaryClicked()
    signal tertiaryClicked()
    signal dismissed()

    visible: open
    enabled: open
    anchors.fill: parent
    z: 1000

    property Item _fallbackParent: null

    function refreshLayerParent() {
        const overlayParent = Controls.Overlay.overlay
        const targetParent = useOverlayLayer && overlayParent ? overlayParent : _fallbackParent
        if (targetParent && parent !== targetParent)
            parent = targetParent
    }

    onOpenChanged: {
        if (open)
            refreshLayerParent()
    }

    onParentChanged: {
        if (parent && parent !== Controls.Overlay.overlay)
            _fallbackParent = parent
    }

    Rectangle {
        anchors.fill: parent
        color: root.backdropColor
        visible: root.open

        MouseArea {
            anchors.fill: parent
            enabled: root.dismissOnBackground
            onClicked: {
                root.open = false
                root.dismissed()
            }
        }
    }

    Rectangle {
        id: alertCard
        width: Math.min(root.maxWidth,
                        Math.max(root.minWidth,
                                 Math.min(root.preferredWidth,
                                          root.width - (root.sidePadding * 2))))
        radius: Theme.radiusLg
        color: root.cardBackgroundColor
        antialiasing: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        Column {
            id: contentColumn
            anchors.fill: parent
            spacing: Theme.gap8
            topPadding: 32

            Item {
                width: parent.width
                height: 64

                Rectangle {
                    width: 48
                    height: 48
                    radius: 10
                    anchors.centerIn: parent
                    color: root.appIconBackgroundColor
                    border.width: 4
                    border.color: root.appIconFrameColor

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: 8
                        color: root.appIconInnerColor
                        opacity: 0.42
                    }
                }
            }

            Item {
                width: parent.width
                implicitHeight: textColumn.implicitHeight

                Column {
                    id: textColumn
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - (Theme.gap24 * 2)
                    spacing: Theme.gap12

                    Label {
                        style: title2
                        text: root.title
                        visible: root.title.length > 0
                        color: Theme.textPrimary
                        wrapMode: Text.WordWrap
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        style: body
                        text: root.message
                        visible: root.message.length > 0
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Item {
                width: parent.width
                readonly property real actionContentHeight: root.useVerticalActionLayout
                    ? verticalActions.implicitHeight
                    : root.hasSecondaryAction
                        ? horizontalActions.implicitHeight
                        : singleActionButton.implicitHeight
                implicitHeight: actionContentHeight + (Theme.gap24 * 2)

                Column {
                    id: verticalActions
                    visible: root.useVerticalActionLayout
                    x: Theme.gap24
                    y: Theme.gap24
                    width: parent.width - (Theme.gap24 * 2)
                    spacing: Theme.gap12

                    AlertButton {
                        visible: root.resolvedPrimaryText.length > 0
                        width: parent.width
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }

                    AlertButton {
                        visible: root.hasSecondaryAction
                        width: parent.width
                        text: root.resolvedSecondaryText
                        tone: AbstractButton.Default
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }

                    AlertButton {
                        visible: root.hasTertiaryAction
                        width: parent.width
                        text: root.resolvedTertiaryText
                        tone: AbstractButton.Default
                        enabled: root.tertiaryEnabled
                        onClicked: root.tertiaryClicked()
                    }
                }

                Row {
                    id: horizontalActions
                    visible: !root.useVerticalActionLayout && root.hasSecondaryAction
                    x: Theme.gap24
                    y: Theme.gap24
                    width: parent.width - (Theme.gap24 * 2)
                    spacing: Theme.gap12
                    readonly property real buttonWidth: (width - spacing) / 2

                    AlertButton {
                        width: horizontalActions.buttonWidth
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }

                    AlertButton {
                        width: horizontalActions.buttonWidth
                        visible: root.hasSecondaryAction
                        text: root.resolvedSecondaryText
                        tone: AbstractButton.Default
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }
                }

                AlertButton {
                    id: singleActionButton
                    visible: !root.useVerticalActionLayout && !root.hasSecondaryAction && root.resolvedPrimaryText.length > 0
                    x: Theme.gap24
                    y: Theme.gap24
                    width: parent.width - (Theme.gap24 * 2)
                    text: root.resolvedPrimaryText
                    tone: AbstractButton.Primary
                    enabled: root.primaryEnabled
                    onClicked: root.primaryClicked()
                }
            }
        }
    }

    QtObject {
        Component.onCompleted: {
            root._fallbackParent = root.parent
            root.refreshLayerParent()
            Qt.callLater(root.refreshLayerParent)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Alert {
//     open: true
//     buttonCount: 3
//     title: "Alert Dialog"
//     message: "It can have 2 or 3 actions depending on your needs."
//     primaryText: "Button"
//     secondaryText: "Button"
//     tertiaryText: "Button"
// }

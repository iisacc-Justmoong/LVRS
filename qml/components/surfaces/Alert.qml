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
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int cardCornerRadius: Theme.radiusLg
    property color backdropColor: Theme.overlayBackdrop
    property color cardBackgroundColor: Theme.panelBackground07
    property url appIconSource: "qrc:/qt/qml/LVRS/resources/images/alertAppIcon.png"
    property int appIconSize: Theme.scaleMetric(64)
    property color appIconBackgroundColor: "#C9D4DB"
    property color appIconFrameColor: "#E8F0F5"
    property color appIconInnerColor: "#D8E0E6"
    readonly property real actionButtonVerticalPadding: Theme.scaleRealMetric(4.5)
    readonly property real actionButtonHeight: Theme.textBodyLineHeight
                                               + (actionButtonVerticalPadding * 2)

    readonly property int preferredWidth: Theme.scaleMetric(328)
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
    readonly property real resolvedCardCornerRadius: shapeStyle === shapeCylinder
        ? Math.max(0, Math.min(alertCard.width, alertCard.height) / 2)
        : cardCornerRadius

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
        objectName: "alertCard"
        width: Math.min(root.maxWidth,
                        Math.max(root.minWidth,
                                 Math.min(root.preferredWidth,
                                          root.width - (root.sidePadding * 2))))
        height: contentColumn.topPadding
                + appIconSection.height
                + textSection.implicitHeight
                + actionSection.implicitHeight
                + (contentColumn.spacing * 2)
        radius: root.resolvedCardCornerRadius
        color: root.cardBackgroundColor
        antialiasing: true
        clip: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        Column {
            id: contentColumn
            objectName: "alertContentColumn"
            anchors.fill: parent
            spacing: Theme.gap8
            topPadding: Theme.scaleMetric(32)

            Item {
                id: appIconSection
                width: parent.width
                height: root.appIconSize

                Image {
                    id: appIconImage
                    objectName: "alertAppIcon"
                    width: root.appIconSize
                    height: root.appIconSize
                    anchors.centerIn: parent
                    source: root.appIconSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: RenderQuality.mipmapEnabled
                    visible: root.appIconSource.toString().length > 0
                }

                Rectangle {
                    width: Theme.scaleMetric(48)
                    height: Theme.scaleMetric(48)
                    radius: Theme.scaleMetric(10)
                    anchors.centerIn: parent
                    color: root.appIconBackgroundColor
                    border.width: Theme.scaleRealMetric(4)
                    border.color: root.appIconFrameColor
                    visible: !appIconImage.visible

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: Theme.scaleMetric(2)
                        radius: Theme.scaleMetric(8)
                        color: root.appIconInnerColor
                        opacity: 0.42
                    }
                }
            }

            Item {
                id: textSection
                width: parent.width
                implicitHeight: titleLineBox.implicitHeight
                                + messageLineBox.implicitHeight
                                + (titleLineBox.visible && messageLineBox.visible ? Theme.gap12 : 0)

                Column {
                    id: textColumn
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - (Theme.gap24 * 2)
                    spacing: Theme.gap12

                    Item {
                        id: titleLineBox
                        visible: root.title.length > 0
                        width: parent.width
                        implicitHeight: visible
                            ? Math.max(1, titleLabel.lineCount) * titleLabel.styleLineHeight
                            : 0
                        height: implicitHeight

                        Label {
                            id: titleLabel
                            objectName: "alertTitle"
                            style: title2
                            text: root.title
                            color: Theme.bodyColor
                            wrapMode: Text.WordWrap
                            elide: Text.ElideNone
                            sizeToContentHeight: true
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    Item {
                        id: messageLineBox
                        visible: root.message.length > 0
                        width: parent.width
                        implicitHeight: visible
                            ? Math.max(1, messageLabel.lineCount) * messageLabel.styleLineHeight
                            : 0
                        height: implicitHeight

                        Label {
                            id: messageLabel
                            objectName: "alertMessage"
                            style: body
                            text: root.message
                            color: Theme.bodyColor
                            wrapMode: Text.WordWrap
                            elide: Text.ElideNone
                            sizeToContentHeight: true
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }

            Item {
                id: actionSection
                width: parent.width
                readonly property real actionContentHeight: root.useVerticalActionLayout
                    ? verticalActions.implicitHeight
                    : root.hasSecondaryAction
                        ? horizontalActions.implicitHeight
                        : singleActionButton.implicitHeight
                implicitHeight: actionContentHeight + (Theme.gap24 * 2)

                Column {
                    id: verticalActions
                    objectName: "alertVerticalActions"
                    visible: root.useVerticalActionLayout
                    x: Theme.gap24
                    y: Theme.gap24
                    width: parent.width - (Theme.gap24 * 2)
                    spacing: Theme.gap12

                    AlertButton {
                        objectName: "alertPrimaryVertical"
                        visible: root.resolvedPrimaryText.length > 0
                        width: parent.width
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        verticalPadding: root.actionButtonVerticalPadding
                        implicitHeight: root.actionButtonHeight
                        height: root.actionButtonHeight
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }

                    AlertButton {
                        objectName: "alertSecondaryVertical"
                        visible: root.hasSecondaryAction
                        width: parent.width
                        text: root.resolvedSecondaryText
                        tone: AbstractButton.Default
                        verticalPadding: root.actionButtonVerticalPadding
                        implicitHeight: root.actionButtonHeight
                        height: root.actionButtonHeight
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }

                    AlertButton {
                        objectName: "alertTertiaryVertical"
                        visible: root.hasTertiaryAction
                        width: parent.width
                        text: root.resolvedTertiaryText
                        tone: AbstractButton.Default
                        verticalPadding: root.actionButtonVerticalPadding
                        implicitHeight: root.actionButtonHeight
                        height: root.actionButtonHeight
                        enabled: root.tertiaryEnabled
                        onClicked: root.tertiaryClicked()
                    }
                }

                Row {
                    id: horizontalActions
                    objectName: "alertHorizontalActions"
                    visible: !root.useVerticalActionLayout && root.hasSecondaryAction
                    x: Theme.gap24
                    y: Theme.gap24
                    width: parent.width - (Theme.gap24 * 2)
                    spacing: Theme.gap12
                    readonly property real buttonWidth: (width - spacing) / 2

                    AlertButton {
                        objectName: "alertPrimaryHorizontal"
                        width: horizontalActions.buttonWidth
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        verticalPadding: root.actionButtonVerticalPadding
                        implicitHeight: root.actionButtonHeight
                        height: root.actionButtonHeight
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }

                    AlertButton {
                        objectName: "alertSecondaryHorizontal"
                        width: horizontalActions.buttonWidth
                        visible: root.hasSecondaryAction
                        text: root.resolvedSecondaryText
                        tone: AbstractButton.Default
                        verticalPadding: root.actionButtonVerticalPadding
                        implicitHeight: root.actionButtonHeight
                        height: root.actionButtonHeight
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }
                }

                AlertButton {
                    id: singleActionButton
                    objectName: "alertPrimarySingle"
                    visible: !root.useVerticalActionLayout && !root.hasSecondaryAction && root.resolvedPrimaryText.length > 0
                    x: Theme.gap24
                    y: Theme.gap24
                    width: parent.width - (Theme.gap24 * 2)
                    text: root.resolvedPrimaryText
                    tone: AbstractButton.Primary
                    verticalPadding: root.actionButtonVerticalPadding
                    implicitHeight: root.actionButtonHeight
                    height: root.actionButtonHeight
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

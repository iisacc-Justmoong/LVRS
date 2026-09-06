import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Effects
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
    property bool secondaryDestructive: hasTertiaryAction
    property bool dismissOnBackground: false
    property bool useOverlayLayer: true
    property int maxWidth: Theme.scaleMetric(500)
    property int minWidth: Theme.dialogMinWidth
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int cardCornerRadius: Theme.scaleMetric(36)
    property color backdropColor: Theme.overlayBackdrop
    property color cardBackgroundColor: Theme.alertGlassTint
    property bool glassEnabled: true
    property real glassBlurRadius: Theme.scaleRealMetric(28)
    // In ApplicationWindow the content and Controls.Overlay are siblings.
    // Plain windows/custom hosts can supply a sibling background item.
    property Item backdropSource: Controls.ApplicationWindow.contentItem
    readonly property Item resolvedBackdropSource: {
        const source = root.backdropSource
        if (!source || source === root)
            return null
        // Capturing an ancestor would include the Alert and create feedback.
        for (let ancestor = root.parent; ancestor; ancestor = ancestor.parent) {
            if (ancestor === source)
                return null
        }
        return source
    }
    readonly property bool glassActive: root.open && root.glassEnabled
                                        && root.resolvedBackdropSource !== null

    property bool showIcon: true
    property url appIconSource: hasTertiaryAction
        ? "qrc:/qt/qml/LVRS/resources/images/alert-file-text.svg"
        : "qrc:/qt/qml/LVRS/resources/images/alert-adjustments.svg"
    property int appIconSize: Theme.scaleMetric(hasTertiaryAction ? 56 : 64)
    property int iconFrameSize: Math.max(Theme.scaleMetric(86), appIconSize)
    property color appIconBackgroundColor: Theme.alertIconSurface
    property color appIconFrameColor: Theme.alertIconBorder
    property color appIconInnerColor: Theme.alertTitleColor
    readonly property real actionButtonVerticalPadding: 0
    readonly property real actionButtonHeight: Theme.scaleMetric(56)
    readonly property real cancelButtonHeight: Theme.scaleMetric(44)

    readonly property int preferredWidth: Theme.scaleMetric(500)
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
        : Math.min(cardCornerRadius, Math.min(alertCard.width, alertCard.height) * 0.4)

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
    onUseOverlayLayerChanged: refreshLayerParent()
    onParentChanged: {
        if (parent && parent !== Controls.Overlay.overlay)
            _fallbackParent = parent
    }

    Rectangle {
        objectName: "alertBackdrop"
        anchors.fill: parent
        color: root.backdropColor

        MouseArea {
            anchors.fill: parent
            // Always consume background input, even when dismissal is disabled.
            acceptedButtons: Qt.AllButtons
            onClicked: {
                if (root.dismissOnBackground) {
                    root.open = false
                    root.dismissed()
                }
            }
        }
    }

    Rectangle {
        id: shadowShape
        width: alertCard.width
        height: alertCard.height
        radius: root.resolvedCardCornerRadius
        color: Qt.rgba(0, 0, 0, 0.28)
        visible: false
    }

    MultiEffect {
        objectName: "alertShadow"
        x: alertCard.x
        y: alertCard.y + Theme.scaleMetric(16)
        width: alertCard.width
        height: alertCard.height
        source: shadowShape
        blurEnabled: true
        blurMax: Math.min(64, Theme.scaleMetric(32))
        blur: 1
        autoPaddingEnabled: true
        visible: root.open
    }

    ShaderEffectSource {
        id: glassCapture
        objectName: "alertGlassCapture"
        width: alertCard.width
        height: alertCard.height
        sourceItem: root.glassActive ? root.resolvedBackdropSource : null
        sourceRect: {
            if (!sourceItem)
                return Qt.rect(0, 0, width, height)
            const point = root.mapToItem(sourceItem, alertCard.x, alertCard.y)
            return Qt.rect(point.x, point.y, width, height)
        }
        live: root.glassActive
        hideSource: false
        recursive: false
        visible: false
    }

    Rectangle {
        id: glassMask
        width: alertCard.width
        height: alertCard.height
        radius: root.resolvedCardCornerRadius
        color: "white"
        antialiasing: true
        visible: false
        layer.enabled: root.glassActive
    }

    MultiEffect {
        objectName: "alertGlassEffect"
        anchors.fill: alertCard
        source: glassCapture
        visible: root.glassActive
        blurEnabled: true
        blurMax: Math.max(2, Math.min(64, Math.ceil(root.glassBlurRadius)))
        blur: 1
        autoPaddingEnabled: false
        maskEnabled: true
        maskSource: glassMask
    }

    Rectangle {
        id: alertCard
        objectName: "alertCard"
        width: Math.max(0, Math.min(root.width - root.sidePadding * 2,
                                   root.maxWidth,
                                   Math.max(root.minWidth, root.preferredWidth)))
        height: contentColumn.implicitHeight + divider.height + actionSection.implicitHeight
        radius: root.resolvedCardCornerRadius
        color: root.cardBackgroundColor
        border.width: Theme.scaleRealMetric(1)
        border.color: Theme.alertGlassEdge
        antialiasing: true
        clip: true
        anchors.centerIn: parent

        // A blank area inside the dialog must not dismiss it.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        Column {
            width: parent.width

            Column {
                id: contentColumn
                objectName: "alertContentColumn"
                width: parent.width
                spacing: Theme.scaleMetric(28)
                topPadding: Theme.scaleMetric(46)
                bottomPadding: Theme.scaleMetric(36)

                Item {
                    id: appIconSection
                    width: parent.width
                    height: root.showIcon ? root.iconFrameSize : 0
                    visible: root.showIcon

                    Rectangle {
                        objectName: "alertIconFrame"
                        width: root.iconFrameSize
                        height: root.iconFrameSize
                        anchors.centerIn: parent
                        radius: Theme.scaleMetric(28)
                        color: root.appIconBackgroundColor
                        border.width: Theme.scaleRealMetric(1)
                        border.color: root.appIconFrameColor
                        antialiasing: true

                        Image {
                            id: appIconImage
                            objectName: "alertAppIcon"
                            width: root.appIconSize
                            height: root.appIconSize
                            anchors.centerIn: parent
                            source: root.appIconSource
                            sourceSize: Qt.size(width, height)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: RenderQuality.mipmapEnabled
                            visible: root.appIconSource.toString().length > 0
                        }

                        Rectangle {
                            width: Theme.scaleMetric(32)
                            height: Theme.scaleMetric(32)
                            radius: Theme.scaleMetric(8)
                            anchors.centerIn: parent
                            color: root.appIconInnerColor
                            opacity: 0.42
                            visible: !appIconImage.visible
                        }
                    }
                }

                Column {
                    id: textSection
                    width: Math.max(0, parent.width - root.sidePadding * 2)
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.gap14

                    Item {
                        id: titleLineBox
                        visible: root.title.length > 0
                        width: parent.width
                        // Preserve the Figma line boxes, using existing Title typography.
                        height: visible ? Math.max(Theme.scaleMetric(34),
                                                   titleLabel.lineCount * titleLabel.styleLineHeight) : 0

                        Label {
                            id: titleLabel
                            objectName: "alertTitle"
                            style: title
                            text: root.title
                            color: Theme.alertTitleColor
                            anchors.fill: parent
                            wrapMode: Text.WordWrap
                            elide: Text.ElideNone
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Item {
                        id: messageLineBox
                        visible: root.message.length > 0
                        width: parent.width
                        height: visible ? Math.max(Theme.scaleMetric(56),
                                                   messageLabel.lineCount * messageLabel.styleLineHeight) : 0

                        Label {
                            id: messageLabel
                            objectName: "alertMessage"
                            style: body
                            text: root.message
                            color: Theme.alertBodyColor
                            anchors.fill: parent
                            wrapMode: Text.WordWrap
                            elide: Text.ElideNone
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            Rectangle {
                id: divider
                objectName: "alertDivider"
                width: parent.width - (root.useVerticalActionLayout ? 0 : root.sidePadding * 2)
                height: Theme.scaleMetric(1)
                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.alertDivider
            }

            Item {
                id: actionSection
                width: parent.width
                readonly property real topPadding: root.useVerticalActionLayout
                    ? Theme.scaleMetric(18) : Theme.scaleMetric(28)
                readonly property real bottomPadding: root.useVerticalActionLayout
                    ? Theme.scaleMetric(18) : Theme.scaleMetric(32)
                implicitHeight: topPadding + bottomPadding
                                + (root.useVerticalActionLayout ? verticalActions.implicitHeight
                                   : root.hasSecondaryAction ? horizontalActions.implicitHeight
                                   : singleActionButton.height)

                Column {
                    id: verticalActions
                    objectName: "alertVerticalActions"
                    visible: root.useVerticalActionLayout
                    x: root.sidePadding
                    y: actionSection.topPadding
                    width: Math.max(0, parent.width - root.sidePadding * 2)
                    spacing: Theme.gap12

                    AlertButton {
                        objectName: "alertPrimaryVertical"
                        width: parent.width
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        dialogStyle: true
                        verticalPadding: root.actionButtonVerticalPadding
                        height: root.actionButtonHeight
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }

                    AlertButton {
                        objectName: "alertSecondaryVertical"
                        width: parent.width
                        text: root.resolvedSecondaryText
                        dialogStyle: true
                        textColor: root.secondaryDestructive ? Theme.danger : Theme.alertTitleColor
                        verticalPadding: root.actionButtonVerticalPadding
                        height: root.actionButtonHeight
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }

                    AlertButton {
                        objectName: "alertTertiaryVertical"
                        width: parent.width
                        text: root.resolvedTertiaryText
                        tone: AbstractButton.Borderless
                        dialogStyle: true
                        verticalPadding: root.actionButtonVerticalPadding
                        height: root.cancelButtonHeight
                        enabled: root.tertiaryEnabled
                        onClicked: root.tertiaryClicked()
                    }
                }

                Row {
                    id: horizontalActions
                    objectName: "alertHorizontalActions"
                    visible: !root.useVerticalActionLayout && root.hasSecondaryAction
                    x: root.sidePadding
                    y: actionSection.topPadding
                    width: Math.max(0, parent.width - root.sidePadding * 2)
                    spacing: Theme.gap14
                    readonly property real buttonWidth: Math.max(0, (width - spacing) / 2)

                    AlertButton {
                        objectName: "alertSecondaryHorizontal"
                        width: horizontalActions.buttonWidth
                        text: root.resolvedSecondaryText
                        dialogStyle: true
                        textColor: root.secondaryDestructive ? Theme.danger : Theme.alertTitleColor
                        horizontalPadding: Theme.gap12
                        verticalPadding: root.actionButtonVerticalPadding
                        height: root.actionButtonHeight
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }

                    AlertButton {
                        objectName: "alertPrimaryHorizontal"
                        width: horizontalActions.buttonWidth
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        dialogStyle: true
                        horizontalPadding: Theme.gap12
                        verticalPadding: root.actionButtonVerticalPadding
                        height: root.actionButtonHeight
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }
                }

                AlertButton {
                    id: singleActionButton
                    objectName: "alertPrimarySingle"
                    visible: !root.useVerticalActionLayout && !root.hasSecondaryAction
                    x: root.sidePadding
                    y: actionSection.topPadding
                    width: Math.max(0, parent.width - root.sidePadding * 2)
                    text: root.resolvedPrimaryText
                    tone: AbstractButton.Primary
                    dialogStyle: true
                    verticalPadding: root.actionButtonVerticalPadding
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
//     title: "Save changes?"
//     message: "You have unsaved changes.\nSave them before closing?"
//     primaryText: "Save changes"
//     secondaryText: "Discard changes"
//     tertiaryText: "Cancel"
// }

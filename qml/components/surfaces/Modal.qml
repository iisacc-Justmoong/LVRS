import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Item {
    id: root

    property bool open: false
    property bool dismissOnBackground: true
    property bool useOverlayLayer: true

    property string title: ""
    property string description: ""
    property string message: ""
    property alias desc: root.description

    property string iconName: ""
    property url iconSource: ""
    property bool showIcon: true
    property int iconSize: Theme.scaleMetric(72)
    property int iconCornerRadius: Theme.scaleMetric(16)
    property color iconBackgroundColor: Theme.panelBackground07
    property color iconBorderColor: Qt.rgba(Theme.strokeSoft.r, Theme.strokeSoft.g, Theme.strokeSoft.b, 0.18)

    // 0 = auto, 1~3 = explicit (values outside range are clamped)
    property int buttonCount: 0
    property string primaryText: "OK"
    property string secondaryText: ""
    property string tertiaryText: ""
    property bool primaryEnabled: true
    property bool secondaryEnabled: true
    property bool tertiaryEnabled: true

    property int maxWidth: Theme.scaleMetric(980)
    property int minWidth: Theme.scaleMetric(520)
    readonly property int preferredWidth: Theme.scaleMetric(760)
    property int sidePadding: Theme.gap24
    property int frameMinHeight: Theme.scaleMetric(210)
    property int verticalOffset: -Theme.gap16

    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int frameCornerRadius: Theme.radiusLg
    readonly property real resolvedFrameCornerRadius: shapeStyle === shapeCylinder
        ? Math.max(0, Math.min(modalFrame.width, modalFrame.height) / 2)
        : frameCornerRadius

    property color backdropColor: Theme.overlayBackdrop
    property color frameColor: Theme.panelBackground08

    readonly property string resolvedIconName: {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        return rawName.trim()
    }
    readonly property url resolvedIconSource: iconSource.toString().length > 0
        ? iconSource
        : resolvedIconName.length > 0
            ? Theme.iconPath(resolvedIconName)
            : ""
    readonly property string resolvedDescription: {
        const primaryDescription = description === undefined || description === null ? "" : String(description)
        if (primaryDescription.length > 0)
            return primaryDescription
        return message === undefined || message === null ? "" : String(message)
    }
    readonly property int resolvedButtonCount: {
        if (buttonCount > 0)
            return Math.max(1, Math.min(3, Math.trunc(buttonCount)))
        if (tertiaryText.length > 0)
            return 3
        if (secondaryText.length > 0)
            return 2
        if (primaryText.length > 0)
            return 1
        return 0
    }
    readonly property bool hasPrimaryAction: resolvedButtonCount >= 1
    readonly property bool hasSecondaryAction: resolvedButtonCount >= 2
    readonly property bool hasTertiaryAction: resolvedButtonCount >= 3
    readonly property string resolvedPrimaryText: primaryText.length > 0 ? primaryText : "OK"
    readonly property string resolvedSecondaryText: secondaryText.length > 0 ? secondaryText : "Cancel"
    readonly property string resolvedTertiaryText: tertiaryText.length > 0 ? tertiaryText : "More"

    signal canceled()
    signal primaryClicked()
    signal secondaryClicked()
    signal tertiaryClicked()

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

    function containsFramePoint(localX, localY) {
        return localX >= modalFrame.x
            && localX <= modalFrame.x + modalFrame.width
            && localY >= modalFrame.y
            && localY <= modalFrame.y + modalFrame.height
    }

    function cancel() {
        if (!open)
            return false
        open = false
        canceled()
        return true
    }

    function handleBackdropClick(localX, localY) {
        if (!open || !dismissOnBackground)
            return false
        if (containsFramePoint(localX, localY))
            return false
        return cancel()
    }

    function actionVisible(index) {
        if (index === 1)
            return hasPrimaryAction
        if (index === 2)
            return hasSecondaryAction
        if (index === 3)
            return hasTertiaryAction
        return false
    }

    function triggerAction(index) {
        if (index === 1) {
            if (!hasPrimaryAction || !primaryEnabled)
                return false
            primaryClicked()
            return true
        }
        if (index === 2) {
            if (!hasSecondaryAction || !secondaryEnabled)
                return false
            secondaryClicked()
            return true
        }
        if (index === 3) {
            if (!hasTertiaryAction || !tertiaryEnabled)
                return false
            tertiaryClicked()
            return true
        }
        return false
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
            enabled: root.dismissOnBackground && root.open
            onClicked: function(mouse) {
                root.handleBackdropClick(mouse.x, mouse.y)
            }
        }
    }

    Rectangle {
        id: modalFrame
        width: Math.min(root.maxWidth,
                        Math.max(root.minWidth,
                                 Math.min(root.preferredWidth,
                                          root.width - (root.sidePadding * 2))))
        height: Math.max(root.frameMinHeight, modalContent.implicitHeight + (Theme.gap20 * 2))
        radius: root.resolvedFrameCornerRadius
        color: root.frameColor
        antialiasing: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: root.verticalOffset

        Column {
            id: modalContent
            anchors.fill: parent
            anchors.margins: Theme.gap20
            spacing: Theme.gap20

            Item {
                id: headerSection
                width: parent.width
                implicitHeight: Math.max(iconContainer.visible ? iconContainer.height : 0,
                                          textColumn.implicitHeight)

                Rectangle {
                    id: iconContainer
                    visible: root.showIcon
                    width: root.iconSize
                    height: root.iconSize
                    radius: root.iconCornerRadius
                    anchors.left: parent.left
                    anchors.top: parent.top
                    color: root.iconBackgroundColor
                        border.width: Theme.scaleRealMetric(1)
                    border.color: root.iconBorderColor
                    antialiasing: true

                    Image {
                        id: iconImage
                        anchors.fill: parent
                        anchors.margins: Theme.scaleMetric(8)
                        source: RenderQuality.resolveTextureSource(root.resolvedIconSource)
                        visible: root.resolvedIconSource.toString().length > 0
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: RenderQuality.mipmapEnabled
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        visible: !iconImage.visible
                        width: Theme.scaleMetric(20)
                        height: Theme.scaleMetric(20)
                        radius: Theme.radiusSm
                        color: Theme.darkGrey10
                        opacity: 0.72
                    }
                }

                Column {
                    id: textColumn
                    anchors.top: parent.top
                    anchors.left: iconContainer.visible ? iconContainer.right : parent.left
                    anchors.leftMargin: iconContainer.visible ? Theme.gap16 : 0
                    anchors.right: parent.right
                    spacing: Theme.gap8

                    Label {
                        id: titleLabel
                        objectName: "modalTitle"
                        style: title2
                        text: root.title
                        visible: text.length > 0
                        color: Theme.textPrimary
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    Label {
                        id: descriptionLabel
                        objectName: "modalDescription"
                        style: body
                        text: root.resolvedDescription
                        visible: text.length > 0
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                        width: parent.width
                        lineHeight: Theme.scaleTextMetric(18)
                        lineHeightMode: Text.FixedHeight
                    }
                }
            }

            Item {
                id: actionSection
                width: parent.width
                implicitHeight: actionRow.implicitHeight
                visible: root.resolvedButtonCount > 0

                Row {
                    id: actionRow
                    objectName: "modalActionRow"
                    anchors.right: parent.right
                    spacing: Theme.gap12

                    AlertButton {
                        id: tertiaryButton
                        objectName: "modalTertiaryButton"
                        visible: root.hasTertiaryAction
                        text: root.resolvedTertiaryText
                        tone: AbstractButton.Default
                        enabled: root.tertiaryEnabled
                        onClicked: root.tertiaryClicked()
                    }

                    AlertButton {
                        id: secondaryButton
                        objectName: "modalSecondaryButton"
                        visible: root.hasSecondaryAction
                        text: root.resolvedSecondaryText
                        tone: AbstractButton.Default
                        enabled: root.secondaryEnabled
                        onClicked: root.secondaryClicked()
                    }

                    AlertButton {
                        id: primaryButton
                        objectName: "modalPrimaryButton"
                        visible: root.hasPrimaryAction
                        text: root.resolvedPrimaryText
                        tone: AbstractButton.Primary
                        enabled: root.primaryEnabled
                        onClicked: root.primaryClicked()
                    }
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
// LV.Modal {
//     open: true
//     iconName: "projectStructure"
//     title: "Unlock iPhone 15 Pro Max to Continue"
//     description: "Xcode cannot launch because the device is locked."
//     buttonCount: 2
//     primaryText: "Cancel Running"
//     secondaryText: "Later"
// }

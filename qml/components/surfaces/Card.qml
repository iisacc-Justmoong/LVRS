pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0
import "." as Private

AbstractButton {
    id: control

    enum CardType {
        File,
        FilePreview,
        Folder,
        Project,
        Device,
        Model,
        Member,
        Link
    }
    enum CardSize {
        Small,
        Medium,
        Large
    }
    enum CardDetail {
        Brief,
        Detailed
    }
    enum DisplayState {
        Automatic,
        DefaultState,
        HoverState,
        SelectedState
    }

    property int type: Card.File
    property int size: Card.Medium
    property int detail: Card.Brief
    // Automatic follows pointer/focus and checked. Overrides are for static previews.
    property int displayState: Card.Automatic
    property alias selected: control.checked
    property bool selectable: type !== Card.FilePreview

    property string title: ""
    property alias filename: control.title
    property string description: ""
    property string details: ""
    property string metadata: ""
    property string statusText: ""
    property string summary: ""
    property string footnote: ""
    property string actionText: ["", "", qsTr("Open"), qsTr("View"), qsTr("Manage"), qsTr("Use"), qsTr("Profile"), qsTr("Visit")][type] || ""
    property string domain: ""
    property string previewTitle: ""

    // Folder/Model/Member: label-value rows. Project/Device: progress, then rows.
    property var rows: []
    property string progressLabel: type === Card.Device ? qsTr("Storage") : qsTr("Milestones")
    property string progressText: ""
    property real progress: 0
    property color progressColor: Theme.primary
    property color statusColor: type === Card.Device || type === Card.Model || type === Card.Member ? Theme.success : Theme.descriptionColor

    property bool showDescription: true
    property bool showStatus: true
    property bool showMenu: type !== Card.File && type !== Card.FilePreview
    property bool showAction: type !== Card.File && type !== Card.FilePreview
    property string menuAccessibleName: qsTr("More actions")
    property string iconName: ""
    property url iconSource: ""
    property url previewSource: ""
    property bool asynchronous: true
    // The loader sizes custom content to the complete preview/icon rectangle.
    property Component previewComponent: null
    property Component iconComponent: null

    readonly property bool imageCard: type === Card.File || type === Card.FilePreview
    readonly property bool previewOnly: type === Card.FilePreview
    readonly property bool progressCard: type === Card.Project || type === Card.Device
    readonly property bool effectiveSelected: !previewOnly && (displayState === Card.Automatic ? selected : displayState === Card.SelectedState)
    readonly property bool effectiveHovered: !previewOnly && (displayState === Card.Automatic ? effectiveEnabled && (hovered || visualFocus) : displayState === Card.HoverState)
    readonly property real borderWidth: previewOnly ? 0 : effectiveSelected ? Theme.scaleRealMetric(2) : Theme.scaleRealMetric(1)
    readonly property real contentInset: (type === Card.File && size === Card.Small ? Theme.gap12 : Theme.gap18) + borderWidth
    readonly property color borderColor: effectiveSelected ? Theme.primary : imageCard && effectiveHovered ? Theme.titleHeaderColor : Theme.panelBackground10
    readonly property color surfaceColor: imageCard ? Theme.panelBackground03 : effectiveSelected ? Theme.panelBackground06 : effectiveHovered ? Theme.panelBackground07 : Theme.panelBackground05
    readonly property url resolvedIconSource: iconSource.toString().length ? iconSource : iconName.length ? Theme.iconPath(iconName) : cardIcon(["image", "image", "folder", "project", "device", "model", "member", "link"][type] || "image")
    readonly property int previewStatus: {
        const preview = bodyLoader.item as Private.CardImageContent;
        return preview ? preview.previewStatus : Image.Null;
    }
    readonly property Item previewItem: {
        const preview = bodyLoader.item as Private.CardImageContent;
        return preview ? preview.previewItem : null;
    }
    readonly property string variantName: ["File", "FilePreview", "Folder", "Project", "Device", "Model", "Member", "Link"][type] || "File"

    signal menuRequested
    signal actionTriggered

    function cardIcon(name: string): url {
        return "qrc:/qt/qml/LVRS/resources/images/card/" + name + ".svg";
    }

    tone: AbstractButton.Default
    checkable: selectable
    focusPolicy: effectiveEnabled && !previewOnly ? Qt.StrongFocus : Qt.NoFocus
    activeFocusOnTab: effectiveEnabled && !previewOnly
    cornerRadius: previewOnly ? 0 : Theme.radiusLg
    horizontalPadding: 0
    verticalPadding: 0
    spacing: 0
    clip: true
    implicitWidth: Theme.scaleMetric(previewOnly ? 480 : type === Card.File ? size === Card.Small ? 192 : size === Card.Large ? 480 : 256 : 256)
    implicitHeight: previewOnly ? Theme.scaleMetric(320) : type === Card.File ? Theme.scaleMetric(size === Card.Small ? 192 : size === Card.Large ? 280 : 320) : Math.max(Theme.scaleMetric(280), bodyLoader.item ? (bodyLoader.item as Item).implicitHeight : 0)
    Accessible.name: title
    Accessible.description: [description, details, metadata, statusText].filter(value => value.length > 0).join(". ")

    background: Rectangle {
        StateColorBehavior on color { motionEnabled: control.motionEnabled && control.enabled }
        color: control.surfaceColor
        radius: control.resolvedCornerRadius
        antialiasing: true
    }

    contentItem: Item {
        opacity: control.effectiveEnabled ? 1 : 0.4

        Loader {
            id: bodyLoader
            anchors.fill: parent
            sourceComponent: control.imageCard ? imageContent : informationContent
        }
        Component {
            id: imageContent
            Private.CardImageContent {
                card: control
            }
        }
        Component {
            id: informationContent
            Private.CardInformationContent {
                card: control
            }
        }
        // Overlay the stroke after the image, so a full-bleed preview cannot hide it.
        Rectangle {
            anchors.fill: parent
            radius: control.resolvedCornerRadius
            color: "transparent"
            border.width: control.borderWidth
            border.color: control.borderColor
            antialiasing: true
            visible: !control.previewOnly
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Card { type: LV.Card.File; size: LV.Card.Large; detail: LV.Card.Detailed
//     previewSource: "file:///photos/coast.png"; title: "Coast.png"; metadata: "PNG · 1536 × 1024"
//     details: "A quiet view of the sea."; onMenuRequested: fileMenu.open() }

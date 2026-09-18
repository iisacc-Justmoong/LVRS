import QtQuick
import QtQuick.Effects
import LVRS 1.0

Item {
    id: root
    required property var card
    readonly property int previewStatus: previewLoader.active ? (previewLoader.status === Loader.Ready ? Image.Ready : Image.Loading) : previewImage.status
    readonly property Item previewItem: previewLoader.active ? (previewLoader.item as Item) : previewImage
    readonly property bool small: card.size === Card.Small
    readonly property real titleHeight: Theme.scaleMetric(small ? 17 : 19)
    readonly property real detailHeight: Theme.scaleMetric(small ? 32 : card.size === Card.Large ? 36 : 54)
    readonly property bool softwareRenderer: GraphicsInfo.api === GraphicsInfo.Software
    readonly property real scrimHeight: Math.min(height, caption.height + (small ? Theme.gap12 : Theme.gap18) + Theme.scaleMetric(64))

    Item {
        id: pixels
        anchors.fill: parent
        visible: root.card.previewOnly || root.softwareRenderer
        clip: true

        Image {
            id: previewImage
            objectName: "card_previewImage"
            anchors.fill: parent
            source: previewLoader.active ? "" : root.card.previewSource
            asynchronous: root.card.asynchronous
            fillMode: Image.PreserveAspectCrop
            smooth: true
            mipmap: true
            visible: root.card.previewOnly || !root.softwareRenderer
            onStatusChanged: softwarePaint.requestPaint()
        }
        Loader {
            id: previewLoader
            anchors.fill: parent
            active: root.card.previewComponent !== null
            sourceComponent: root.card.previewComponent
        }
        Image {
            anchors.centerIn: parent
            width: Theme.scaleMetric(36)
            height: width
            source: root.card.cardIcon("image")
            fillMode: Image.PreserveAspectFit
            visible: !previewLoader.active && previewImage.status !== Image.Ready
        }
        // MultiEffect is unavailable on the software scene graph. Load the URL
        // into Canvas's image cache and paint the same crop and alpha mask there.
        Canvas {
            id: softwarePaint
            anchors.fill: parent
            visible: root.softwareRenderer && !root.card.previewOnly
            property real scrimHeight: root.scrimHeight
            property real radius: root.card.resolvedCornerRadius
            property url imageSource: previewLoader.active ? "" : root.card.previewSource
            property url cachedSource: ""
            function refreshImage() {
                if (cachedSource !== imageSource && cachedSource.toString().length)
                    unloadImage(cachedSource);
                cachedSource = imageSource;
                if (visible && imageSource.toString().length)
                    loadImage(imageSource);
                requestPaint();
            }
            onImageSourceChanged: refreshImage()
            onImageLoaded: requestPaint()
            Component.onCompleted: refreshImage()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onVisibleChanged: refreshImage()
            onScrimHeightChanged: requestPaint()
            onRadiusChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                if (width <= 0 || height <= 0)
                    return;
                ctx.beginPath();
                ctx.roundedRect(0, 0, width, height, radius, radius);
                ctx.clip();
                if (!previewLoader.active && previewImage.status === Image.Ready && isImageLoaded(imageSource)) {
                    const sourceWidth = previewImage.sourceSize.width;
                    const sourceHeight = previewImage.sourceSize.height;
                    const scale = Math.max(width / sourceWidth, height / sourceHeight);
                    const cropWidth = width / scale;
                    const cropHeight = height / scale;
                    ctx.drawImage(imageSource, (sourceWidth - cropWidth) / 2, (sourceHeight - cropHeight) / 2, cropWidth, cropHeight, 0, 0, width, height);
                }
                const gradient = ctx.createLinearGradient(0, height - scrimHeight, 0, height);
                gradient.addColorStop(0, Qt.rgba(0, 0, 0, 0));
                gradient.addColorStop(0.36, Qt.rgba(0, 0, 0, 0.84));
                gradient.addColorStop(1, Qt.rgba(0, 0, 0, 0.96));
                ctx.fillStyle = gradient;
                ctx.fillRect(0, height - scrimHeight, width, scrimHeight);
            }
        }
        Rectangle {
            objectName: "card_scrim"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.scrimHeight
            visible: !root.card.previewOnly && !root.softwareRenderer
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#00000000"
                }
                GradientStop {
                    position: 0.36
                    color: "#D6000000"
                }
                GradientStop {
                    position: 1
                    color: "#F5000000"
                }
            }
        }
    }
    Rectangle {
        id: mask
        anchors.fill: parent
        radius: root.card.resolvedCornerRadius
        color: "white"
        antialiasing: true
        visible: false
        layer.enabled: !root.card.previewOnly && !root.softwareRenderer
    }
    MultiEffect {
        anchors.fill: parent
        source: pixels
        maskEnabled: true
        maskSource: mask
        autoPaddingEnabled: false
        visible: !root.card.previewOnly && !root.softwareRenderer
    }

    Column {
        id: caption
        objectName: "card_caption"
        x: root.card.contentInset
        y: parent.height - root.card.contentInset - height
        width: Math.max(0, parent.width - root.card.contentInset * 2)
        spacing: Theme.gap6
        visible: !root.card.previewOnly

        Label {
            objectName: "card_filename"
            width: parent.width
            height: root.titleHeight
            style: header2
            font.pixelSize: Theme.scaleTextMetric(root.small ? 13 : 15)
            lineHeight: root.titleHeight
            text: root.card.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            maximumLineCount: 1
        }
        Label {
            objectName: "card_details"
            width: parent.width
            height: root.detailHeight
            visible: root.card.detail === Card.Detailed
            style: body
            font.pixelSize: Theme.scaleTextMetric(root.small ? 12 : 13)
            lineHeight: Theme.scaleMetric(root.small ? 16 : 18)
            text: root.card.details
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: root.small || root.card.size === Card.Large ? 2 : 3
            clip: true
        }
        Label {
            objectName: "card_metadata"
            width: parent.width
            height: Theme.scaleMetric(14)
            style: description
            font.pixelSize: Theme.scaleTextMetric(11)
            lineHeight: Theme.scaleMetric(14)
            color: Theme.bodyColor
            text: root.card.metadata
            textFormat: Text.PlainText
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    Rectangle {
        objectName: "card_selectionMarker"
        x: Theme.gap12
        y: Theme.gap12
        width: Theme.scaleMetric(24)
        height: width
        radius: width / 2
        color: Theme.primary
        visible: root.card.effectiveSelected
        Image {
            anchors.centerIn: parent
            width: Theme.iconSm
            height: width
            source: root.card.cardIcon("check")
        }
    }
    IconButton {
        objectName: "card_menu"
        anchors.right: parent.right
        anchors.rightMargin: Theme.gap12
        y: Theme.gap12
        width: Theme.scaleMetric(28)
        height: width
        cornerRadius: Theme.scaleMetric(6)
        visible: root.card.showMenu && !root.card.previewOnly
        enabled: root.card.effectiveEnabled
        tone: AbstractButton.Default
        iconSource: root.card.cardIcon("more")
        backgroundColor: Qt.rgba(Theme.panelBackground03.r, Theme.panelBackground03.g, Theme.panelBackground03.b, 0.9)
        Accessible.name: root.card.menuAccessibleName
        onClicked: root.card.menuRequested()
    }
}

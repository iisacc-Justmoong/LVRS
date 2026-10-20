import QtQuick
import QtQuick.Effects
import LVRS 1.0

Item {
    id: control

    enum AvatarSize {
        Small,
        Medium,
        Large
    }

    property int size: Avatar.Large
    property url source: ""
    property alias asynchronous: avatarImage.asynchronous
    property alias cache: avatarImage.cache
    property color backgroundColor: Theme.surfaceRaised
    readonly property int status: avatarImage.status
    readonly property real progress: avatarImage.progress
    readonly property bool softwareRenderer: GraphicsInfo.api === GraphicsInfo.Software

    implicitWidth: Theme.scaleMetric(size === Avatar.Small ? 24 : size === Avatar.Medium ? 40 : 64)
    implicitHeight: implicitWidth
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("User avatar")

    Rectangle {
        id: circle
        objectName: "avatar_circle"
        anchors.centerIn: parent
        width: Math.max(0, Math.min(control.width, control.height))
        height: width
        radius: width / 2
        color: control.backgroundColor
        antialiasing: true

        // Keep the standard Qt image loader for resources, providers and URLs.
        Image {
            id: avatarImage
            anchors.fill: parent
            source: control.source
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter
            smooth: true
            mipmap: true
            visible: false
            onStatusChanged: softwarePaint.requestPaint()
        }

        Rectangle {
            id: mask
            anchors.fill: parent
            radius: width / 2
            color: "white"
            antialiasing: true
            visible: false
            layer.enabled: !control.softwareRenderer
        }

        MultiEffect {
            anchors.fill: parent
            source: avatarImage
            maskEnabled: true
            maskSource: mask
            autoPaddingEnabled: false
            visible: !control.softwareRenderer && control.status === Image.Ready
        }

        // The software scene graph cannot run MultiEffect shaders. Canvas uses
        // the same URL loader and applies the equivalent center crop and mask.
        Canvas {
            id: softwarePaint
            anchors.fill: parent
            visible: control.softwareRenderer && control.status === Image.Ready
            property url imageSource: avatarImage.source
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
            onVisibleChanged: refreshImage()
            onImageLoaded: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Component.onCompleted: refreshImage()

            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                if (width <= 0 || height <= 0 || control.status !== Image.Ready || !isImageLoaded(imageSource))
                    return;
                const imageWidth = avatarImage.sourceSize.width;
                const imageHeight = avatarImage.sourceSize.height;
                if (imageWidth <= 0 || imageHeight <= 0)
                    return;
                const cropSize = Math.min(imageWidth, imageHeight);
                ctx.beginPath();
                ctx.arc(width / 2, height / 2, width / 2, 0, Math.PI * 2);
                ctx.clip();
                ctx.drawImage(imageSource, (imageWidth - cropSize) / 2, (imageHeight - cropSize) / 2,
                    cropSize, cropSize, 0, 0, width, height);
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Avatar { source: Qt.resolvedUrl("images/profile.webp"); size: LV.Avatar.Medium }
// LV.Avatar { source: user.photoUrl; width: 80; height: 80 }

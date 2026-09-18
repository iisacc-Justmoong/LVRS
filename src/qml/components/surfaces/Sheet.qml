import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Popup {
    id: root
    property bool motionEnabled: true

    enum Presentation {
        Automatic,
        Mobile,
        Desktop
    }
    enum Detent {
        Fit,
        Medium,
        Large
    }

    property int presentation: Sheet.Automatic
    property int detent: Sheet.Fit
    readonly property bool mobilePresentation: presentation === Sheet.Mobile || (presentation === Sheet.Automatic && Theme.mobileTarget)

    // Measured device radius in QML logical pixels. Physical pixels / display DPR.
    // Defaults are design fallbacks, never a hardware or OS-name measurement.
    property real cornerRadius: mobilePresentation ? 24 : 16
    readonly property real resolvedCornerRadius: Math.min(finiteNonNegative(cornerRadius), Math.max(0, width / 2), Math.max(0, height / 2))

    property string title: ""
    property string description: ""
    property bool showHeader: true
    property bool showDescription: true
    property bool showCloseButton: true
    property bool showGrabber: mobilePresentation

    property Component contentComponent: null
    readonly property alias loadedContent: contentLoader.item
    default property alias content: inlineContent.data
    property bool scrollContent: true
    readonly property real availableContentHeight: viewport.height
    readonly property real availableContentWidth: viewport.width

    property real preferredWidth: 560
    property real preferredHeight: 420
    property real desktopMargin: Theme.gap24
    property real contentPadding: Theme.gap20
    property real topSafeInset: safeArea.topInset
    property real bottomSafeInset: safeArea.bottomInset
    property bool dismissOnBackground: true
    property bool dismissOnEscape: true
    property bool dismissOnDrag: true
    property int animationDuration: Motion.surfaceDuration
    property color surfaceColor: Theme.panelBackground05
    property color backdropColor: Qt.rgba(0, 0, 0, 0.48)

    readonly property real _viewportWidth: parent ? parent.width : 0
    readonly property real _viewportHeight: parent ? parent.height : 0
    readonly property real _bottomInset: mobilePresentation ? finiteNonNegative(bottomSafeInset) : 0
    readonly property real _maximumHeight: Math.max(0, _viewportHeight - (mobilePresentation ? Math.max(Theme.gap24, finiteNonNegative(topSafeInset)) : 2 * finiteNonNegative(desktopMargin)))
    readonly property real _naturalContentHeight: contentComponent !== null ? (contentLoader.item ? finiteNonNegative(contentLoader.item.implicitHeight) : 0) : inlineContent.implicitHeight
    readonly property real _chromeHeight: headerFrame.y + headerFrame.height + contentPadding + _bottomInset
    readonly property real _restingY: mobilePresentation ? _viewportHeight - height : (_viewportHeight - height) / 2
    property real _reveal: 0
    property real _dragOffset: 0
    property Item _returnFocusItem: null
    readonly property WindowSafeAreaObserver safeArea: WindowSafeAreaObserver {
        window: root.parent ? root.parent.Window.window : null
    }
    readonly property NumberAnimation snapBack: NumberAnimation {
        target: root
        property: "_dragOffset"
        to: 0
        duration: root.motionEnabled ? Motion.duration(root.animationDuration) : 0
        easing.type: Easing.OutBack
        easing.overshoot: Motion.overshoot
    }

    function finiteNonNegative(value) {
        return isFinite(value) ? Math.max(0, value) : 0;
    }

    parent: Controls.Overlay.overlay
    modal: true
    dim: true
    focus: true
    padding: 0
    // Geometry is bounded above; allow the enter/exit path below the viewport.
    // Popup's nonnegative margins otherwise clamp away the slide animation.
    margins: -1
    closePolicy: (dismissOnBackground ? Controls.Popup.CloseOnPressOutside : Controls.Popup.NoAutoClose) | (dismissOnEscape ? Controls.Popup.CloseOnEscape : Controls.Popup.NoAutoClose)
    width: mobilePresentation ? _viewportWidth : Math.min(finiteNonNegative(preferredWidth), Math.max(0, _viewportWidth - 2 * finiteNonNegative(desktopMargin)))
    height: Math.min(_maximumHeight, mobilePresentation ? (detent === Sheet.Large ? Math.round(_viewportHeight * 0.9) : detent === Sheet.Medium ? Math.round(_viewportHeight * 0.57) : _naturalContentHeight + _chromeHeight) : Math.max(_chromeHeight, finiteNonNegative(preferredHeight)))
    x: (_viewportWidth - width) / 2
    y: _restingY + (mobilePresentation ? height * (1 - _reveal) + _dragOffset : 0)
    opacity: mobilePresentation ? 1 : Math.max(0, Math.min(1, _reveal))
    scale: mobilePresentation ? 1 : 0.94 + 0.06 * _reveal

    onAboutToShow: {
        snapBack.stop();
        _dragOffset = 0;
        const hostWindow = parent ? parent.Window.window : null;
        _returnFocusItem = hostWindow ? hostWindow.activeFocusItem : null;
        viewport.contentY = 0;
    }
    onClosed: {
        _reveal = 0;
        _dragOffset = 0;
        if (_returnFocusItem && _returnFocusItem.visible && _returnFocusItem.enabled)
            _returnFocusItem.forceActiveFocus(Qt.PopupFocusReason);
        _returnFocusItem = null;
    }

    enter: Transition {
        NumberAnimation {
            target: root
            property: "_reveal"
            from: 0
            to: 1
            duration: root.motionEnabled ? Motion.duration(root.animationDuration) : 0
            easing.type: Easing.OutBack
        easing.overshoot: Motion.overshoot
        }
    }
    exit: Transition {
        NumberAnimation {
            target: root
            property: "_reveal"
            to: 0
            duration: root.motionEnabled ? Motion.duration(root.animationDuration) : 0
            easing.type: Easing.InCubic
        }
    }

    Controls.Overlay.modal: Rectangle {
        color: root.backdropColor
    }
    background: Rectangle {
        objectName: "sheet_surface"
        color: root.surfaceColor
        radius: root.resolvedCornerRadius
        antialiasing: true
    }

    contentItem: Item {
        objectName: "sheet_frame"
        clip: true

        Item {
            id: grabber
            objectName: "sheet_grabber"
            width: parent.width
            height: visible ? 28 : 0
            visible: root.showGrabber
            Rectangle {
                width: 36
                height: 4
                radius: 2
                anchors.centerIn: parent
                color: Theme.descriptionColor
            }
            DragHandler {
                id: grabberDrag
                target: null
                enabled: root.mobilePresentation && root.dismissOnDrag && root.opened
                xAxis.enabled: false
                onActiveTranslationChanged: {
                    if (active)
                        root._dragOffset = Math.max(0, activeTranslation.y);
                }
                onActiveChanged: {
                    if (active) {
                        root.snapBack.stop();
                    } else if (root._dragOffset >= Math.min(120, root.height * 0.2)) {
                        root.close();
                    } else {
                        root.snapBack.restart();
                    }
                }
            }
        }

        Item {
            id: headerFrame
            objectName: "sheet_header"
            x: root.contentPadding
            y: grabber.height + (root.mobilePresentation ? 0 : Theme.gap24)
            width: Math.max(0, parent.width - root.contentPadding * 2)
            height: visible ? Math.max(labels.implicitHeight, closeButton.visible ? 44 : 0) + Theme.gap16 : 0
            visible: root.showHeader
            Column {
                id: labels
                width: Math.max(0, parent.width - (closeButton.visible ? 44 + Theme.gap12 : 0))
                anchors.verticalCenter: closeButton.visible ? closeButton.verticalCenter : undefined
                spacing: Theme.gap8
                Label {
                    width: parent.width
                    style: header
                    text: root.title
                    visible: text.length > 0
                    wrapMode: Text.WordWrap
                    sizeToContentHeight: true
                }
                Label {
                    width: parent.width
                    style: body
                    color: Theme.descriptionColor
                    text: root.description
                    visible: root.showDescription && text.length > 0
                    wrapMode: Text.WordWrap
                    sizeToContentHeight: true
                }
            }
            IconButton {
                id: closeButton
                objectName: "sheet_close"
                anchors.right: parent.right
                width: 44
                height: 44
                iconName: "generalclose"
                iconSize: 16
                cornerRadius: 16
                backgroundColor: Theme.panelBackground09
                textColor: Theme.descriptionColor
                visible: root.showCloseButton
                Accessible.name: qsTr("Close sheet")
                contentItem: Item {
                    Image {
                        objectName: "sheet_closeIcon"
                        anchors.centerIn: parent
                        width: closeButton.iconSize
                        height: closeButton.iconSize
                        source: RenderQuality.resolveTextureSource(closeButton.resolvedIconSource)
                        sourceSize: Qt.size(closeButton.iconSourceSize, closeButton.iconSourceSize)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: RenderQuality.mipmapEnabled
                    }
                }
                onClicked: root.close()
            }
        }

        Flickable {
            id: viewport
            objectName: "sheet_viewport"
            x: root.contentPadding
            y: headerFrame.y + headerFrame.height
            width: Math.max(0, parent.width - root.contentPadding * 2)
            height: Math.max(0, parent.height - y - root.contentPadding - root._bottomInset)
            clip: true
            contentWidth: width
            contentHeight: root.scrollContent ? root._naturalContentHeight : height
            interactive: root.scrollContent && contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            onContentHeightChanged: contentY = Math.max(0, Math.min(contentY, contentHeight - height))
            onHeightChanged: contentY = Math.max(0, Math.min(contentY, contentHeight - height))
            Controls.ScrollBar.vertical: Controls.ScrollBar {}

            Loader {
                id: contentLoader
                objectName: "sheet_contentLoader"
                sourceComponent: root.contentComponent
                active: root.contentComponent !== null
                visible: active
                width: viewport.width
                height: root.scrollContent ? root._naturalContentHeight : viewport.height
            }
            Item {
                id: inlineContent
                objectName: "sheet_inlineContent"
                visible: root.contentComponent === null
                width: viewport.width
                height: root.scrollContent ? implicitHeight : viewport.height
                implicitHeight: {
                    let result = 0;
                    for (let i = 0; i < children.length; ++i) {
                        const child = children[i];
                        if (child.visible)
                            result = Math.max(result, child.y + (child.implicitHeight > 0 ? child.implicitHeight : child.height));
                    }
                    return result;
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Sheet { id: sheet; title: "Export"; cornerRadius: deviceRadius; contentComponent: exportView }
// LV.LabelButton { text: "Export"; onClicked: sheet.open() }

import QtQuick
import QtQuick.Window as QtQuickWindow
import LVRS 1.0

QtQuickWindow.Window {
    id: root

    readonly property string platform: Qt.platform.os
    readonly property bool isMobilePlatform: platform === "android" || platform === "ios"
    readonly property bool isDesktopPlatform: platform === "osx" || platform === "windows" || platform === "linux"

    readonly property int compact: 0
    readonly property int medium: 1
    readonly property int expanded: 2

    readonly property int widthClass: root.width < 600 ? compact : (root.width < 1000 ? medium : expanded)
    readonly property int heightClass: root.height < 600 ? compact : (root.height < 900 ? medium : expanded)
    readonly property bool isCompact: widthClass === compact || heightClass === compact
    readonly property bool isExpanded: widthClass === expanded && heightClass === expanded

    property int desktopMinWidth: 360
    property int desktopMinHeight: 240
    property int mobileMinWidth: 320
    property int mobileMinHeight: 240
    property bool usePlatformSafeMargin: false
    property int safeMargin: usePlatformSafeMargin && isMobilePlatform ? 12 : 0
    property color windowColor: Theme.window
    property bool forceNativeDarkTitleBar: Theme.dark
    property bool solidChrome: true
    property bool autoApplyRenderQuality: true
    property bool autoAttachRuntimeEvents: false

    readonly property real effectiveSupersampleScale: autoApplyRenderQuality && RenderQuality.enabled
        ? Math.max(RenderQuality.minimumSupersampleScale,
                   Math.min(RenderQuality.maximumSupersampleScale, RenderQuality.supersampleScale))
        : 1.0

    default property alias content: contentHost.data

    minimumWidth: isMobilePlatform ? mobileMinWidth : desktopMinWidth
    minimumHeight: isMobilePlatform ? mobileMinHeight : desktopMinHeight
    color: root.windowColor

    function matchesMedia(rule) {
        if (!rule)
            return true
        var token = String(rule).toLowerCase()
        if (token === "mobile")
            return isMobilePlatform
        if (token === "desktop")
            return isDesktopPlatform
        if (token === "compact")
            return isCompact
        if (token === "expanded")
            return isExpanded
        if (token === "medium")
            return widthClass === medium || heightClass === medium
        return false
    }

    function applyNativeWindowStyle() {
        if (root.solidChrome && NativeWindowStyle.solidChromeSupported)
            return NativeWindowStyle.applySolidChrome(root, root.windowColor, root.forceNativeDarkTitleBar)
        if (!NativeWindowStyle.titleBarColorSupported)
            return false
        return NativeWindowStyle.applyTitleBarColor(root, root.windowColor, root.forceNativeDarkTitleBar)
    }

    onVisibleChanged: {
        if (root.visible)
            applyNativeWindowStyle()
    }
    onWindowColorChanged: applyNativeWindowStyle()
    onForceNativeDarkTitleBarChanged: applyNativeWindowStyle()
    onSolidChromeChanged: applyNativeWindowStyle()
    onAutoApplyRenderQualityChanged: {
        if (autoApplyRenderQuality)
            RenderQuality.applyWindow(root)
    }
    onAutoAttachRuntimeEventsChanged: {
        if (!autoAttachRuntimeEvents)
            return
        RuntimeEvents.start()
        RuntimeEvents.attachWindow(root)
    }

    Item {
        id: contentHost
        anchors.fill: parent
        anchors.margins: root.safeMargin
        layer.enabled: root.effectiveSupersampleScale > 1.0
        layer.smooth: layer.enabled
        layer.mipmap: layer.enabled
        layer.textureSize: layer.enabled
            ? Qt.size(
                  Math.max(1, Math.round(width * root.effectiveSupersampleScale)),
                  Math.max(1, Math.round(height * root.effectiveSupersampleScale)))
            : Qt.size(
                  Math.max(1, Math.round(width)),
                  Math.max(1, Math.round(height)))
    }

    QtObject {
        Component.onCompleted: {
            FontPolicy.enforceApplicationFallback()
            if (root.autoApplyRenderQuality) {
                RenderQuality.applyWindow(root)
                if (SvgManager.minimumScale < root.effectiveSupersampleScale)
                    SvgManager.minimumScale = root.effectiveSupersampleScale
            }
            if (root.autoAttachRuntimeEvents) {
                RuntimeEvents.start()
                RuntimeEvents.attachWindow(root)
            }
            Debug.log("Window", "supersample-scale", root.effectiveSupersampleScale)
            root.applyNativeWindowStyle()
            Qt.callLater(root.applyNativeWindowStyle)
        }
    }

}

// API usage (external):
// import LVRS as LV
// LV.Window { title: "Settings"; visible: true }

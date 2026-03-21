import QtQuick
import QtQuick.Window as QtQuickWindow
import LVRS 1.0

QtQuickWindow.Window {
    id: root

    readonly property string platform: Qt.platform.os
    readonly property var backendRuntimeProfile: Platform.runtimeProfile(platform)
    readonly property string canonicalPlatform: {
        const target = backendRuntimeProfile && backendRuntimeProfile.target !== undefined
            ? String(backendRuntimeProfile.target)
            : ""
        if (target.length > 0 && target !== "unknown")
            return target
        return Platform.canonicalOs
    }
    readonly property bool isMobilePlatform: backendRuntimeProfile.mobile === true
    readonly property bool isDesktopPlatform: backendRuntimeProfile.desktop === true
    readonly property bool backendMobilePlatform: isMobilePlatform

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
    property bool useBackendMobileScale: true
    property real mobileViewScale: 1.0
    readonly property real effectiveMobileViewScale: useBackendMobileScale && backendMobilePlatform
        ? Math.max(1.0, mobileViewScale)
        : 1.0
    property bool usePlatformSafeMargin: backendMobilePlatform
    property int safeMargin: usePlatformSafeMargin && isMobilePlatform ? 16 : 0
    readonly property real layoutSafeLeftInset: safeMargin
    readonly property real layoutSafeTopInset: safeMargin
    readonly property real layoutSafeRightInset: safeMargin
    readonly property real layoutSafeBottomInset: safeMargin
    readonly property rect renderSurfaceBounds: Qt.rect(contentHost.x, contentHost.y, contentHost.width, contentHost.height)
    readonly property rect layoutSafeAreaBounds: Qt.rect(contentHost.x + layoutSafeAreaHost.x,
                                                         contentHost.y + layoutSafeAreaHost.y,
                                                         layoutSafeAreaHost.width,
                                                         layoutSafeAreaHost.height)
    property color windowColor: Theme.window
    property bool forceNativeDarkTitleBar: Theme.dark
    property bool solidChrome: true
    property bool autoApplyRenderQuality: true
    property bool autoApplyDeviceTierPreset: true
    property int forcedDeviceTierPreset: -1
    property bool autoAttachRuntimeEvents: false

    readonly property real effectiveSupersampleScale: autoApplyRenderQuality && RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property bool sceneSupersamplingActive: autoApplyRenderQuality
        && RenderQuality.sceneSupersamplingActive

    default property alias content: scaledContentHost.data

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
        if (autoApplyRenderQuality) {
            if (autoApplyDeviceTierPreset)
                RenderQuality.applyDeviceTierPreset(forcedDeviceTierPreset)
            RenderQuality.applyWindow(root)
        }
    }
    onAutoApplyDeviceTierPresetChanged: {
        if (!autoApplyRenderQuality || !autoApplyDeviceTierPreset)
            return
        RenderQuality.applyDeviceTierPreset(forcedDeviceTierPreset)
        RenderQuality.applyWindow(root)
    }
    onForcedDeviceTierPresetChanged: {
        if (!autoApplyRenderQuality || !autoApplyDeviceTierPreset)
            return
        RenderQuality.applyDeviceTierPreset(forcedDeviceTierPreset)
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
        layer.enabled: root.sceneSupersamplingActive
        layer.smooth: layer.enabled
        layer.mipmap: RenderQuality.mipmapEnabled
        layer.textureSize: RenderQuality.resolveLayerTextureSize(width, height, layer.enabled)

        Item {
            id: layoutSafeAreaHost
            objectName: "windowLayoutSafeAreaHost"
            x: root.layoutSafeLeftInset
            y: root.layoutSafeTopInset
            width: Math.max(1, parent.width - root.layoutSafeLeftInset - root.layoutSafeRightInset)
            height: Math.max(1, parent.height - root.layoutSafeTopInset - root.layoutSafeBottomInset)

            Item {
                id: scaledContentHost
                x: 0
                y: 0
                width: Math.max(1, Math.round(parent.width / root.effectiveMobileViewScale))
                height: Math.max(1, Math.round(parent.height / root.effectiveMobileViewScale))
                scale: root.effectiveMobileViewScale
                transformOrigin: Item.TopLeft
            }
        }
    }

    QtObject {
        Component.onCompleted: {
            FontPolicy.enforceApplicationFallback()
            if (root.autoApplyRenderQuality) {
                if (root.autoApplyDeviceTierPreset)
                    RenderQuality.applyDeviceTierPreset(root.forcedDeviceTierPreset)
                RenderQuality.applyWindow(root)
                SvgManager.ensureMinimumScale(root.effectiveSupersampleScale)
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

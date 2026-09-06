pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LVRS as LV

LV.ApplicationWindow {
    id: root

    visible: true
    width: 1540
    height: 980
    desktopMinWidth: 960
    desktopMinHeight: 680
    usePlatformSafeMargin: true
    autoAttachRuntimeEvents: true
    autoHookBackendUserEvents: false
    globalEventListenersEnabled: true
    navigationEnabled: false
    title: "LVRS Visual Catalog"
    subtitle: activeEntry ? activeEntry.label : "Catalog Overview"

    readonly property bool catalogCompactLayout: width < 1260
    readonly property int catalogOuterMargin: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap16
    readonly property int catalogSectionGap: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap12
    readonly property int catalogContentInset: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap12
    readonly property int catalogCardInset: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap12
    readonly property int sidebarWidth: catalogCompactLayout ? 264 : 320
    readonly property rect catalogViewport: layoutSafeAreaBounds
    property bool catalogViewportReady: catalogViewport.width > 0 && catalogViewport.height > 0

    readonly property var runtimeSnapshot: LV.AppState.runtimeSnapshot
    readonly property var viewStateSnapshot: LV.AppState.viewStateSnapshot

    readonly property bool metricsRenderScaleCompliant:
        effectiveSupersampleScale >= 1.0
        && effectiveSupersampleScale <= LV.RenderQuality.maximumSupersampleScale
    readonly property bool metricsFontFallbackCompliant:
        LV.Theme.fontBody.length > 0
        && LV.FontPolicy.resolveFamily(LV.FontPolicy.preferredFamily).length > 0
    readonly property bool metricsThemeTextCompliant:
        LV.Theme.isThemeTextStyleCompliant(LV.Theme.textTitle, LV.Theme.textTitleWeight, LV.Theme.textTitleStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textTitle2, LV.Theme.textTitle2Weight, LV.Theme.textTitle2StyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textHeader, LV.Theme.textHeaderWeight, LV.Theme.textHeaderStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textHeader2, LV.Theme.textHeader2Weight, LV.Theme.textHeader2StyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textBody, LV.Theme.textBodyWeight, LV.Theme.textBodyStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textDescription, LV.Theme.textDescriptionWeight, LV.Theme.textDescriptionStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textCaption, LV.Theme.textCaptionWeight, LV.Theme.textCaptionStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textDisabled, LV.Theme.textDisabledWeight, LV.Theme.textDisabledStyleName)
    readonly property bool metricsRuntimeCompliant:
        runtimeSnapshot
        && runtimeSnapshot.pid !== undefined
        && runtimeSnapshot.uptimeMs !== undefined
        && runtimeSnapshot.rssBytes !== undefined
    readonly property bool metricsSvgCompliant:
        LV.SvgManager.minimumScale >= 1.0
        && LV.SvgManager.maximumScale >= LV.SvgManager.minimumScale
    readonly property bool metricsPageCompliant:
        LV.AppState.pageHistory.length > 0
    readonly property int metricsTotalChecks: 6
    readonly property int metricsPassedChecks:
        (metricsRenderScaleCompliant ? 1 : 0)
        + (metricsFontFallbackCompliant ? 1 : 0)
        + (metricsThemeTextCompliant ? 1 : 0)
        + (metricsRuntimeCompliant ? 1 : 0)
        + (metricsSvgCompliant ? 1 : 0)
        + (metricsPageCompliant ? 1 : 0)
    readonly property bool metricsPass: metricsPassedChecks === metricsTotalChecks
    readonly property string metricsSummary: metricsPassedChecks + "/" + metricsTotalChecks

    CatalogRegistry {
        id: catalogRegistry
    }

    property string activeEntryKey: catalogRegistry.overview.key
    readonly property var activeEntry: catalogRegistry.entryByKey(activeEntryKey)
    readonly property var activeBreadcrumb: catalogRegistry.breadcrumb(activeEntryKey)
    readonly property int catalogComponentCount: catalogRegistry.componentCount
    readonly property int catalogDocumentCount: catalogRegistry.totalEntryCount
    readonly property var activeChildren: catalogRegistry.directChildren(activeEntryKey)
    readonly property var activeRelated: root.recordsForKeys(activeEntry ? activeEntry.related : [])
    readonly property bool catalogSafeAreaEntryReady:
        catalogRegistry.entryByKey("window-safe-area-observer").key === "window-safe-area-observer"

    function syncRuntimeState() {
        LV.AppState.syncRuntimeSnapshot(LV.RuntimeEvents.snapshot())
        LV.AppState.syncViewStateSnapshot(LV.ViewStateTracker.snapshot())
        LV.AppState.syncPageHistory(LV.PageMonitor.history)
    }

    function breadcrumbText(parts) {
        if (!parts || parts.length === undefined || parts.length === 0)
            return ""
        return parts.join(" / ")
    }

    function recordsForKeys(keys) {
        const result = []
        if (!keys || keys.length === undefined)
            return result
        for (let i = 0; i < keys.length; i++) {
            const record = catalogRegistry.entryByKey(keys[i])
            if (record && result.indexOf(record) === -1)
                result.push(record)
        }
        return result
    }

    function activateCatalogEntry(key) {
        const record = catalogRegistry.entryByKey(key)
        if (!record)
            return false
        activeEntryKey = record.key
        Qt.callLater(function() {
            if (catalogHierarchy && catalogHierarchy.activeListItemKey !== record.key)
                catalogHierarchy.activateListItemByKey(record.key)
        })
        return true
    }

    function previewComponentFor(previewId) {
        switch (String(previewId || "")) {
        case "overview":
            return overviewPreview
        case "section":
            return sectionPreview
        case "application-shell":
            return applicationShellPreview
        case "window-shell":
            return windowShellPreview
        case "safe-area-observer":
            return safeAreaObserverPreview
        case "app-header":
            return appHeaderPreview
        case "stack-layout":
            return stackLayoutPreview
        case "abstract-button":
            return abstractButtonPreview
        case "button-family":
            return buttonFamilyPreview
        case "segmented-control":
            return segmentedControlPreview
        case "selector-control":
            return selectorControlPreview
        case "selection-control":
            return selectionControlPreview
        case "label-display":
            return labelDisplayPreview
        case "progress-display":
            return progressDisplayPreview
        case "table-display":
            return tableDisplayPreview
        case "input-field":
            return inputFieldPreview
        case "text-editor":
            return textEditorPreview
        case "code-editor":
            return codeEditorPreview
        case "event-listener":
            return eventListenerPreview
        case "guard-utility":
            return guardUtilityPreview
        case "router-navigation":
            return routerNavigationPreview
        case "hierarchy-navigation":
            return hierarchyNavigationPreview
        case "list-navigation":
            return listNavigationPreview
        case "menu-navigation":
            return menuNavigationPreview
        case "app-card-surface":
            return appCardSurfacePreview
        case "alert-surface":
            return alertSurfacePreview
        case "modal-surface":
            return modalSurfacePreview
        default:
            return placeholderPreview
        }
    }

    Component.onCompleted: {
        LV.AppState.bootstrap()
        LV.FontPolicy.enforceApplicationFallback()
        LV.RenderMonitor.attachWindow(root)
        LV.PageMonitor.record("/visual-catalog")
        syncRuntimeState()
        Qt.callLater(function() {
            root.activateCatalogEntry(root.activeEntryKey)
        })
    }

    onActiveEntryChanged: {
        if (previewLoader.item && previewLoader.item.catalogEntry !== undefined)
            previewLoader.item.catalogEntry = activeEntry
    }

    Timer {
        interval: 220
        running: true
        repeat: true
        onTriggered: root.syncRuntimeState()
    }

    Component {
        id: overviewPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                GridLayout {
                    width: parent.width
                    columns: root.catalogCompactLayout ? 1 : 3
                    rowSpacing: LV.Theme.gap8
                    columnSpacing: LV.Theme.gap8

                    Repeater {
                        model: [
                            { title: "Public Types", value: String(root.catalogComponentCount) },
                            { title: "Catalog Entries", value: String(root.catalogDocumentCount) },
                            { title: "Health Checks", value: root.metricsSummary }
                        ]

                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 84
                            radius: LV.Theme.radiusMd
                            color: LV.Theme.surfaceAlt
                            border.width: 1
                            border.color: LV.Theme.contextMenuDivider

                            Column {
                                anchors.fill: parent
                                anchors.margins: root.catalogContentInset
                                spacing: LV.Theme.gap4

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: modelData.title
                                }

                                LV.Label {
                                    style: title2
                                    color: LV.Theme.textPrimary
                                    text: modelData.value
                                }
                            }
                        }
                    }
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: "Start from a section in the sidebar, then drill down to a concrete type. Component pages stay consistent: summary, live preview, usage snippet, source location, and related links."
                }

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    Repeater {
                        model: [
                            { key: "application-window", label: "ApplicationWindow" },
                            { key: "input-field", label: "InputField" },
                            { key: "hierarchy", label: "Hierarchy" },
                            { key: "context-menu", label: "ContextMenu" },
                            { key: "modal", label: "Modal" }
                        ]

                        delegate: LV.LabelButton {
                            required property var modelData
                            text: modelData.label
                            tone: LV.AbstractButton.Default
                            onClicked: root.activateCatalogEntry(modelData.key)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: sectionPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: "Section pages act as local indexes. Choose any child type below to open its dedicated visual reference page."
                }

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    Repeater {
                        model: catalogRegistry.directChildren(preview.catalogEntry ? preview.catalogEntry.key : "")

                        delegate: LV.LabelButton {
                            required property var modelData
                            text: modelData.label
                            tone: LV.AbstractButton.Default
                            onClicked: root.activateCatalogEntry(modelData.key)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: applicationShellPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Rectangle {
                    width: parent.width
                    implicitHeight: shellColumn.implicitHeight + root.catalogContentInset * 2
                    radius: LV.Theme.radiusMd
                    color: LV.Theme.surfaceAlt
                    border.width: 1
                    border.color: LV.Theme.contextMenuDivider

                    Column {
                        id: shellColumn
                        x: root.catalogContentInset
                        y: root.catalogContentInset
                        width: parent.width - root.catalogContentInset * 2
                        spacing: LV.Theme.gap8

                        LV.AppHeader {
                            width: parent.width
                            title: preview.catalogEntry && preview.catalogEntry.key === "app-shell" ? "Compatibility Shell" : "Adaptive Shell"
                            subtitle: "Shell anatomy preview"
                            menuVisible: true

                            LV.IconButton {
                                iconName: "add"
                                tone: LV.AbstractButton.Borderless
                            }

                            LV.LabelButton {
                                text: "Refresh"
                                tone: LV.AbstractButton.Default
                            }
                        }

                        GridLayout {
                            width: parent.width
                            columns: root.catalogCompactLayout ? 1 : 3
                            rowSpacing: LV.Theme.gap8
                            columnSpacing: LV.Theme.gap8

                            Repeater {
                                model: [
                                    { title: "Current Route", value: LV.AppState.currentRoute.length > 0 ? LV.AppState.currentRoute : "/visual-catalog" },
                                    { title: "Runtime Uptime", value: root.runtimeSnapshot.uptimeMs !== undefined ? String(root.runtimeSnapshot.uptimeMs) + " ms" : "n/a" },
                                    { title: "Adaptive Mode", value: root.catalogCompactLayout ? "Compact" : "Wide" }
                                ]

                                delegate: Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    implicitHeight: 72
                                    radius: LV.Theme.radiusSm
                                    color: LV.Theme.surfaceGhost

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: root.catalogContentInset
                                        spacing: LV.Theme.gap2

                                        LV.Label {
                                            style: caption
                                            color: LV.Theme.textTertiary
                                            text: modelData.title
                                        }

                                        LV.Label {
                                            style: body
                                            color: LV.Theme.textPrimary
                                            text: modelData.value
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: windowShellPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: frameRect.implicitHeight

            Rectangle {
                id: frameRect
                width: parent.width
                implicitHeight: windowColumn.implicitHeight + root.catalogContentInset * 2
                radius: LV.Theme.radiusMd
                color: LV.Theme.surfaceAlt
                border.width: 1
                border.color: LV.Theme.contextMenuDivider

                Column {
                    id: windowColumn
                    x: root.catalogContentInset
                    y: root.catalogContentInset
                    width: parent.width - root.catalogContentInset * 2
                    spacing: LV.Theme.gap8

                    RowLayout {
                        width: parent.width

                        LV.Label {
                            Layout.fillWidth: true
                            style: header2
                            color: LV.Theme.textPrimary
                            text: "Window metrics"
                        }

                        LV.Label {
                            style: caption
                            color: LV.Theme.textSecondary
                            text: root.width + " x " + root.height
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 160
                        radius: LV.Theme.radiusMd
                        color: LV.Theme.windowAlt
                        border.width: 1
                        border.color: LV.Theme.contextMenuDivider

                        Column {
                            anchors.fill: parent
                            anchors.margins: root.catalogContentInset
                            spacing: LV.Theme.gap6

                            LV.Label {
                                style: body
                                color: LV.Theme.textPrimary
                                text: "LV.Window is a lighter wrapper than LV.ApplicationWindow."
                            }

                            LV.Label {
                                style: description
                                color: LV.Theme.textSecondary
                                text: "Use it when the scaffold is unnecessary but render-quality and platform policies still matter."
                                wrapMode: Text.WordWrap
                            }

                            LV.Label {
                                style: caption
                                color: LV.Theme.textTertiary
                                text: "layout viewport="
                                    + Math.round(root.layoutSafeAreaBounds.width)
                                    + "x"
                                    + Math.round(root.layoutSafeAreaBounds.height)
                                    + " | systemTopInset="
                                    + Math.round(root.mobileSystemSafeTopInset)
                                    + " | supersample="
                                    + root.effectiveSupersampleScale.toFixed(2)
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: safeAreaObserverPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: "WindowSafeAreaObserver exposes platform insets, while layout ownership remains with the consumer. Under the new contract, the render surface stays full-bleed and the app decides where to reserve the safe area."
                }

                GridLayout {
                    width: parent.width
                    columns: root.catalogCompactLayout ? 1 : 2
                    rowSpacing: LV.Theme.gap8
                    columnSpacing: LV.Theme.gap8

                    Repeater {
                        model: [
                            { title: "Resolved", value: root.mobileSystemSafeAreaResolved ? "true" : "false" },
                            { title: "Layout Viewport", value: Math.round(root.layoutSafeAreaBounds.width) + " x " + Math.round(root.layoutSafeAreaBounds.height) },
                            { title: "System Insets", value: "top " + Math.round(root.mobileSystemSafeTopInset) + " | bottom " + Math.round(root.mobileSystemSafeBottomInset) },
                            { title: "Horizontal Insets", value: "left " + Math.round(root.mobileSystemSafeLeftInset) + " | right " + Math.round(root.mobileSystemSafeRightInset) }
                        ]

                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 78
                            radius: LV.Theme.radiusMd
                            color: LV.Theme.surfaceAlt
                            border.width: 1
                            border.color: LV.Theme.contextMenuDivider

                            Column {
                                anchors.fill: parent
                                anchors.margins: root.catalogContentInset
                                spacing: LV.Theme.gap4

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: modelData.title
                                }

                                LV.Label {
                                    style: body
                                    color: LV.Theme.textPrimary
                                    wrapMode: Text.WordWrap
                                    text: modelData.value
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: appHeaderPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: headerColumn.implicitHeight

            Column {
                id: headerColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.AppHeader {
                    width: parent.width
                    title: "Design System"
                    subtitle: "Header actions and page chrome"
                    menuVisible: true

                    LV.LabelButton {
                        text: "Share"
                        tone: LV.AbstractButton.Default
                    }

                    LV.IconButton {
                        iconName: "viewMoreSymbolicDefault"
                        tone: LV.AbstractButton.Borderless
                    }
                }
            }
        }
    }

    Component {
        id: stackLayoutPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.Label {
                    style: caption
                    color: LV.Theme.textTertiary
                    text: "Selected: " + (preview.catalogEntry ? preview.catalogEntry.label : "")
                }

                LV.VStack {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    Rectangle {
                        width: parent.width
                        height: 56
                        radius: LV.Theme.radiusMd
                        color: LV.Theme.surfaceAlt

                        LV.HStack {
                            anchors.fill: parent
                            anchors.margins: root.catalogContentInset
                            spacing: LV.Theme.gap8

                            Rectangle {
                                width: 72
                                height: 24
                                radius: LV.Theme.radiusSm
                                color: LV.Theme.accentOverlay
                            }

                            LV.Spacer { }

                            Rectangle {
                                width: 120
                                height: 24
                                radius: LV.Theme.radiusSm
                                color: LV.Theme.surfaceGhost
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 120
                        radius: LV.Theme.radiusMd
                        color: LV.Theme.surfaceAlt
                        border.width: 1
                        border.color: LV.Theme.contextMenuDivider

                        LV.ZStack {
                            anchors.fill: parent
                            anchors.margins: root.catalogContentInset

                            Rectangle {
                                anchors.fill: parent
                                radius: LV.Theme.radiusSm
                                color: LV.Theme.surfaceGhost
                            }

                            LV.Label {
                                anchors.centerIn: parent
                                style: header2
                                color: LV.Theme.textPrimary
                                text: "Layered content"
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: abstractButtonPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    LV.AbstractButton { text: "Primary"; tone: LV.AbstractButton.Primary }
                    LV.AbstractButton { text: "Default"; tone: LV.AbstractButton.Default }
                    LV.AbstractButton { text: "Borderless"; tone: LV.AbstractButton.Borderless }
                    LV.AbstractButton { text: "Destructive"; tone: LV.AbstractButton.Destructive }
                    LV.AbstractButton { text: "Disabled"; tone: LV.AbstractButton.Disabled }
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: "AbstractButton is the tone and spacing foundation for the rest of the button family."
                }
            }
        }
    }

    Component {
        id: buttonFamilyPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight
            property string selectionLabel: preview.catalogEntry ? preview.catalogEntry.label : "Button"

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.Label {
                    style: caption
                    color: LV.Theme.textTertiary
                    text: "Selected: " + preview.selectionLabel
                }

                GridLayout {
                    columns: 5
                    columnSpacing: LV.Theme.gap12
                    rowSpacing: LV.Theme.gap8

                    LV.Label { style: caption; text: "Accent" }
                    LV.LabelButton { text: "Button" }
                    LV.IconButton { iconName: "projectStructure" }
                    LV.LabelMenuButton { text: "Open" }
                    LV.IconMenuButton { iconName: "projectStructure" }

                    LV.Label { style: caption; text: "Default" }
                    LV.LabelButton { text: "Button"; tone: LV.AbstractButton.Default }
                    LV.IconButton { iconName: "projectStructure"; tone: LV.AbstractButton.Default }
                    LV.LabelMenuButton { text: "Open"; tone: LV.AbstractButton.Default }
                    LV.IconMenuButton { iconName: "projectStructure"; tone: LV.AbstractButton.Default }

                    LV.Label { style: caption; text: "Borderless" }
                    LV.LabelButton { text: "Button"; tone: LV.AbstractButton.Borderless }
                    LV.IconButton { iconName: "projectStructure"; tone: LV.AbstractButton.Borderless }
                    LV.LabelMenuButton { text: "Open"; tone: LV.AbstractButton.Borderless }
                    LV.IconMenuButton { iconName: "projectStructure"; tone: LV.AbstractButton.Borderless }

                    LV.Label { style: caption; text: "Destructive" }
                    LV.LabelButton { text: "Button"; tone: LV.AbstractButton.Destructive }
                    LV.IconButton { iconName: "projectStructure"; tone: LV.AbstractButton.Destructive }
                    LV.LabelMenuButton { text: "Open"; tone: LV.AbstractButton.Destructive }
                    LV.IconMenuButton { iconName: "projectStructure"; tone: LV.AbstractButton.Destructive }

                    LV.Label { style: caption; text: "Disabled" }
                    LV.LabelButton { text: "Button"; tone: LV.AbstractButton.Disabled }
                    LV.IconButton { iconName: "projectStructure"; tone: LV.AbstractButton.Disabled }
                    LV.LabelMenuButton { text: "Open"; tone: LV.AbstractButton.Disabled }
                    LV.IconMenuButton { iconName: "projectStructure"; tone: LV.AbstractButton.Disabled }
                }
            }
        }
    }

    Component {
        id: segmentedControlPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.LabelSegmentedControl {
                    width: implicitWidth

                    LV.LabelButton { text: "Button" }
                    LV.LabelButton { text: "Button" }
                }

                LV.IconSegmentedControl {
                    width: implicitWidth

                    LV.IconButton { iconName: "projectStructure" }
                    LV.IconButton { iconName: "projectStructure" }
                }
            }
        }
    }

    Component {
        id: selectorControlPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap12

                    LV.ComboBox {
                        text: "Primary"
                        tone: LV.ComboBox.Primary
                        arrow: LV.Stepper.UpDown
                    }

                    LV.ComboBox {
                        text: "Borderless"
                        tone: LV.ComboBox.Borderless
                        arrow: LV.Stepper.Down
                    }
                }

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap12

                    LV.Stepper { tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Up }
                    LV.Stepper { tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }
                    LV.Stepper { tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.Down }
                }
            }
        }
    }

    Component {
        id: selectionControlPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap16

                    LV.CheckBox { text: "Remember"; checked: true }
                    LV.RadioButton { text: "Choice A"; checked: true }
                    LV.ToggleSwitch { checked: true }
                    LV.ToggleSwitch { checked: false }
                }

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap16

                    LV.CheckBox { text: "Disabled"; enabled: false; checked: true }
                    LV.RadioButton { text: "Choice B"; enabled: false }
                    LV.ToggleSwitch { text: "Disabled"; enabled: false; checked: false }
                }
            }
        }
    }

    Component {
        id: labelDisplayPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: labelColumn.implicitHeight

            Column {
                id: labelColumn
                width: parent.width
                spacing: LV.Theme.gap4

                LV.Label { style: title; text: "Title" }
                LV.Label { style: title2; text: "Title2" }
                LV.Label { style: header; text: "Header" }
                LV.Label { style: header2; text: "Header2" }
                LV.Label { style: body; text: "Body" }
                LV.Label { style: description; text: "Description" }
                LV.Label { style: caption; text: "Caption" }
                LV.Label { style: disabled; text: "Disabled" }
            }
        }
    }

    Component {
        id: progressDisplayPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.ProgressBar {
                    width: parent.width
                    size: large
                    currentValue: 72
                    endValue: 100
                }

                LV.ProgressBar {
                    width: parent.width
                    size: regular
                    currentValue: 36
                    endValue: 100
                }
            }
        }
    }

    Component {
        id: tableDisplayPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Rectangle {
                    width: parent.width
                    implicitHeight: 168
                    radius: LV.Theme.radiusMd
                    color: LV.Theme.surfaceAlt
                    border.width: 1
                    border.color: LV.Theme.contextMenuDivider

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: root.catalogContentInset
                        contentWidth: table.width
                        contentHeight: table.implicitHeight
                        interactive: contentWidth > width
                        flickableDirection: Flickable.HorizontalFlick
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        LV.Table {
                            id: table
                            width: 528
                            columns: [
                                { label: "Name", type: "string" },
                                { label: "Count", type: "int" },
                                { label: "Enabled", type: "bool" }
                            ]
                            model: [
                                [{ value: "Renderer" }, { value: 8 }, { value: true }],
                                [{ value: "Input" }, { value: 5 }, { value: true }],
                                [{ value: "Metrics" }, { value: 3 }, { value: false }],
                                [{ value: "Tools" }, { value: 2 }, { value: true }]
                            ]
                            sortingEnabled: true
                        }
                    }
                }

                LV.TableHeader {
                    width: implicitWidth
                    cellItems: [{ label: "Column" }, { label: "Column" }, { label: "Column" }]
                }

                LV.TableRow {
                    width: implicitWidth
                    cellItems: ["Text", "Text", "Text"]
                }
            }
        }
    }

    Component {
        id: inputFieldPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            readonly property var inputStates: [
                { kind: "default", enabled: true, text: "", search: false },
                { kind: "disabled", enabled: false, text: "", search: false },
                { kind: "cursor", enabled: true, text: "Placeholder", search: false },
                { kind: "selected", enabled: true, text: "Placeholder", search: false },
                { kind: "active", enabled: true, text: "Placeholder", search: false },
                { kind: "search", enabled: true, text: "Placeholder", search: true }
            ]
            implicitHeight: inputStylesRow.implicitHeight

            Row {
                id: inputStylesRow
                spacing: LV.Theme.gap8

                Repeater {
                    model: ["rounded", "cylinder", "inline"]

                    delegate: Column {
                        id: styleColumn
                        required property string modelData
                        spacing: LV.Theme.gap4

                        LV.Label {
                            style: description
                            text: styleColumn.modelData === "rounded"
                                ? "Rounded"
                                : styleColumn.modelData === "cylinder"
                                    ? "Cylinder"
                                    : "Inline"
                        }

                        Repeater {
                            model: preview.inputStates

                            delegate: LV.InputField {
                                id: stateField
                                required property var modelData
                                style: styleColumn.modelData === "cylinder"
                                    ? cylinderStyle
                                    : styleColumn.modelData === "inline"
                                        ? inlineStyle
                                        : roundedStyle
                                enabled: modelData.enabled
                                placeholderText: "Placeholder"
                                text: modelData.text
                                search: modelData.search
                                persistentSelection: modelData.kind === "selected"

                                Component.onCompleted: {
                                    if (modelData.kind === "selected")
                                        selectAll()
                                    if (modelData.kind === "cursor" || modelData.kind === "selected")
                                        inputItem.cursorVisible = true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: textEditorPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: editorColumn.implicitHeight

            Column {
                id: editorColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.TextEditor {
                    width: parent.width
                    filePath: "/tmp/lvrs-visual-catalog-notes.txt"
                    chunkSize: 65536
                    editorHeight: 180
                    placeholderText: "Open or create /tmp/lvrs-visual-catalog-notes.txt"
                }
            }
        }
    }

    Component {
        id: codeEditorPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: codeColumn.implicitHeight

            Column {
                id: codeColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.CodeEditor {
                    width: parent.width
                    editorHeight: 200
                    readOnly: true
                    snippetTitle: "Example"
                    snippetLanguage: "qml"
                    text: "import QtQuick\nimport LVRS as LV\n\nLV.LabelButton {\n    text: \"Open\"\n    tone: LV.AbstractButton.Primary\n}"
                }
            }
        }
    }

    Component {
        id: eventListenerPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            property string lastEvent: "No event captured yet"
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Rectangle {
                    id: targetArea
                    width: parent.width
                    height: 180
                    radius: LV.Theme.radiusMd
                    color: LV.Theme.surfaceAlt
                    border.width: 1
                    border.color: LV.Theme.contextMenuDivider

                    LV.Label {
                        anchors.centerIn: parent
                        style: body
                        color: LV.Theme.textPrimary
                        text: "Click or press inside this area"
                    }

                    LV.EventListener {
                        trigger: "pressed"
                        action: function(eventData) {
                            preview.lastEvent = "pressed at " + Math.round(eventData.x) + ", " + Math.round(eventData.y)
                        }
                    }

                    LV.EventListener {
                        trigger: "clicked"
                        action: function(eventData) {
                            preview.lastEvent = "clicked at " + Math.round(eventData.x) + ", " + Math.round(eventData.y)
                        }
                    }
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: preview.lastEvent
                }
            }
        }
    }

    Component {
        id: guardUtilityPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.InputField {
                    id: guardedField
                    width: parent.width
                    placeholderText: "IME guard target"
                    text: "Compose here"
                }

                LV.InputMethodGuard {
                    target: guardedField.inputItem
                    guardEnabled: true
                }

                Rectangle {
                    width: parent.width
                    height: 152
                    radius: LV.Theme.radiusMd
                    color: LV.Theme.surfaceAlt
                    border.width: 1
                    border.color: LV.Theme.contextMenuDivider

                    Flickable {
                        id: guardFlickable
                        anchors.fill: parent
                        anchors.margins: root.catalogContentInset
                        contentWidth: width
                        contentHeight: guardColumn.implicitHeight
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        Column {
                            id: guardColumn
                            width: guardFlickable.width
                            spacing: LV.Theme.gap6

                            Repeater {
                                model: 10

                                delegate: LV.Label {
                                    required property int index
                                    width: parent.width
                                    style: body
                                    color: LV.Theme.textPrimary
                                    text: "Scrollable row " + (index + 1)
                                }
                            }
                        }
                    }

                    LV.WheelScrollGuard {
                        anchors.fill: parent
                        targetFlickable: guardFlickable
                        consumeInside: true
                    }
                }
            }
        }
    }

    Component {
        id: routerNavigationPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: routerColumn.implicitHeight

            Column {
                id: routerColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Row {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    LV.Link {
                        router: previewRouter
                        href: "/"
                        text: "Home"
                    }

                    LV.Link {
                        router: previewRouter
                        href: "/reports"
                        text: "Reports"
                    }

                    LV.LabelButton {
                        text: "Navigator.go"
                        tone: LV.AbstractButton.Default
                        onClicked: LV.Navigator.go("/reports")
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 180
                    radius: LV.Theme.radiusMd
                    color: LV.Theme.surfaceAlt
                    border.width: 1
                    border.color: LV.Theme.contextMenuDivider

                    LV.PageRouter {
                        id: previewRouter
                        anchors.fill: parent
                        anchors.margins: root.catalogContentInset
                        registerAsGlobalNavigator: true
                        initialPath: "/"
                        routes: [
                            { path: "/", component: routerHomePage },
                            { path: "/reports", component: routerReportsPage }
                        ]
                    }

                    Component {
                        id: routerHomePage

                        Rectangle {
                            color: LV.Theme.surfaceGhost
                            radius: LV.Theme.radiusSm

                            LV.Label {
                                anchors.centerIn: parent
                                style: header2
                                color: LV.Theme.textPrimary
                                text: "Home Route"
                            }
                        }
                    }

                    Component {
                        id: routerReportsPage

                        Rectangle {
                            color: LV.Theme.accentOverlay
                            radius: LV.Theme.radiusSm

                            LV.Label {
                                anchors.centerIn: parent
                                style: header2
                                color: LV.Theme.textPrimary
                                text: "Reports Route"
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: hierarchyNavigationPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            property var editableHierarchyModel: createEditableHierarchyModel()
            property string dragEventSummary: "Drag rows vertically to reorder them, then move right in 8 px steps to redefine parent and depth."
            property string hierarchyArraySummary: ""
            property string activeItemSummary: "Select or drag a generated row to inspect the item-owned hierarchy API."
            implicitHeight: contentColumn.implicitHeight

            function createEditableHierarchyModel() {
                return [
                    { key: "scene", depth: 0, label: "Scene", expanded: true, iconGlyph: "S", parentKey: "", parentItemKey: "" },
                    { key: "camera", depth: 1, label: "Camera", expanded: true, iconGlyph: "C", parentKey: "scene", parentItemKey: "scene" },
                    { key: "frustum", depth: 2, label: "Frustum", iconGlyph: "F", parentKey: "camera", parentItemKey: "camera" },
                    { key: "lights", depth: 1, label: "Lights", expanded: true, iconGlyph: "L", parentKey: "scene", parentItemKey: "scene" },
                    { key: "key", depth: 2, label: "Key Light", iconGlyph: "K", parentKey: "lights", parentItemKey: "lights" },
                    { key: "rim", depth: 2, label: "Rim Light", iconGlyph: "R", parentKey: "lights", parentItemKey: "lights" }
                ]
            }

            function resolvedNodeParentKey(node) {
                if (!node)
                    return "root"

                const parentItemKey = node.parentItemKey === undefined || node.parentItemKey === null
                    ? ""
                    : String(node.parentItemKey)
                if (parentItemKey.length > 0)
                    return parentItemKey

                const parentKey = node.parentKey === undefined || node.parentKey === null ? "" : String(node.parentKey)
                return parentKey.length > 0 ? parentKey : "root"
            }

            function syncHierarchyArraySummary() {
                const fragments = []
                for (let i = 0; i < editableHierarchyModel.length; i++) {
                    const node = editableHierarchyModel[i]
                    if (!node)
                        continue
                    const key = node.key === undefined || node.key === null ? "node-" + i : String(node.key)
                    const depth = Number(node.depth)
                    const normalizedDepth = Number.isFinite(depth) ? Math.max(0, Math.trunc(depth)) : 0
                    fragments.push(key + " d" + normalizedDepth + " <- " + resolvedNodeParentKey(node))
                }
                hierarchyArraySummary = fragments.join(" | ")
            }

            function syncActiveItemSummary() {
                const item = hierarchyPreview.activeListItem
                if (!item) {
                    activeItemSummary = "Select or drag a generated row to inspect the item-owned hierarchy API."
                    return
                }

                const itemKey = item.resolvedItemKey === undefined || item.resolvedItemKey === null
                    ? ""
                    : String(item.resolvedItemKey)
                const parentKey = item.parentItemKey === undefined || item.parentItemKey === null || String(item.parentItemKey).length === 0
                    ? "root"
                    : String(item.parentItemKey)
                const dragMode = item.dragTargetModeName === undefined || item.dragTargetModeName === null || String(item.dragTargetModeName).length === 0
                    ? "none"
                    : String(item.dragTargetModeName)
                activeItemSummary = itemKey
                    + " depth " + item.indentLevel
                    + ", parent " + parentKey
                    + ", dragEnabled " + item.dragEnabled
                    + ", canToggleExpanded " + item.canToggleExpanded
                    + ", dragTarget " + dragMode
            }

            function resetEditableHierarchyModel() {
                editableHierarchyModel = createEditableHierarchyModel()
                dragEventSummary = "Drag rows vertically to reorder them, then move right in 8 px steps to redefine parent and depth."
                syncHierarchyArraySummary()
                Qt.callLater(preview.syncActiveItemSummary)
            }

            Component.onCompleted: syncHierarchyArraySummary()

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.HierarchyToolbar {
                    width: parent.width
                    buttonItems: [
                        { id: "overview", iconName: "projectStructure" },
                        { id: "expand", iconName: "viewMoreSymbolicDefault" },
                        { id: "collapse", iconName: "panDownSymbolicDefault" }
                    ]
                }

                LV.HierarchyItem {
                    width: parent.width
                    label: "Generated rows emit drag signals from the item itself"
                    itemKey: "delegate-contract"
                    showChevron: true
                    hasChildItems: true
                    expanded: false
                    iconGlyph: "I"
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: "Editable hierarchy is now a flat depth-array demo. Drag a row up or down to move it, then push right by one indent step to reparent it as a child."
                }

                LV.Hierarchy {
                    id: hierarchyPreview
                    width: parent.width
                    height: 240
                    editable: true
                    model: preview.editableHierarchyModel

                    onListItemActivated: function(item) {
                        preview.syncActiveItemSummary()
                    }
                    onListItemMoved: function(item, itemId, itemKey, fromIndex, toIndex, depth) {
                        preview.dragEventSummary = String(itemKey) + " moved from " + fromIndex + " to " + toIndex + " at depth " + depth + "."
                        preview.syncHierarchyArraySummary()
                        Qt.callLater(preview.syncActiveItemSummary)
                    }
                    onToolbarButtonTriggered: function(button, buttonId) {
                        if (buttonId === "overview")
                            hierarchyPreview.activateListItemByKey("scene")
                        else if (buttonId === "expand")
                            hierarchyPreview.expandAll()
                        else if (buttonId === "collapse")
                            hierarchyPreview.collapseAll(true)
                        else if (buttonId === "reset")
                            preview.resetEditableHierarchyModel()
                    }

                    LV.ToolbarButton { buttonId: "overview"; iconGlyph: "O" }
                    LV.ToolbarButton { buttonId: "expand"; iconGlyph: "+" }
                    LV.ToolbarButton { buttonId: "collapse"; iconGlyph: "-" }
                    LV.ToolbarButton { buttonId: "reset"; iconGlyph: "R" }
                }

                Connections {
                    target: hierarchyPreview.activeListItem
                    ignoreUnknownSignals: true

                    function onDragStarted(sourceIndex, sourceEndIndex, sourceDepth) {
                        const item = hierarchyPreview.activeListItem
                        const label = item && item.resolvedLabel !== undefined && item.resolvedLabel !== null
                            ? String(item.resolvedLabel)
                            : "Row"
                        preview.dragEventSummary = label + " drag start [" + sourceIndex + "-" + sourceEndIndex + "] depth " + sourceDepth + "."
                        preview.syncActiveItemSummary()
                    }

                    function onDragUpdated(targetIndex, targetDepth, modeName, parentItemKey, anchorItemKey) {
                        const parentKey = parentItemKey && String(parentItemKey).length > 0 ? String(parentItemKey) : "root"
                        const anchorKey = anchorItemKey && String(anchorItemKey).length > 0 ? String(anchorItemKey) : "none"
                        preview.dragEventSummary = "Preview " + String(modeName) + " -> index " + targetIndex + ", depth " + targetDepth + ", parent " + parentKey + ", anchor " + anchorKey + "."
                        preview.syncActiveItemSummary()
                    }

                    function onDragEnded(committed, fromIndex, toIndex, targetDepth, modeName, parentItemKey, anchorItemKey) {
                        const parentKey = parentItemKey && String(parentItemKey).length > 0 ? String(parentItemKey) : "root"
                        const anchorKey = anchorItemKey && String(anchorItemKey).length > 0 ? String(anchorItemKey) : "none"
                        preview.dragEventSummary = committed
                            ? "Committed " + String(modeName) + " move from " + fromIndex + " to " + toIndex + " at depth " + targetDepth + " under " + parentKey + " with anchor " + anchorKey + "."
                            : "Canceled drag from " + fromIndex + "."
                        preview.syncHierarchyArraySummary()
                        Qt.callLater(preview.syncActiveItemSummary)
                    }
                }

                LV.Label {
                    width: parent.width
                    style: caption
                    color: LV.Theme.textTertiary
                    text: "Active Item API"
                }

                LV.Label {
                    width: parent.width
                    style: body
                    color: LV.Theme.textPrimary
                    wrapMode: Text.WordWrap
                    text: preview.activeItemSummary
                }

                LV.Label {
                    width: parent.width
                    style: caption
                    color: LV.Theme.textTertiary
                    text: "Last Drag Event"
                }

                LV.Label {
                    width: parent.width
                    style: body
                    color: LV.Theme.textPrimary
                    wrapMode: Text.WordWrap
                    text: preview.dragEventSummary
                }

                LV.Label {
                    width: parent.width
                    style: caption
                    color: LV.Theme.textTertiary
                    text: "Bound Depth Array"
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: preview.hierarchyArraySummary
                }
            }
        }
    }

    Component {
        id: listNavigationPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.ListToolbar {
                    width: implicitWidth
                    icon1: "projectStructure"
                    icon2: "add"
                    icon3: "viewMoreSymbolicDefault"
                }

                LV.ListItem {
                    width: parent.width
                    label: "Inspector row"
                }

                LV.List {
                    width: 220
                    height: 220
                    items: [
                        { label: "Overview" },
                        { label: "Reports" },
                        { label: "Settings" }
                    ]
                }

                LV.ListFooter {
                    button1: ({ type: "icon", iconName: "projectStructure" })
                    button2: ({ type: "menu", iconName: "add" })
                    button3: ({ type: "icon", iconName: "viewMoreSymbolicDefault" })
                }
            }
        }
    }

    Component {
        id: menuNavigationPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.MenuItem {
                    width: 240
                    label: "Open Recent"
                    key: "Cmd+O"
                    keyVisible: true
                    showChevron: true
                    hasChildItems: true
                    expanded: false
                    state: selectedState
                }

                LV.MenuDivider {
                    width: parent.width
                    axis: "horizontal"
                }

                LV.LabelButton {
                    id: menuOpenButton
                    text: "Open ContextMenu"
                    tone: LV.AbstractButton.Default
                    onClicked: previewMenu.openAt(menuOpenButton.x, menuOpenButton.y + menuOpenButton.height + LV.Theme.gap8)
                }

                LV.ContextMenu {
                    id: previewMenu
                    items: [
                        { label: "Inspect", eventName: "inspect" },
                        { type: "divider" },
                        { label: "Copy Label", key: "Cmd+C", showChevron: false }
                    ]
                }
            }
        }
    }

    Component {
        id: appCardSurfacePreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: card.implicitHeight

            LV.AppCard {
                id: card
                width: parent.width
                title: "System Health"
                subtitle: "Reusable surface"

                Column {
                    width: parent.width
                    spacing: LV.Theme.gap6

                    LV.Label {
                        width: parent.width
                        style: body
                        color: LV.Theme.textPrimary
                        text: "AppCard keeps header, padding, and content spacing consistent."
                        wrapMode: Text.WordWrap
                    }

                    LV.ProgressBar {
                        width: parent.width
                        currentValue: 84
                        endValue: 100
                    }
                }
            }
        }
    }

    Component {
        id: alertSurfacePreview

        Item {
            id: preview
            property var catalogEntry: ({})
            property bool alertOpen: false
            property int alertButtonCount: 2
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    LV.AlertButton {
                        text: "Primary"
                        tone: LV.AbstractButton.Primary
                    }

                    LV.AlertButton {
                        text: "Default"
                        tone: LV.AbstractButton.Default
                    }

                    LV.LabelButton {
                        text: "Open two-action alert"
                        tone: LV.AbstractButton.Default
                        onClicked: {
                            preview.alertButtonCount = 2
                            preview.alertOpen = true
                        }
                    }

                    LV.LabelButton {
                        text: "Open three-action alert"
                        tone: LV.AbstractButton.Default
                        onClicked: {
                            preview.alertButtonCount = 3
                            preview.alertOpen = true
                        }
                    }
                }

                LV.Alert {
                    open: preview.alertOpen
                    buttonCount: preview.alertButtonCount
                    title: buttonCount === 3 ? "Save changes?" : "Apply changes?"
                    message: buttonCount === 3
                        ? "You have unsaved changes.\nSave them before closing?"
                        : "Your new settings will be applied\nto this workspace."
                    primaryText: buttonCount === 3 ? "Save changes" : "Apply changes"
                    secondaryText: buttonCount === 3 ? "Discard changes" : "Cancel"
                    tertiaryText: buttonCount === 3 ? "Cancel" : ""
                    onPrimaryClicked: preview.alertOpen = false
                    onSecondaryClicked: preview.alertOpen = false
                    onTertiaryClicked: preview.alertOpen = false
                    onDismissed: preview.alertOpen = false
                }
            }
        }
    }

    Component {
        id: modalSurfacePreview

        Item {
            id: preview
            property var catalogEntry: ({})
            property bool modalOpen: false
            implicitHeight: contentColumn.implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.catalogSectionGap

                LV.LabelButton {
                    text: "Open Modal"
                    tone: LV.AbstractButton.Default
                    onClicked: preview.modalOpen = true
                }

                LV.Modal {
                    open: preview.modalOpen
                    title: "Continue?"
                    description: "The modal preview uses the live component so overlay and action layout behavior stay visible."
                    iconName: "projectStructure"
                    primaryText: "Continue"
                    secondaryText: "Cancel"
                    onPrimaryClicked: preview.modalOpen = false
                    onSecondaryClicked: preview.modalOpen = false
                    onCanceled: preview.modalOpen = false
                }
            }
        }
    }

    Component {
        id: placeholderPreview

        Item {
            id: preview
            property var catalogEntry: ({})
            implicitHeight: 96

            LV.Label {
                anchors.fill: parent
                style: description
                color: LV.Theme.textSecondary
                wrapMode: Text.WordWrap
                text: "No dedicated live preview is registered for this entry yet."
            }
        }
    }

    Item {
        x: root.catalogViewport.x
        y: root.catalogViewport.y
        width: root.catalogViewport.width
        height: root.catalogViewport.height

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.catalogOuterMargin
            spacing: root.catalogSectionGap

            LV.AppCard {
                Layout.fillWidth: true
                implicitHeight: root.catalogCompactLayout ? 158 : 126

                Column {
                    anchors.fill: parent
                    anchors.margins: root.catalogContentInset
                    spacing: LV.Theme.gap6

                    LV.Label {
                        style: title2
                        color: LV.Theme.textPrimary
                        text: "Visual Documentation Browser"
                    }

                    LV.Label {
                        width: parent.width
                        style: description
                        color: LV.Theme.textSecondary
                        wrapMode: Text.WordWrap
                        text: "The left hierarchy is the primary index. Section entries summarize domains, and component entries open dedicated reference pages with live LVRS previews."
                    }

                    Flow {
                        width: parent.width
                        spacing: LV.Theme.gap8

                        Repeater {
                            model: [
                                { label: root.catalogComponentCount + " public types" },
                                { label: root.catalogDocumentCount + " catalog entries" },
                                { label: "health " + root.metricsSummary },
                                { label: root.activeEntry ? root.activeEntry.roleLabel : "Overview" }
                            ]

                            delegate: Rectangle {
                                required property var modelData
                                radius: LV.Theme.radiusSm
                                color: LV.Theme.surfaceAlt
                                border.width: 1
                                border.color: LV.Theme.contextMenuDivider
                                implicitWidth: chipLabel.implicitWidth + LV.Theme.gap12
                                implicitHeight: chipLabel.implicitHeight + LV.Theme.gap6

                                LV.Label {
                                    id: chipLabel
                                    anchors.centerIn: parent
                                    style: caption
                                    color: LV.Theme.textPrimary
                                    text: modelData.label
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.catalogSectionGap

                ColumnLayout {
                    Layout.preferredWidth: root.sidebarWidth
                    Layout.minimumWidth: 240
                    Layout.maximumWidth: 360
                    Layout.fillHeight: true
                    spacing: root.catalogSectionGap

                    LV.AppCard {
                        Layout.fillWidth: true
                        implicitHeight: 124

                        Column {
                            anchors.fill: parent
                            anchors.margins: root.catalogContentInset
                            spacing: LV.Theme.gap4

                            LV.Label {
                                style: header2
                                color: LV.Theme.textPrimary
                                text: "Component Index"
                            }

                            LV.Label {
                                width: parent.width
                                style: description
                                color: LV.Theme.textSecondary
                                wrapMode: Text.WordWrap
                                text: "Use O for overview, + to expand, and - to collapse the full tree."
                            }
                        }
                    }

                    LV.Hierarchy {
                        id: catalogHierarchy
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        minimumPanelWidth: root.sidebarWidth
                        model: catalogRegistry.hierarchyModel
                        footerVisible: false

                        onListItemActivated: function(item) {
                            if (item && item.itemKey)
                                root.activeEntryKey = String(item.itemKey)
                        }

                        onToolbarButtonTriggered: function(button, buttonId) {
                            if (buttonId === "overview")
                                root.activateCatalogEntry(catalogRegistry.overview.key)
                            else if (buttonId === "expand")
                                catalogHierarchy.expandAll()
                            else if (buttonId === "collapse")
                                catalogHierarchy.collapseAll(true)
                        }

                        LV.ToolbarButton {
                            buttonId: "overview"
                            iconGlyph: "O"
                        }

                        LV.ToolbarButton {
                            buttonId: "expand"
                            iconGlyph: "+"
                        }

                        LV.ToolbarButton {
                            buttonId: "collapse"
                            iconGlyph: "-"
                        }
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: Math.max(parent.width, 720)
                        spacing: root.catalogSectionGap

                        LV.AppCard {
                            Layout.fillWidth: true
                            implicitHeight: heroColumn.implicitHeight + root.catalogContentInset * 2

                            Column {
                                id: heroColumn
                                x: root.catalogContentInset
                                y: root.catalogContentInset
                                width: parent.width - root.catalogContentInset * 2
                                spacing: LV.Theme.gap6

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: root.breadcrumbText(root.activeBreadcrumb)
                                }

                                LV.Label {
                                    style: title
                                    color: LV.Theme.textPrimary
                                    text: root.activeEntry ? root.activeEntry.label : "Catalog Overview"
                                }

                                LV.Label {
                                    style: description
                                    color: LV.Theme.textSecondary
                                    text: root.activeEntry ? root.activeEntry.roleLabel : "Overview"
                                }

                                LV.Label {
                                    width: parent.width
                                    style: body
                                    color: LV.Theme.textPrimary
                                    wrapMode: Text.WordWrap
                                    text: root.activeEntry ? root.activeEntry.summary : ""
                                }
                            }
                        }

                        LV.AppCard {
                            Layout.fillWidth: true
                            title: "Live Preview"
                            subtitle: root.activeEntry ? root.activeEntry.label : ""
                            implicitHeight: Math.max(root.catalogCompactLayout ? 320 : 360, (previewLoader.item && previewLoader.item.implicitHeight ? previewLoader.item.implicitHeight : 0) + root.catalogCardInset * 2)

                            Item {
                                anchors.fill: parent
                                anchors.margins: root.catalogCardInset

                                Loader {
                                    id: previewLoader
                                    anchors.fill: parent
                                    sourceComponent: root.previewComponentFor(root.activeEntry ? root.activeEntry.previewId : "")
                                    onLoaded: {
                                        if (item && item.catalogEntry !== undefined)
                                            item.catalogEntry = root.activeEntry
                                    }
                                }
                            }
                        }

                        LV.AppCard {
                            Layout.fillWidth: true
                            visible: root.activeEntry && root.activeEntry.usage.length > 0
                            title: "Usage"
                            subtitle: "Canonical QML snippet"
                            implicitHeight: codePreview.editorHeight + root.catalogCardInset * 2

                            LV.CodeEditor {
                                id: codePreview
                                x: root.catalogCardInset
                                y: root.catalogCardInset
                                width: parent.width - root.catalogCardInset * 2
                                readOnly: true
                                editorHeight: Math.max(160, Math.min(260, contentHeight + LV.Theme.gap20))
                                snippetTitle: root.activeEntry ? root.activeEntry.label + ".qml" : "usage.qml"
                                snippetLanguage: "qml"
                                text: root.activeEntry ? root.activeEntry.usage : ""
                            }
                        }

                        LV.AppCard {
                            Layout.fillWidth: true
                            title: "Structure"
                            subtitle: "Source, related types, and branch contents"
                            implicitHeight: structureColumn.implicitHeight + root.catalogCardInset * 2

                            Column {
                                id: structureColumn
                                x: root.catalogCardInset
                                y: root.catalogCardInset
                                width: parent.width - root.catalogCardInset * 2
                                spacing: root.catalogSectionGap

                            Column {
                                width: parent.width
                                spacing: LV.Theme.gap4

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: "Source Path"
                                }

                                LV.Label {
                                    width: parent.width
                                    style: body
                                    color: LV.Theme.textPrimary
                                    wrapMode: Text.WrapAnywhere
                                    text: root.activeEntry && root.activeEntry.location.length > 0 ? root.activeEntry.location : "Catalog section"
                                }
                            }

                            Column {
                                width: parent.width
                                spacing: LV.Theme.gap4
                                visible: root.activeEntry && root.activeEntry.docPath.length > 0

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: "Documentation Path"
                                }

                                LV.Label {
                                    width: parent.width
                                    style: body
                                    color: LV.Theme.textPrimary
                                    wrapMode: Text.WrapAnywhere
                                    text: root.activeEntry ? root.activeEntry.docPath : ""
                                }
                            }

                            Column {
                                width: parent.width
                                spacing: LV.Theme.gap6
                                visible: root.activeRelated.length > 0

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: "Related Types"
                                }

                                Flow {
                                    width: parent.width
                                    spacing: LV.Theme.gap8

                                    Repeater {
                                        model: root.activeRelated

                                        delegate: LV.LabelButton {
                                            required property var modelData
                                            text: modelData.label
                                            tone: LV.AbstractButton.Default
                                            onClicked: root.activateCatalogEntry(modelData.key)
                                        }
                                    }
                                }
                            }

                            Column {
                                width: parent.width
                                spacing: LV.Theme.gap6
                                visible: root.activeChildren.length > 0

                                LV.Label {
                                    style: caption
                                    color: LV.Theme.textTertiary
                                    text: "Branch Contents"
                                }

                                Flow {
                                    width: parent.width
                                    spacing: LV.Theme.gap8

                                    Repeater {
                                        model: root.activeChildren

                                        delegate: LV.LabelButton {
                                            required property var modelData
                                            text: modelData.label
                                            tone: LV.AbstractButton.Default
                                            onClicked: root.activateCatalogEntry(modelData.key)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    LV.AppCard {
                        Layout.fillWidth: true
                        visible: root.activeEntry && root.activeEntry.notes.length > 0
                        title: "Notes"
                        subtitle: "Behavior and positioning guidance"
                        implicitHeight: notesColumn.implicitHeight + root.catalogCardInset * 2

                        Column {
                            id: notesColumn
                            x: root.catalogCardInset
                            y: root.catalogCardInset
                            width: parent.width - root.catalogCardInset * 2
                            spacing: LV.Theme.gap6

                            Repeater {
                                model: root.activeEntry ? root.activeEntry.notes : []

                                delegate: LV.Label {
                                    required property var modelData
                                    width: parent.width
                                    style: description
                                    color: LV.Theme.textSecondary
                                    wrapMode: Text.WordWrap
                                    text: modelData
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
}

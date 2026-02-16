pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LVRS as LV

LV.ApplicationWindow {
    id: root

    visible: true
    width: 1480
    height: 980
    desktopMinWidth: 360
    desktopMinHeight: 520
    usePlatformSafeMargin: true
    autoAttachRuntimeEvents: true
    autoHookBackendUserEvents: false
    globalEventListenersEnabled: true
    scaffoldLayoutMode: "auto"
    scaffoldPreferBottomNavigation: true
    scaffoldBottomNavigationMaxItems: 5
    scaffoldCompactSpacingEnabled: true
    scaffoldCompactSpacingBreakpoint: 1040
    scaffoldNavRailMaxWidthRatio: 0.3
    scaffoldDrawerMarginSafety: LV.Theme.gap24
    title: "LVRS Visual Catalog"
    subtitle: "Developer-focused design system console"
    navItems: LV.AppState.navItems
    readonly property bool catalogCompactLayout: matchesMedia("mobile-layout") || isCompact
    readonly property int catalogOuterMargin: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap16
    readonly property int catalogSectionGap: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap12
    readonly property int catalogContentInset: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap12
    readonly property int catalogCardInset: catalogCompactLayout ? LV.Theme.gap8 : LV.Theme.gap12

    readonly property var accentPreviewTokens: LV.Theme.accentPaletteTokens

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

    readonly property var pageMeta: [
        {
            tab: "Overview",
            title: "Runtime + Metrics Overview",
            doc: "Entry page for immediate validation of theme, runtime snapshot, and global overlays."
        },
        {
            tab: "Typography",
            title: "Label Typography",
            doc: "Verification panel for all label styles and fallback policy."
        },
        {
            tab: "Buttons",
            title: "Button Families",
            doc: "Primary/default/borderless/destructive/disabled combinations and icon variants."
        },
        {
            tab: "Table",
            title: "Table Components",
            doc: "Figma-based table composition: Table + Header + Row + CellItem primitives."
        },
        {
            tab: "Inputs",
            title: "Input Components",
            doc: "InputField/TextEditor/CodeEditor behavior with fixed editor heights."
        },
        {
            tab: "Navigation",
            title: "Hierarchy Navigation",
            doc: "Tree-model rendering with chevron-only expand/collapse behavior."
        },
        {
            tab: "Accent",
            title: "Accent Token Audit",
            doc: "Color token preview generated from icon-set driven accent palette."
        },
        {
            tab: "Runtime",
            title: "Event Monitor",
            doc: "Global click/context traces and backend event tail monitor for developers."
        }
    ]

    property int activeTabIndex: 0
    property var runtimeRows: []
    property int runtimeMaxRows: 160
    property int lastRuntimeSequence: -1
    property string runtimeFilter: "all"
    property var runtimeHealth: ({})
    property var lastGlobalPress: ({})
    property var lastGlobalContext: ({})
    property var activeContextMenuItems: LV.AppState.demoContextMenuItems

    readonly property int runtimeVisibleCount: runtimeFilteredCount(runtimeFilter)

    function runtimeFilteredCount(filterValue) {
        if (filterValue === "all")
            return runtimeRows.length
        let count = 0
        for (let i = 0; i < runtimeRows.length; i++) {
            const row = runtimeRows[i]
            if (row && String(row.category) === filterValue)
                count += 1
        }
        return count
    }

    function responsiveColumns(availableWidth, minimumCellWidth, minimumColumns, maximumColumns) {
        const widthValue = Math.max(1, Number(availableWidth || 0))
        const cellWidth = Math.max(1, Number(minimumCellWidth || 1))
        const lowerBound = Math.max(1, Number(minimumColumns || 1))
        let resolved = Math.max(lowerBound, Math.floor(widthValue / cellWidth))

        if (maximumColumns !== undefined && maximumColumns !== null) {
            const upperBound = Number(maximumColumns)
            if (upperBound > 0)
                resolved = Math.min(resolved, upperBound)
        }
        return resolved
    }

    function categoryForEvent(type) {
        const normalized = String(type || "")
        if (normalized.indexOf("mouse-") === 0
                || normalized.indexOf("key-") === 0
                || normalized.indexOf("touch-") === 0
                || normalized.indexOf("context-") === 0
                || normalized.indexOf("global-") === 0)
            return "input"
        if (normalized.indexOf("render-") === 0)
            return "render"
        if (normalized.indexOf("route-") === 0 || normalized.indexOf("viewstack-") === 0)
            return "navigation"
        if (normalized.indexOf("daemon-") === 0 || normalized.indexOf("window-") === 0)
            return "runtime"
        return "system"
    }

    function timestamp(epochMs) {
        const ms = Number(epochMs || 0)
        if (ms <= 0)
            return "--:--:--.--"
        const date = new Date(ms)
        const hh = date.getHours() < 10 ? "0" + date.getHours() : String(date.getHours())
        const mm = date.getMinutes() < 10 ? "0" + date.getMinutes() : String(date.getMinutes())
        const ss = date.getSeconds() < 10 ? "0" + date.getSeconds() : String(date.getSeconds())
        const cs = Math.floor(date.getMilliseconds() / 10)
        const cc = cs < 10 ? "0" + cs : String(cs)
        return hh + ":" + mm + ":" + ss + "." + cc
    }

    function appendRuntimeRow(category, source, type, payload, sequence, epochMs) {
        const rows = runtimeRows.slice()
        rows.push({
            category: category,
            source: source,
            type: type,
            payload: payload || ({}),
            sequence: sequence !== undefined ? sequence : -1,
            epochMs: epochMs !== undefined ? epochMs : Date.now()
        })
        if (rows.length > runtimeMaxRows)
            rows.splice(0, rows.length - runtimeMaxRows)
        runtimeRows = rows
    }

    function syncRuntimeState() {
        LV.AppState.syncRuntimeSnapshot(LV.RuntimeEvents.snapshot())
        LV.AppState.syncViewStateSnapshot(LV.ViewStateTracker.snapshot())
        LV.AppState.syncPageHistory(LV.PageMonitor.history)
        runtimeHealth = LV.RuntimeEvents.daemonHealth()
    }

    function ingestBackendTail(maxRows) {
        if (!LV.Backend || !LV.Backend.hookedUserEvents)
            return
        const rows = LV.Backend.hookedUserEvents(maxRows)
        if (!rows || rows.length === undefined)
            return
        for (let i = 0; i < rows.length; i++) {
            const eventData = rows[i]
            const sequence = eventData.sequence !== undefined ? Number(eventData.sequence) : -1
            if (sequence >= 0 && sequence <= lastRuntimeSequence)
                continue
            if (sequence >= 0)
                lastRuntimeSequence = sequence

            const type = String(eventData.type || "unknown")
            if (type === "ui-event" || type === "mouse-move" || type === "hover-move")
                continue

            appendRuntimeRow(
                        categoryForEvent(type),
                        "Backend",
                        type,
                        eventData.payload || ({}),
                        sequence,
                        eventData.timestampEpochMs)
        }
    }

    function contextMenuItemsForEvent(eventData) {
        const ui = eventData && eventData.ui ? eventData.ui : ({})
        const className = ui.className ? String(ui.className) : ""
        const objectName = ui.objectName ? String(ui.objectName) : ""
        const text = ui.text ? String(ui.text) : ""

        if (className.indexOf("TextEdit") !== -1 || objectName.toLowerCase().indexOf("editor") !== -1) {
            return [
                { id: "copy", label: "Copy", key: "Cmd+C", showChevron: false },
                { id: "paste", label: "Paste", key: "Cmd+V", showChevron: false },
                { type: "divider" },
                { id: "select-all", label: "Select All", key: "Cmd+A", showChevron: false }
            ]
        }

        return [
            { id: "inspect", label: "Inspect", key: "", showChevron: false },
            { id: "focus", label: "Focus " + (objectName.length > 0 ? objectName : className || "Target"), key: "", showChevron: false },
            { type: "divider" },
            { id: "copy", label: text.length > 0 ? ("Copy \"" + text + "\"") : "Copy Label", key: "", showChevron: false }
        ]
    }

    function openContextMenuAtGlobal(globalX, globalY) {
        const overlayParent = demoContextMenu ? demoContextMenu.parent : null
        if (overlayParent && overlayParent.mapFromGlobal) {
            const mapped = overlayParent.mapFromGlobal(globalX, globalY)
            demoContextMenu.openAt(mapped.x, mapped.y)
            return
        }
        demoContextMenu.openAt(globalX - root.x, globalY - root.y)
    }

    function dismissContextIfOutside(eventData) {
        if (!demoContextMenu || !demoContextMenu.opened || !demoContextMenu.dismissIfOutsideGlobalEvent)
            return
        demoContextMenu.dismissIfOutsideGlobalEvent(eventData)
    }

    onGlobalPressedEvent: function(eventData) {
        lastGlobalPress = eventData || ({})
        dismissContextIfOutside(eventData)
        appendRuntimeRow("input", "ApplicationWindow", "global-pressed", eventData || ({}), -1, Date.now())
    }

    onGlobalContextEvent: function(eventData) {
        lastGlobalContext = eventData || ({})
        dismissContextIfOutside(eventData)
        appendRuntimeRow("input", "ApplicationWindow", "global-context", eventData || ({}), -1, Date.now())

        if (!eventData)
            return
        const x = eventData.globalX !== undefined ? eventData.globalX : eventData.x
        const y = eventData.globalY !== undefined ? eventData.globalY : eventData.y
        if (x === undefined || y === undefined)
            return

        activeContextMenuItems = contextMenuItemsForEvent(eventData)
        openContextMenuAtGlobal(x, y)
    }

    Component.onCompleted: {
        LV.AppState.bootstrap()
        LV.FontPolicy.enforceApplicationFallback()
        LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.LowLatencyCapture
        LV.RenderMonitor.attachWindow(root)
        LV.PageMonitor.record("/visual-catalog")
        syncRuntimeState()

        if (LV.Backend && LV.Backend.hookUserEvents)
            LV.Backend.hookUserEvents()

        LV.Debug.enabled = true
        LV.Debug.runtimeCaptureEnabled = false
        LV.Debug.runtimeEchoEnabled = false
        LV.Debug.stdoutMinimumLevel = "WARN"
        LV.Debug.stdoutNoiseReductionEnabled = true

        appendRuntimeRow("runtime", "Main", "catalog-opened", { route: LV.AppState.currentRoute }, -1, Date.now())
    }

    Timer {
        interval: 220
        running: true
        repeat: true
        onTriggered: {
            root.syncRuntimeState()
            root.ingestBackendTail(12)
        }
    }

    LV.Alert {
        id: sampleAlert
        anchors.fill: parent
        open: LV.AppState.alertOpen
        title: "Alert Dialog"
        message: "This alert verifies frame, overlay, and action callback behavior."
        primaryText: "Confirm"
        secondaryText: "Cancel"
        tertiaryText: "Ignore"
        onPrimaryClicked: LV.AppState.alertOpen = false
        onSecondaryClicked: LV.AppState.alertOpen = false
        onTertiaryClicked: LV.AppState.alertOpen = false
        onDismissed: LV.AppState.alertOpen = false
    }

    LV.ContextMenu {
        id: demoContextMenu
        items: root.activeContextMenuItems
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.catalogOuterMargin
        spacing: root.catalogSectionGap

        TabBar {
            id: tabBar
            visible: !root.catalogCompactLayout
            Layout.fillWidth: true
            currentIndex: root.activeTabIndex
            onCurrentIndexChanged: {
                if (root.activeTabIndex !== currentIndex)
                    root.activeTabIndex = currentIndex
            }

            Repeater {
                model: root.pageMeta
                TabButton {
                    required property var modelData
                    text: modelData.tab
                }
            }
        }

        Flickable {
            id: compactTabStrip
            visible: root.catalogCompactLayout
            Layout.fillWidth: true
            Layout.preferredHeight: compactTabRow.implicitHeight
            Layout.maximumHeight: compactTabRow.implicitHeight
            contentWidth: compactTabRow.implicitWidth
            contentHeight: compactTabRow.implicitHeight
            interactive: contentWidth > width
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            Row {
                id: compactTabRow
                spacing: LV.Theme.gap6

                Repeater {
                    model: root.pageMeta

                    delegate: LV.LabelButton {
                        required property int index
                        required property var modelData
                        text: modelData.tab
                        tone: root.activeTabIndex === index ? LV.AbstractButton.Primary : LV.AbstractButton.Default
                        onClicked: root.activeTabIndex = index
                    }
                }
            }
        }

        LV.AppCard {
            Layout.fillWidth: true
            implicitHeight: root.catalogCompactLayout ? 136 : 108

            Column {
                anchors.fill: parent
                anchors.margins: root.catalogContentInset
                spacing: LV.Theme.gap6

                LV.Label {
                    style: header
                    color: LV.Theme.textPrimary
                    text: root.pageMeta[root.activeTabIndex].title
                }

                LV.Label {
                    width: parent.width
                    style: description
                    color: LV.Theme.textSecondary
                    wrapMode: Text.WordWrap
                    text: root.pageMeta[root.activeTabIndex].doc
                }

                LV.Label {
                    style: caption
                    color: root.metricsPass ? LV.Theme.accentGreenBright : LV.Theme.accentRose
                    text: "Design-health metrics: " + root.metricsSummary
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.activeTabIndex

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: LV.Theme.gap12

                    LV.AppCard {
                        Layout.fillWidth: true
                        implicitHeight: root.catalogCompactLayout ? 216 : 132

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: root.catalogContentInset
                            columns: root.catalogCompactLayout ? 1 : 3
                            rowSpacing: LV.Theme.gap8
                            columnSpacing: LV.Theme.gap16

                            Column {
                                Layout.fillWidth: true
                                spacing: LV.Theme.gap4
                                LV.Label { style: header2; color: LV.Theme.textPrimary; text: "Core Runtime" }
                                LV.Label { style: description; color: LV.Theme.textSecondary; text: "pid=" + (root.runtimeSnapshot.pid !== undefined ? root.runtimeSnapshot.pid : "n/a") }
                                LV.Label { style: description; color: LV.Theme.textSecondary; text: "uptimeMs=" + (root.runtimeSnapshot.uptimeMs !== undefined ? root.runtimeSnapshot.uptimeMs : "n/a") }
                                LV.Label { style: description; color: LV.Theme.textSecondary; text: "rssBytes=" + (root.runtimeSnapshot.rssBytes !== undefined ? root.runtimeSnapshot.rssBytes : "n/a") }
                            }

                            Column {
                                Layout.fillWidth: true
                                spacing: LV.Theme.gap4
                                LV.Label { style: header2; color: LV.Theme.textPrimary; text: "Compliance" }
                                LV.Label { style: description; color: LV.Theme.textSecondary; text: "renderScale=" + root.metricsRenderScaleCompliant }
                                LV.Label { style: description; color: LV.Theme.textSecondary; text: "fontFallback=" + root.metricsFontFallbackCompliant }
                                LV.Label { style: description; color: LV.Theme.textSecondary; text: "themeText=" + root.metricsThemeTextCompliant }
                            }

                            Column {
                                Layout.fillWidth: true
                                Layout.alignment: root.catalogCompactLayout ? Qt.AlignLeft : Qt.AlignRight
                                spacing: LV.Theme.gap8
                                LV.LabelButton {
                                    text: "Open Alert"
                                    tone: LV.AbstractButton.Primary
                                    onClicked: LV.AppState.alertOpen = true
                                }
                                LV.LabelButton {
                                    text: "Refresh Snapshot"
                                    tone: LV.AbstractButton.Default
                                    onClicked: {
                                        root.syncRuntimeState()
                                        root.appendRuntimeRow("system", "Main", "manual-refresh", {}, -1, Date.now())
                                    }
                                }
                            }
                        }
                    }

                    LV.AppCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Column {
                            anchors.fill: parent
                            anchors.margins: root.catalogCardInset
                            spacing: LV.Theme.gap8

                            LV.Label {
                                style: body
                                color: LV.Theme.textPrimary
                                text: "Use right-click anywhere in this window to open the global context menu."
                            }

                            LV.Label {
                                style: description
                                color: LV.Theme.textSecondary
                                text: "Outside-click dismissal and global context behavior are integrated at ApplicationWindow level."
                                wrapMode: Text.WordWrap
                            }

                            LV.Label {
                                style: caption
                                color: LV.Theme.textTertiary
                                text: "Last context target: " + (root.lastGlobalContext.ui && root.lastGlobalContext.ui.path ? root.lastGlobalContext.ui.path : "n/a")
                            }
                        }
                    }
                }
            }

            Item {
                LV.AppCard {
                    anchors.fill: parent

                    Column {
                        anchors.fill: parent
                        anchors.margins: root.catalogCardInset
                        spacing: LV.Theme.gap8

                        LV.Label { style: title2; color: LV.Theme.textPrimary; text: "Typography" }
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

            Item {
                LV.AppCard {
                    anchors.fill: parent

                    Column {
                        anchors.fill: parent
                        anchors.margins: root.catalogCardInset
                        spacing: LV.Theme.gap10

                        LV.Label { style: title2; color: LV.Theme.textPrimary; text: "Button States" }

                        Flow {
                            width: parent.width
                            spacing: LV.Theme.gap8
                            LV.LabelButton { text: "Label"; tone: LV.AbstractButton.Primary }
                            LV.IconButton { iconName: "add"; tone: LV.AbstractButton.Primary }
                            LV.LabelMenuButton { text: "Menu"; tone: LV.AbstractButton.Primary }
                            LV.IconMenuButton { iconName: "viewMoreSymbolicDefault"; tone: LV.AbstractButton.Primary }
                        }

                        Flow {
                            width: parent.width
                            spacing: LV.Theme.gap8
                            LV.LabelButton { text: "Label"; tone: LV.AbstractButton.Default }
                            LV.IconButton { iconName: "add"; tone: LV.AbstractButton.Default }
                            LV.LabelMenuButton { text: "Menu"; tone: LV.AbstractButton.Default }
                            LV.IconMenuButton { iconName: "viewMoreSymbolicDefault"; tone: LV.AbstractButton.Default }
                        }

                        Flow {
                            width: parent.width
                            spacing: LV.Theme.gap8
                            LV.LabelButton { text: "Label"; tone: LV.AbstractButton.Borderless }
                            LV.IconButton { iconName: "add"; tone: LV.AbstractButton.Borderless }
                            LV.LabelMenuButton { text: "Menu"; tone: LV.AbstractButton.Borderless }
                            LV.IconMenuButton { iconName: "viewMoreSymbolicDefault"; tone: LV.AbstractButton.Borderless }
                        }

                        Flow {
                            width: parent.width
                            spacing: LV.Theme.gap8
                            LV.LabelButton { text: "Label"; tone: LV.AbstractButton.Destructive }
                            LV.IconButton { iconName: "add"; tone: LV.AbstractButton.Destructive }
                            LV.LabelMenuButton { text: "Menu"; tone: LV.AbstractButton.Destructive }
                            LV.IconMenuButton { iconName: "viewMoreSymbolicDefault"; tone: LV.AbstractButton.Destructive }
                        }

                        Flow {
                            width: parent.width
                            spacing: LV.Theme.gap8
                            LV.LabelButton { text: "Label"; enabled: false }
                            LV.IconButton { iconName: "add"; enabled: false }
                            LV.LabelMenuButton { text: "Menu"; enabled: false }
                            LV.IconMenuButton { iconName: "viewMoreSymbolicDefault"; enabled: false }
                        }
                    }
                }
            }

            Item {
                LV.AppCard {
                    anchors.fill: parent

                    ScrollView {
                        anchors.fill: parent
                        clip: true

                        ColumnLayout {
                            width: parent.width
                            spacing: root.catalogSectionGap

                            LV.Label {
                                style: title2
                                color: LV.Theme.textPrimary
                                text: "Table Components"
                                Layout.leftMargin: root.catalogContentInset
                                Layout.rightMargin: root.catalogContentInset
                                Layout.topMargin: root.catalogContentInset
                            }

                            LV.Label {
                                style: description
                                color: LV.Theme.textSecondary
                                text: "Table primitives are shown together for design-parity checks against Figma nodes."
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                Layout.leftMargin: root.catalogContentInset
                                Layout.rightMargin: root.catalogContentInset
                            }

                            Rectangle {
                                Layout.leftMargin: root.catalogContentInset
                                Layout.rightMargin: root.catalogContentInset
                                Layout.fillWidth: true
                                implicitHeight: tablePreview.implicitHeight + (root.catalogContentInset * 2)
                                radius: LV.Theme.radiusMd
                                color: LV.Theme.surfaceAlt
                                border.width: 1
                                border.color: LV.Theme.contextMenuDivider
                                clip: true

                                Flickable {
                                    id: tablePreviewFlick
                                    anchors.fill: parent
                                    anchors.margins: root.catalogContentInset
                                    contentWidth: tablePreview.implicitWidth
                                    contentHeight: tablePreview.implicitHeight
                                    interactive: contentWidth > width
                                    flickableDirection: Flickable.HorizontalFlick
                                    boundsBehavior: Flickable.StopAtBounds
                                    clip: true

                                    LV.Table {
                                        id: tablePreview
                                        width: 405
                                        headerColumns: ["Name", "State", "Owner"]
                                        rows: [
                                            ["Renderer", "Active", "Core"],
                                            ["Input", "Idle", "UX"],
                                            ["Pipeline", "Active", "Render"],
                                            ["Metrics", "Paused", "Ops"]
                                        ]
                                    }
                                }
                            }

                            LV.Label {
                                style: header2
                                color: LV.Theme.textPrimary
                                text: "Subcomponents"
                                Layout.leftMargin: root.catalogContentInset
                                Layout.rightMargin: root.catalogContentInset
                                Layout.topMargin: LV.Theme.gap4
                            }

                            Rectangle {
                                Layout.leftMargin: root.catalogContentInset
                                Layout.rightMargin: root.catalogContentInset
                                Layout.fillWidth: true
                                implicitHeight: subcomponentColumn.implicitHeight + (root.catalogContentInset * 2)
                                radius: LV.Theme.radiusMd
                                color: LV.Theme.surfaceAlt
                                border.width: 1
                                border.color: LV.Theme.contextMenuDivider
                                clip: true

                                Flickable {
                                    id: subcomponentFlick
                                    anchors.fill: parent
                                    anchors.margins: root.catalogContentInset
                                    contentWidth: subcomponentColumn.width
                                    contentHeight: subcomponentColumn.height
                                    interactive: contentWidth > width
                                    flickableDirection: Flickable.HorizontalFlick
                                    boundsBehavior: Flickable.StopAtBounds
                                    clip: true

                                    Column {
                                        id: subcomponentColumn
                                        width: Math.max(subcomponentFlick.width, 717)
                                        spacing: LV.Theme.gap8

                                        LV.TableHeader {
                                            width: parent.width
                                            columns: ["Column", "Column", "Column"]
                                        }

                                        LV.TableRow {
                                            width: parent.width
                                            cells: ["Text", "Text", "Text"]
                                        }

                                        LV.TableCellItem {
                                            text: "Text"
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: LV.Theme.gap12
                            }
                        }
                    }
                }
            }

            Item {
                LV.AppCard {
                    anchors.fill: parent

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.catalogCardInset
                        spacing: LV.Theme.gap10

                        LV.Label { style: title2; color: LV.Theme.textPrimary; text: "Input Components" }

                        LV.InputField {
                            Layout.fillWidth: true
                            placeholderText: "Standard input"
                            text: "LVRS"
                        }

                        LV.InputField {
                            Layout.fillWidth: true
                            placeholderText: "Search mode"
                            mode: searchMode
                        }

                        LV.TextEditor {
                            Layout.fillWidth: true
                            editorHeight: 140
                            text: "TextEditor uses fixed external height while keeping internal scroll responsive."
                        }

                        LV.CodeEditor {
                            Layout.fillWidth: true
                            editorHeight: 160
                            snippetTitle: "main.qml"
                            snippetLanguage: "qml"
                            text: "import QtQuick\\nRectangle { color: \"#1E1F22\" }"
                        }
                    }
                }
            }

            Item {
                LV.AppCard {
                    anchors.fill: parent

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.catalogCardInset
                        spacing: LV.Theme.gap10

                        LV.Label {
                            style: title2
                            color: LV.Theme.textPrimary
                            text: "Hierarchy"
                        }

                        LV.Label {
                            Layout.fillWidth: true
                            style: description
                            color: LV.Theme.textSecondary
                            wrapMode: Text.WordWrap
                            text: "Expand/collapse is handled via chevron area. Row click selects item only."
                        }

                        LV.Hierarchy {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: [
                                {
                                    key: "scene",
                                    label: "Scene",
                                    expanded: true,
                                    iconGlyph: "■",
                                    children: [
                                        {
                                            key: "camera",
                                            label: "Main Camera",
                                            iconGlyph: "◎",
                                            children: [
                                                { key: "camera-frustum", label: "Frustum", iconGlyph: "△" }
                                            ]
                                        },
                                        {
                                            key: "lights",
                                            label: "Lights",
                                            expanded: true,
                                            iconGlyph: "◉",
                                            children: [
                                                { key: "key-light", label: "Key Light", iconGlyph: "●" },
                                                { key: "rim-light", label: "Rim Light", iconGlyph: "●" }
                                            ]
                                        }
                                    ]
                                }
                            ]
                        }
                    }
                }
            }

            Item {
                LV.AppCard {
                    anchors.fill: parent

                    ScrollView {
                        anchors.fill: parent
                        clip: true

                        GridLayout {
                            width: parent.width
                            columns: root.catalogCompactLayout
                                ? 1
                                : root.responsiveColumns(width - (LV.Theme.gap10 * 2), 220, 2, 4)
                            rowSpacing: LV.Theme.gap10
                            columnSpacing: LV.Theme.gap10

                            Repeater {
                                model: root.accentPreviewTokens

                                delegate: Rectangle {
                                    id: accentCard
                                    required property var modelData
                                    Layout.fillWidth: true
                                    implicitHeight: 84
                                    radius: LV.Theme.radiusMd
                                    color: LV.Theme.surfaceAlt
                                    border.width: 1
                                    border.color: LV.Theme.contextMenuDivider

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: root.catalogCardInset
                                        spacing: LV.Theme.gap6

                                        Rectangle {
                                            width: parent.width
                                            height: 24
                                            radius: LV.Theme.radiusSm
                                            color: accentCard.modelData.color
                                            border.width: accentCard.modelData.name === "accentTransparent" ? 1 : 0
                                            border.color: LV.Theme.contextMenuDivider
                                        }

                                        LV.Label {
                                            style: caption
                                            color: LV.Theme.textPrimary
                                            text: accentCard.modelData.name
                                            elide: Text.ElideRight
                                        }

                                        LV.Label {
                                            style: disabled
                                            color: LV.Theme.textTertiary
                                            text: accentCard.modelData.color
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: LV.Theme.gap12

                    LV.AppCard {
                        Layout.fillWidth: true
                        implicitHeight: 130

                        Column {
                            anchors.fill: parent
                            anchors.margins: root.catalogCardInset
                            spacing: LV.Theme.gap6

                            LV.Label { style: title2; color: LV.Theme.textPrimary; text: "Event Monitor" }
                            LV.Label { style: description; color: LV.Theme.textSecondary; text: "rows=" + root.runtimeRows.length + " | visible=" + root.runtimeVisibleCount }
                            LV.Label { style: description; color: LV.Theme.textSecondary; text: "lastPress=" + (root.lastGlobalPress.globalX !== undefined ? Math.round(root.lastGlobalPress.globalX) + "," + Math.round(root.lastGlobalPress.globalY) : "n/a") }
                            LV.Label { style: description; color: LV.Theme.textSecondary; text: "lastContext=" + (root.lastGlobalContext.globalX !== undefined ? Math.round(root.lastGlobalContext.globalX) + "," + Math.round(root.lastGlobalContext.globalY) : "n/a") }
                        }
                    }

                    LV.AppCard {
                        Layout.fillWidth: true
                        implicitHeight: root.catalogCompactLayout ? 96 : 52

                        Flow {
                            anchors.fill: parent
                            anchors.margins: root.catalogCardInset
                            spacing: LV.Theme.gap8

                            LV.LabelButton { text: "All"; tone: root.runtimeFilter === "all" ? LV.AbstractButton.Primary : LV.AbstractButton.Default; onClicked: root.runtimeFilter = "all" }
                            LV.LabelButton { text: "Input"; tone: root.runtimeFilter === "input" ? LV.AbstractButton.Primary : LV.AbstractButton.Default; onClicked: root.runtimeFilter = "input" }
                            LV.LabelButton { text: "Runtime"; tone: root.runtimeFilter === "runtime" ? LV.AbstractButton.Primary : LV.AbstractButton.Default; onClicked: root.runtimeFilter = "runtime" }
                            LV.LabelButton { text: "Render"; tone: root.runtimeFilter === "render" ? LV.AbstractButton.Primary : LV.AbstractButton.Default; onClicked: root.runtimeFilter = "render" }
                            LV.LabelButton { text: "Navigation"; tone: root.runtimeFilter === "navigation" ? LV.AbstractButton.Primary : LV.AbstractButton.Default; onClicked: root.runtimeFilter = "navigation" }
                            LV.LabelButton { text: "System"; tone: root.runtimeFilter === "system" ? LV.AbstractButton.Primary : LV.AbstractButton.Default; onClicked: root.runtimeFilter = "system" }
                            LV.LabelButton {
                                text: "Clear"
                                tone: LV.AbstractButton.Default
                                onClicked: {
                                    root.runtimeRows = []
                                    root.lastRuntimeSequence = -1
                                    if (LV.Backend && LV.Backend.clearHookedUserEvents)
                                        LV.Backend.clearHookedUserEvents()
                                }
                            }
                        }
                    }

                    LV.AppCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Flickable {
                            id: runtimeViewport
                            anchors.fill: parent
                            anchors.margins: root.catalogCardInset
                            clip: true
                            contentWidth: width
                            contentHeight: runtimeColumn.implicitHeight

                            Column {
                                id: runtimeColumn
                                width: runtimeViewport.width
                                spacing: LV.Theme.gap8

                                Repeater {
                                    model: root.runtimeRows

                                    delegate: Rectangle {
                                        id: runtimeRowCard
                                        required property var modelData
                                        width: parent.width
                                        visible: root.runtimeFilter === "all"
                                            || String(runtimeRowCard.modelData.category) === root.runtimeFilter
                                        radius: LV.Theme.radiusSm
                                        color: LV.Theme.surfaceGhost
                                        border.width: 1
                                        border.color: LV.Theme.contextMenuDivider
                                        implicitHeight: rowCol.implicitHeight + LV.Theme.gap8 * 2

                                        Column {
                                            id: rowCol
                                            anchors.fill: parent
                                            anchors.margins: root.catalogCardInset
                                            spacing: LV.Theme.gap4

                                            LV.Label {
                                                style: body
                                                color: LV.Theme.textPrimary
                                                text: "["
                                                    + root.timestamp(runtimeRowCard.modelData.epochMs)
                                                    + "] "
                                                    + runtimeRowCard.modelData.source
                                                    + " / "
                                                    + runtimeRowCard.modelData.type
                                            }

                                            LV.Label {
                                                style: caption
                                                color: LV.Theme.textSecondary
                                                text: "category="
                                                    + runtimeRowCard.modelData.category
                                                    + " | sequence="
                                                    + runtimeRowCard.modelData.sequence
                                            }
                                        }
                                    }
                                }

                                LV.Label {
                                    width: parent.width
                                    visible: root.runtimeRows.length === 0
                                    style: description
                                    color: LV.Theme.textSecondary
                                    text: "No runtime rows collected yet. Click or right-click in the window to generate monitor events."
                                }
                            }
                        }

                        LV.WheelScrollGuard {
                            anchors.fill: parent
                            targetFlickable: runtimeViewport
                            consumeInside: true
                        }
                    }
                }
            }
        }
    }
}

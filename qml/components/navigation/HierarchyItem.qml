import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    readonly property bool __isHierarchyItem: true
    readonly property int directionRight: 0
    readonly property int directionLeft: 1
    readonly property int directionUp: 2
    readonly property int directionDown: 3
    property int itemId: -1
    property string itemKey: ""
    property string parentItemKey: ""
    property string pathLabel: ""
    property var hierarchyList: null
    property var nodeData: null
    property bool generatedByTreeModel: false

    text: "Label"
    property alias label: control.text
    property string iconName: ""
    property url iconSource: ""
    property string iconGlyph: ""
    property bool showChevron: true
    property bool hasChildItems: true
    readonly property bool effectiveShowChevron: showChevron && hasChildItems
    property bool expanded: false
    // Supports int enum or string: auto|right|left|up|down
    property var selectionDirection: "auto"
    property bool selected: false
    property bool inputable: false
    property string inputResult: control.text
    property bool _inputOverlayReadyForFocusClose: false
    property bool dragPreviewActive: false
    property real dragPreviewOpacity: 0.45

    signal inputEdited(string text)
    signal inputSubmitted(string text)

    property int indentLevel: 0
    property int indentStep: 8
    property int rowHeight: 20
    property int itemWidth: 200
    property int iconSize: 16
    property int chevronSize: 16
    property int baseLeftPadding: Theme.gap8
    property int rowRightPadding: Theme.gap8
    property int leadingSpacing: Theme.gap2

    property color iconPlaceholderColor: Theme.darkGrey10
    property color textColorNormal: Theme.bodyColor
    property color textColorDisabled: Theme.disabledColor
    property color chevronColor: Theme.darkGrey10
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property int chevronSourceSize: Math.max(1, Math.round(control.chevronSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property string chevronIconName: "generalchevronDown"
    readonly property url chevronIconSource: Theme.iconPath(control.chevronIconName)
    readonly property int resolvedSelectionDirection: {
        const raw = selectionDirection
        if (raw === undefined || raw === null)
            return control.expanded ? directionDown : directionRight
        if (typeof raw === "number")
            return Math.max(directionRight, Math.min(directionDown, Math.round(raw)))

        const normalized = String(raw).trim().toLowerCase()
        if (normalized.length === 0 || normalized === "auto")
            return control.expanded ? directionDown : directionRight
        if (normalized === "left")
            return directionLeft
        if (normalized === "up")
            return directionUp
        if (normalized === "down")
            return directionDown
        return control.expanded ? directionDown : directionRight
    }
    readonly property real resolvedChevronRotation: {
        if (control.resolvedSelectionDirection === control.directionLeft)
            return 90
        if (control.resolvedSelectionDirection === control.directionUp)
            return 180
        if (control.resolvedSelectionDirection === control.directionDown)
            return 0
        return -90
    }
    readonly property bool resolvedSelected: hierarchyList ? hierarchyList.activeItem === control : selected
    readonly property int stateIdle: 0
    readonly property int stateHover: 1
    readonly property int stateActive: 2
    readonly property int interactionState: resolvedSelected
        ? stateActive
        : (enabled && hovered ? stateHover : stateIdle)
    readonly property string interactionStateName: interactionState === stateActive
        ? "Active"
        : (interactionState === stateHover ? "Hover" : "Idle")
    readonly property bool isHoverState: interactionState === stateHover
    readonly property bool isActiveState: interactionState === stateActive
    property color rowBackgroundColor: isActiveState ? Theme.accentOverlay : "transparent"
    property color rowBackgroundColorHover: isActiveState ? Theme.accentOverlay : Theme.surfaceGhost
    property color rowBackgroundColorPressed: isActiveState ? Theme.accentOverlay : Theme.surfaceAlt
    property bool _rowVisibleInternal: true
    readonly property bool rowVisible: _rowVisibleInternal

    readonly property int computedLeftPadding: baseLeftPadding + Math.max(0, indentLevel) * indentStep
    readonly property string resolvedIconName: {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        return rawName.trim()
    }
    readonly property url resolvedIconSource: iconSource.toString().length > 0
        ? iconSource
        : resolvedIconName.length > 0
            ? Theme.iconPath(resolvedIconName)
            : ""

    function normalizedText(value) {
        if (value === undefined || value === null)
            return ""
        return String(value)
    }

    function applyInputResult(value) {
        const normalized = normalizedText(value)
        if (control.text !== normalized)
            control.text = normalized
        if (control.inputResult !== normalized)
            control.inputResult = normalized
        return normalized
    }

    function requestActivationFromInteraction() {
        if (!control.enabled)
            return

        if (control.hierarchyList && control.hierarchyList.requestActivate) {
            control.hierarchyList.requestActivate(control)
            return
        }

        if (!control.selected)
            control.selected = true
    }

    function syncInputOverlayTextAndFocus(requestFocus) {
        if (labelInputLoader.status !== Loader.Ready || !labelInputLoader.item)
            return

        if (!labelInputLoader.item.activeFocus && labelInputLoader.item.text !== control.inputResult)
            labelInputLoader.item.text = control.inputResult

        if (!requestFocus)
            return

        control._inputOverlayReadyForFocusClose = false
        Qt.callLater(function() {
            if (!control.inputable
                || labelInputLoader.status !== Loader.Ready
                || !labelInputLoader.item) {
                return
            }
            labelInputLoader.item.forceInputFocus()
            if (labelInputLoader.item.selectAll)
                labelInputLoader.item.selectAll()
            control._inputOverlayReadyForFocusClose = true
        })
    }

    tone: AbstractButton.Borderless
    state: control.interactionStateName
    leftPadding: computedLeftPadding
    rightPadding: rowRightPadding
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusControl
    implicitHeight: rowHeight
    implicitWidth: Math.max(itemWidth, contentItem.implicitWidth + leftPadding + rightPadding)
    width: parent ? parent.width : implicitWidth
    visible: rowVisible
    opacity: dragPreviewActive ? dragPreviewOpacity : 1.0

    backgroundColor: rowBackgroundColor
    backgroundColorHover: rowBackgroundColorHover
    backgroundColorPressed: rowBackgroundColorPressed
    backgroundColorDisabled: rowBackgroundColor

    onPressed: control.requestActivationFromInteraction()
    onClicked: control.requestActivationFromInteraction()

    onHierarchyListChanged: {
        if (hierarchyList && hierarchyList.registerItem)
            hierarchyList.registerItem(control)
    }

    contentItem: RowLayout {
        spacing: Theme.gapNone

        RowLayout {
            Layout.fillWidth: true
            spacing: control.leadingSpacing

            Item {
                Layout.preferredWidth: control.iconSize
                Layout.preferredHeight: control.iconSize
                Layout.alignment: Qt.AlignVCenter

                Image {
                    id: iconImage
                    anchors.centerIn: parent
                    visible: control.iconGlyph.length === 0 && control.resolvedIconSource.toString().length > 0
                    source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
                    sourceSize.width: control.iconSourceSize
                    sourceSize.height: control.iconSourceSize
                    width: control.iconSize
                    height: control.iconSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: RenderQuality.mipmapEnabled
                }

                Label {
                    anchors.centerIn: parent
                    visible: control.iconGlyph.length > 0
                    text: control.iconGlyph
                    style: body
                    color: control.enabled ? control.textColorNormal : control.textColorDisabled
                    font.pixelSize: control.iconSize
                    font.weight: Font.Normal
                    font.styleName: "Regular"
                    lineHeight: control.iconSize
                    lineHeightMode: Text.FixedHeight
                }

                Rectangle {
                    anchors.centerIn: parent
                    visible: !iconImage.visible && control.iconGlyph.length === 0
                    width: 12
                    height: 12
                    radius: 2
                    color: control.iconPlaceholderColor
                    antialiasing: true
                }
            }

            Item {
                id: labelHost
                Layout.fillWidth: true
                implicitHeight: 20

                Label {
                    id: labelNode
                    objectName: "hierarchyItemLabel"
                    anchors.fill: parent
                    style: body
                    text: control.inputResult
                    color: control.enabled ? control.textColorNormal : control.textColorDisabled
                    font.pixelSize: 13
                    font.weight: Font.Normal
                    font.styleName: "Regular"
                    lineHeight: 16
                    lineHeightMode: Text.FixedHeight
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    visible: !control.inputable
                }

                Loader {
                    id: labelInputLoader
                    objectName: "hierarchyItemInputLoader"
                    anchors.fill: labelNode
                    active: control.inputable
                    visible: control.inputable
                    sourceComponent: Component {
                        InputField {
                            objectName: "hierarchyItemInputOverlay"
                            enabled: control.enabled
                            backgroundColor: "transparent"
                            backgroundColorHover: "transparent"
                            backgroundColorPressed: "transparent"
                            backgroundColorFocused: "transparent"
                            backgroundColorDisabled: "transparent"
                            placeholderText: ""
                            clearButtonVisible: false
                            fieldMinHeight: 20
                            centeredTextHeight: 16
                            insetHorizontal: 0
                            insetVertical: 0
                            sideSpacing: 0
                            cornerRadius: 0
                            textColor: control.textColorNormal
                            textColorDisabled: control.textColorDisabled
                            placeholderColor: control.textColorDisabled
                            placeholderColorDisabled: control.textColorDisabled

                            onTextEdited: {
                                const value = control.applyInputResult(text)
                                control.inputEdited(value)
                            }
                            onAccepted: {
                                const value = control.applyInputResult(text)
                                control.inputSubmitted(value)
                                control.inputable = false
                            }
                            onFocusedChanged: {
                                if (!control.inputable
                                    || focused
                                    || !control._inputOverlayReadyForFocusClose) {
                                    return
                                }
                                control.inputable = false
                            }
                        }
                    }

                    onLoaded: {
                        control.syncInputOverlayTextAndFocus(control.inputable)
                    }
                    onStatusChanged: {
                        if (status !== Loader.Ready)
                            control._inputOverlayReadyForFocusClose = false
                    }
                }
            }
        }

        Item {
            Layout.preferredWidth: control.chevronSize
            Layout.preferredHeight: control.chevronSize
            Layout.alignment: Qt.AlignVCenter
            visible: control.effectiveShowChevron

            Image {
                id: chevronIcon
                anchors.fill: parent
                source: RenderQuality.resolveTextureSource(control.chevronIconSource)
                sourceSize.width: control.chevronSourceSize
                sourceSize.height: control.chevronSourceSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: RenderQuality.mipmapEnabled
                rotation: control.resolvedChevronRotation
                transformOrigin: Item.Center
                opacity: control.enabled ? 1.0 : 0.45
            }

            MouseArea {
                anchors.fill: parent
                enabled: control.effectiveShowChevron && control.enabled
                acceptedButtons: Qt.LeftButton
                onClicked: function(mouse) {
                    mouse.accepted = true
                    control.expanded = !control.expanded
                    control.requestActivationFromInteraction()
                }
            }
        }
    }

    onIndentLevelChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onShowChevronChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onHasChildItemsChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onTextChanged: {
        const normalized = control.normalizedText(control.text)
        if (control.inputResult !== normalized)
            control.inputResult = normalized
        if (labelInputLoader.status === Loader.Ready
            && labelInputLoader.item
            && !labelInputLoader.item.activeFocus
            && labelInputLoader.item.text !== normalized) {
            labelInputLoader.item.text = normalized
        }
    }
    onInputableChanged: {
        if (!control.inputable) {
            control._inputOverlayReadyForFocusClose = false
            return
        }
        control.syncInputOverlayTextAndFocus(true)
    }
    onExpandedChanged: {
        if (control.hierarchyList && control.hierarchyList.notifyExpansionChanged)
            control.hierarchyList.notifyExpansionChanged(control)
    }
    onEnabledChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
        if (!control.enabled && control.hierarchyList && control.hierarchyList.scheduleNormalizeActiveItem)
            control.hierarchyList.scheduleNormalizeActiveItem()
    }

    QtObject {
        Component.onCompleted: {
            if (control.hierarchyList && control.hierarchyList.registerItem)
                control.hierarchyList.registerItem(control)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.HierarchyItem { label: "Main Camera"; iconGlyph: "■"; indentLevel: 1; showChevron: true; inputable: true }

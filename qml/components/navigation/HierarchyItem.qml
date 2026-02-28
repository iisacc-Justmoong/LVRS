import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    readonly property bool __isHierarchyItem: true
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
    property bool selected: false

    property int indentLevel: 0
    property int indentStep: 13
    property int rowHeight: 28
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
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale))
    readonly property int chevronSourceSize: Math.max(1, Math.round(control.chevronSize * control.iconSupersampleScale))
    readonly property string chevronIconName: "generalchevronDown"
    readonly property url chevronIconSource: Theme.iconPath(control.chevronIconName)
    readonly property real resolvedChevronRotation: control.expanded ? 0 : -90
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

    tone: AbstractButton.Borderless
    state: control.interactionStateName
    leftPadding: computedLeftPadding
    rightPadding: rowRightPadding
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.gapNone
    implicitHeight: rowHeight
    implicitWidth: Math.max(itemWidth, contentItem.implicitWidth + leftPadding + rightPadding)
    width: parent ? parent.width : implicitWidth
    visible: rowVisible

    backgroundColor: rowBackgroundColor
    backgroundColorHover: rowBackgroundColorHover
    backgroundColorPressed: rowBackgroundColorPressed
    backgroundColorDisabled: rowBackgroundColor

    onClicked: {
        if (hierarchyList && hierarchyList.requestActivate)
            hierarchyList.requestActivate(control)
    }

    onHierarchyListChanged: {
        if (hierarchyList && hierarchyList.registerItem)
            hierarchyList.registerItem(control)
    }

    contentItem: RowLayout {
        spacing: Theme.gap14

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

            Label {
                id: labelNode
                Layout.fillWidth: true
                style: body
                text: control.text
                color: control.enabled ? control.textColorNormal : control.textColorDisabled
                font.weight: Font.Normal
                font.styleName: "Regular"
                lineHeight: 16
                lineHeightMode: Text.FixedHeight
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
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
                    if (control.hierarchyList && control.hierarchyList.requestActivate)
                        control.hierarchyList.requestActivate(control)
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
// LV.HierarchyItem { label: "Main Camera"; iconGlyph: "■"; indentLevel: 1; showChevron: true }

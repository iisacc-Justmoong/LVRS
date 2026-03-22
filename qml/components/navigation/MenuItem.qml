import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    readonly property int defaultState: 0
    readonly property int selectedState: 1
    readonly property int inactiveState: 2
    readonly property int directionRight: 0
    readonly property int directionLeft: 1
    readonly property int directionUp: 2
    readonly property int directionDown: 3

    property int state: defaultState
    property string label: "Label"
    QtObject {
        id: shortcutStore
        property string text: "key"
    }

    property alias key: shortcutStore.text
    property alias shortcut: shortcutStore.text
    property bool keyVisible: true
    property string keyPlaceholder: "key"
    property bool showChevron: true
    property bool hasChildItems: true
    readonly property bool effectiveShowChevron: showChevron && hasChildItems
    property bool expanded: false
    // Supports int enum or string: auto|right|left|up|down
    property var selectionDirection: "auto"
    property int itemWidth: Theme.scaleMetric(161)
    property int itemHeight: Theme.scaleMetric(22)
    property int iconSize: Theme.scaleMetric(16)
    property int chevronSize: Theme.scaleMetric(16)
    property string iconName: ""
    property url iconSource: ""
    property color iconPlaceholderColor: Theme.darkGrey10
    property color chevronColor: Theme.descriptionColor
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property int chevronSourceSize: Math.max(1, Math.round(control.chevronSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property string chevronIconName: "generalchevronDown"
    readonly property url chevronIconSource: Theme.iconPath(control.chevronIconName)

    readonly property bool isSelected: state === selectedState
    readonly property bool isInactive: state === inactiveState
    readonly property string resolvedIconName: {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        return rawName.trim()
    }
    readonly property url resolvedIconSource: iconSource.toString().length > 0
        ? iconSource
        : resolvedIconName.length > 0
            ? Theme.iconPath(resolvedIconName)
            : ""
    readonly property string resolvedShortcutText: {
        if (!control.keyVisible)
            return ""
        const rawShortcut = shortcutStore.text === undefined || shortcutStore.text === null
            ? ""
            : String(shortcutStore.text).trim()
        if (rawShortcut.length > 0)
            return rawShortcut
        return control.keyPlaceholder
    }
    // Measure text unconstrained so display elision does not collapse layout math.
    readonly property int labelNaturalWidth: Math.max(0, Math.ceil(labelMetrics.advanceWidth))
    readonly property int shortcutNaturalWidth: control.keyVisible && control.resolvedShortcutText.length > 0
        ? Math.max(0, Math.ceil(shortcutMetrics.advanceWidth))
        : 0
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
    readonly property color resolvedBackgroundColor: isSelected
        ? Theme.contextMenuItemSelectedBackground
        : isInactive
            ? Theme.contextMenuItemInactiveBackground
            : "transparent"
    readonly property real resolvedChevronRotation: {
        if (control.resolvedSelectionDirection === control.directionLeft)
            return 90
        if (control.resolvedSelectionDirection === control.directionUp)
            return 180
        if (control.resolvedSelectionDirection === control.directionDown)
            return 0
        return -90
    }

    tone: AbstractButton.Borderless
    horizontalPadding: Theme.gap4
    verticalPadding: Theme.gap3
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm

    implicitWidth: Math.max(itemWidth, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: itemHeight

    textColor: Theme.titleHeaderColor
    textColorDisabled: Theme.disabledColor
    backgroundColor: resolvedBackgroundColor
    backgroundColorHover: resolvedBackgroundColor
    backgroundColorPressed: resolvedBackgroundColor
    backgroundColorDisabled: resolvedBackgroundColor

    TextMetrics {
        id: labelMetrics
        font.family: Theme.fontBody
        font.pixelSize: Theme.textBody
        font.weight: Theme.textBodyWeight
        font.styleName: Theme.textBodyStyleName
        font.letterSpacing: Theme.textBodyLetterSpacing
        text: control.label
    }

    TextMetrics {
        id: shortcutMetrics
        font.family: Theme.fontBody
        font.pixelSize: Theme.textBody
        font.weight: Theme.textBodyWeight
        font.styleName: Theme.textBodyStyleName
        font.letterSpacing: Theme.textBodyLetterSpacing
        text: control.resolvedShortcutText
    }

    contentItem: Item {
        id: contentRoot
        objectName: "menuItem_contentRow"
        readonly property real layoutWidth: width > 0 ? width : implicitWidth
        readonly property int shortcutNaturalWidth: control.shortcutNaturalWidth
        readonly property int chevronLayoutWidth: control.effectiveShowChevron ? control.chevronSize : 0
        readonly property int shortcutBudgetWidth: control.keyVisible
            ? Math.max(0,
                       Math.round((layoutWidth - control.iconSize - chevronLayoutWidth - (Theme.gap8 * 3)) * 0.45))
            : 0
        readonly property int resolvedShortcutWidth: control.keyVisible
            ? Math.max(0, Math.min(shortcutNaturalWidth, shortcutBudgetWidth))
            : 0
        readonly property int trailingInnerGap: resolvedShortcutWidth > 0 && chevronLayoutWidth > 0 ? Theme.gap8 : 0
        readonly property int resolvedTrailingWidth: resolvedShortcutWidth + trailingInnerGap + chevronLayoutWidth
        readonly property int resolvedLabelWidth: Math.max(
                                                      0,
                                                      Math.min(control.labelNaturalWidth,
                                                               Math.round(layoutWidth)
                                                               - control.iconSize
                                                               - Theme.gap8
                                                               - (resolvedTrailingWidth > 0
                                                                      ? Theme.gap8 + resolvedTrailingWidth
                                                                      : 0)))
        readonly property real labelX: control.iconSize + Theme.gap8
        readonly property real labelRight: labelX + resolvedLabelWidth
        readonly property real trailingX: layoutWidth - resolvedTrailingWidth
        readonly property real spacerX: labelRight
        readonly property real spacerWidth: Math.max(0, trailingX - labelRight)

        implicitWidth: Math.max(
                           control.itemWidth - control.leftPadding - control.rightPadding,
                           control.iconSize
                           + Theme.gap8
                           + control.labelNaturalWidth
                           + (shortcutNaturalWidth > 0 || chevronLayoutWidth > 0
                                  ? Theme.gap8 + shortcutNaturalWidth + (shortcutNaturalWidth > 0 && chevronLayoutWidth > 0 ? Theme.gap8 : 0) + chevronLayoutWidth
                                  : 0))
        implicitHeight: Math.max(
                            control.iconSize,
                            labelNode.implicitHeight,
                            shortcutLabel.implicitHeight,
                            chevronLayoutWidth)

        Item {
            objectName: "menuItem_iconSlot"
            x: 0
            y: Math.round((contentRoot.height - height) * 0.5)
            width: control.iconSize
            height: control.iconSize

            Image {
                id: iconImage
                visible: control.resolvedIconSource.toString().length > 0
                anchors.centerIn: parent
                width: control.iconSize
                height: control.iconSize
                source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
                sourceSize.width: control.iconSourceSize
                sourceSize.height: control.iconSourceSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: RenderQuality.mipmapEnabled
            }

            Item {
                anchors.fill: parent
                visible: !iconImage.visible

                Rectangle {
                    width: Theme.scaleMetric(12)
                    height: Theme.scaleMetric(12)
                    radius: Theme.gap3
                    color: control.iconPlaceholderColor
                    anchors.centerIn: parent
                    antialiasing: true
                }
            }
        }

        Label {
            id: labelNode
            objectName: "menuItem_labelNode"
            x: contentRoot.labelX
            y: Math.round((contentRoot.height - height) * 0.5)
            width: contentRoot.resolvedLabelWidth
            height: implicitHeight
            style: body
            text: control.label
            color: control.isInactive ? Theme.titleHeaderColor
                                      : (control.effectiveEnabled ? Theme.titleHeaderColor : Theme.disabledColor)
            elide: Text.ElideRight
            lineHeight: Theme.textBodyLineHeight
            lineHeightMode: Text.FixedHeight
        }

        Item {
            id: flexibleSpacer
            objectName: "menuItem_flexibleSpacer"
            x: contentRoot.spacerX
            y: 0
            width: contentRoot.spacerWidth
            height: contentRoot.height
        }

        Item {
            id: trailingGroup
            objectName: "menuItem_trailingGroup"
            visible: contentRoot.resolvedTrailingWidth > 0
            x: contentRoot.trailingX
            y: Math.round((contentRoot.height - height) * 0.5)
            width: contentRoot.resolvedTrailingWidth
            height: Math.max(shortcutLabel.implicitHeight, control.chevronSize)

            Label {
                id: shortcutLabel
                objectName: "menuItem_shortcutLabel"
                style: body
                visible: control.keyVisible && contentRoot.resolvedShortcutWidth > 0
                x: 0
                y: Math.round((parent.height - height) * 0.5)
                width: contentRoot.resolvedShortcutWidth
                height: implicitHeight
                text: control.resolvedShortcutText
                color: control.isInactive ? Theme.descriptionColor
                                          : (control.effectiveEnabled ? Theme.descriptionColor : Theme.disabledColor)
                elide: Text.ElideRight
                lineHeight: Theme.textBodyLineHeight
                lineHeightMode: Text.FixedHeight
            }

            Image {
                id: chevronIcon
                objectName: "menuItem_chevronIcon"
                visible: control.effectiveShowChevron
                x: parent.width - width
                y: Math.round((parent.height - height) * 0.5)
                width: control.chevronSize
                height: control.chevronSize
                source: RenderQuality.resolveTextureSource(control.chevronIconSource)
                sourceSize.width: control.chevronSourceSize
                sourceSize.height: control.chevronSourceSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: RenderQuality.mipmapEnabled
                rotation: control.resolvedChevronRotation
                transformOrigin: Item.Center
                opacity: control.isInactive ? 1.0 : (control.effectiveEnabled ? 1.0 : 0.45)
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.MenuItem {
//     iconName: "iconname"
//     state: selectedState
//     label: "Label"
//     keyVisible: true
//     key: "Cmd+K"
//     hasChildItems: true
//     showChevron: true
//     expanded: false
//     selectionDirection: "auto"
// }

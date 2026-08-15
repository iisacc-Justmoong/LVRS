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
    property bool keyVisible: false
    property string keyPlaceholder: ""
    property bool showChevron: false
    property bool hasChildItems: false
    readonly property bool effectiveShowChevron: showChevron && hasChildItems
    property bool expanded: false
    // Supports int enum or string: auto|right|left|up|down
    property var selectionDirection: "auto"
    property int itemWidth: Theme.scaleMetric(161)
    property int itemHeight: Theme.iconSm
    property bool showIconSlot: true
    property int iconSize: Theme.iconSm
    property int chevronSize: Theme.iconSm
    property string iconName: ""
    property url iconSource: ""
    property color iconPlaceholderColor: Theme.accentBlueMuted
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
    readonly property bool effectiveShowIconSlot: showIconSlot
    readonly property int resolvedIconSlotWidth: effectiveShowIconSlot ? iconSize : 0
    readonly property int resolvedIconLabelGap: effectiveShowIconSlot ? Theme.gap8 : 0
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
    verticalPadding: Theme.gapNone
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
        font.weight: Font.Normal
        font.styleName: "Regular"
        font.letterSpacing: Theme.textBodyLetterSpacing
        text: control.label
    }

    TextMetrics {
        id: shortcutMetrics
        font.family: Theme.fontBody
        font.pixelSize: Theme.textDescription
        font.weight: Theme.textDescriptionWeight
        font.styleName: Theme.textDescriptionStyleName
        font.letterSpacing: Theme.textDescriptionLetterSpacing
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
                       Math.round((layoutWidth
                                   - control.resolvedIconSlotWidth
                                   - control.resolvedIconLabelGap
                                   - chevronLayoutWidth
                                   - (Theme.gap8 * 2)) * 0.45))
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
                                                              - control.resolvedIconSlotWidth
                                                              - control.resolvedIconLabelGap
                                                              - (resolvedTrailingWidth > 0
                                                                      ? Theme.gap8 + resolvedTrailingWidth
                                                                      : 0)))
        readonly property real labelX: control.resolvedIconSlotWidth + control.resolvedIconLabelGap
        readonly property real labelRight: labelX + resolvedLabelWidth
        readonly property real trailingX: layoutWidth - resolvedTrailingWidth
        readonly property real spacerX: labelRight
        readonly property real spacerWidth: Math.max(0, trailingX - labelRight)

        implicitWidth: Math.max(
                           control.itemWidth - control.leftPadding - control.rightPadding,
                           control.resolvedIconSlotWidth
                           + control.resolvedIconLabelGap
                           + control.labelNaturalWidth
                           + (shortcutNaturalWidth > 0 || chevronLayoutWidth > 0
                                  ? Theme.gap8 + shortcutNaturalWidth + (shortcutNaturalWidth > 0 && chevronLayoutWidth > 0 ? Theme.gap8 : 0) + chevronLayoutWidth
                                  : 0))
        implicitHeight: Math.max(
                            control.resolvedIconSlotWidth,
                            labelNode.implicitHeight,
                            shortcutLabel.implicitHeight,
                            chevronLayoutWidth)

        Item {
            objectName: "menuItem_iconSlot"
            visible: control.effectiveShowIconSlot
            x: 0
            y: Math.round((contentRoot.height - height) * 0.5)
            width: control.resolvedIconSlotWidth
            height: control.effectiveShowIconSlot ? control.iconSize : 0

            Image {
                id: iconImage
                visible: control.effectiveShowIconSlot && control.resolvedIconSource.toString().length > 0
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
                visible: control.effectiveShowIconSlot && !iconImage.visible

                Rectangle {
                    width: control.iconSize
                    height: control.iconSize
                    radius: width * 0.5
                    color: control.iconPlaceholderColor
                    border.width: control.iconSize * (1.0 / 16.0)
                    border.color: Theme.accentBlue
                    anchors.centerIn: parent
                    antialiasing: true
                }

                Rectangle {
                    width: control.iconSize * 0.25
                    height: control.iconSize * 0.25
                    radius: width * 0.5
                    color: Theme.accentBlue
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
            font.weight: Font.Normal
            font.styleName: "Regular"
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
                style: description
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
//     showIconSlot: true
//     state: selectedState
//     label: "Label"
//     keyVisible: true
//     key: "Cmd+K"
// }

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    // Slot config contract:
    // {
    //   type: "icon" | "menu" | "IconButton" | "IconMenuButton",
    //   iconName: "projectStructure",
    //   iconSource: "qrc:/...",
    //   iconGlyph: "",
    //   text: "",
    //   tone: AbstractButton.Borderless,
    //   iconSize: Theme.iconSm,
    //   enabled: true,
    //   visible: true,
    //   onClicked: function() {},
    //   props: { ...IconButton or IconMenuButton assignable props... }
    // }
    property var button1: ({ type: "icon", iconName: "addFile" })
    property var button2: ({ type: "icon", iconName: "generaldelete" })
    property var button3: ({ type: "menu", iconName: "settings" })
    property int horizontalPadding: Theme.gap2
    property int verticalPadding: Theme.gap2
    property int spacing: Theme.gapNone
    property bool interactive: true
    readonly property int stockButtonPadding: Theme.gap2
    readonly property int stockButtonHeight: Theme.iconSm + (stockButtonPadding * 2)
    readonly property int stockMenuButtonSpacing: -Theme.gap2

    signal buttonClicked(int index, var config)

    function buttonConfigAt(index) {
        if (index === 0)
            return button1 || ({})
        if (index === 1)
            return button2 || ({})
        if (index === 2)
            return button3 || ({})
        return ({})
    }

    function configValue(config, key, fallback) {
        if (!config || typeof config !== "object")
            return fallback
        if (config[key] !== undefined)
            return config[key]
        if (config.props && typeof config.props === "object" && config.props[key] !== undefined)
            return config.props[key]
        return fallback
    }

    function isMenuType(config) {
        const rawType = configValue(config, "type", configValue(config, "kind", "icon"))
        const normalized = String(rawType).toLowerCase()
        return normalized === "menu"
            || normalized === "iconmenu"
            || normalized === "iconmenubutton"
    }

    function stockButtonWidth(config) {
        if (!configValue(config, "visible", true))
            return 0
        const iconSize = configValue(config, "iconSize", Theme.iconSm)
        const horizontalInset = configValue(
            config,
            "horizontalPadding",
            control.stockButtonPadding)
        if (!isMenuType(config))
            return iconSize + (horizontalInset * 2)
        const contentSpacing = configValue(
            config,
            "spacing",
            control.stockMenuButtonSpacing)
        return (iconSize * 2) + contentSpacing + (horizontalInset * 2)
    }

    function stockButtonHeightFor(config) {
        if (!configValue(config, "visible", true))
            return 0
        const iconSize = configValue(config, "iconSize", Theme.iconSm)
        const verticalInset = configValue(
            config,
            "verticalPadding",
            control.stockButtonPadding)
        return iconSize + (verticalInset * 2)
    }

    readonly property int stockVisibleButtonCount:
        (configValue(button1, "visible", true) ? 1 : 0)
        + (configValue(button2, "visible", true) ? 1 : 0)
        + (configValue(button3, "visible", true) ? 1 : 0)
    readonly property int stockRowWidth:
        stockButtonWidth(button1)
        + stockButtonWidth(button2)
        + stockButtonWidth(button3)
        + (Math.max(0, stockVisibleButtonCount - 1) * spacing)
    readonly property int stockRowHeight: Math.max(
        stockButtonHeightFor(button1),
        stockButtonHeightFor(button2),
        stockButtonHeightFor(button3))

    function applyCommonButtonProps(button, config) {
        const source = configValue(config, "iconSource", configValue(config, "url", undefined))
        button.tone = configValue(config, "tone", AbstractButton.Borderless)
        button.iconName = configValue(config, "iconName", "")
        button.iconGlyph = configValue(config, "iconGlyph", "")
        button.text = configValue(config, "text", "")
        button.iconSize = Qt.binding(function() {
            return control.configValue(config, "iconSize", Theme.iconSm)
        })
        button.enabled = Qt.binding(function() {
            return control.interactive && control.configValue(config, "enabled", true)
        })
        if (source !== undefined)
            button.iconSource = source
        button.horizontalPadding = Qt.binding(function() {
            return control.configValue(config, "horizontalPadding", control.stockButtonPadding)
        })
        button.verticalPadding = Qt.binding(function() {
            return control.configValue(config, "verticalPadding", control.stockButtonPadding)
        })
        button.spacing = Qt.binding(function() {
            return control.configValue(
                config,
                "spacing",
                control.isMenuType(config) ? control.stockMenuButtonSpacing : Theme.gapNone)
        })
        button.cornerRadius = Qt.binding(function() {
            return control.configValue(config, "cornerRadius", Theme.radiusSm)
        })
        button.backgroundColor = configValue(config, "backgroundColor", "transparent")
        button.backgroundColorDisabled = configValue(config, "backgroundColorDisabled", "transparent")
        button.backgroundColorHover = configValue(config, "backgroundColorHover", Theme.surfaceAlt)
        button.backgroundColorPressed = configValue(config, "backgroundColorPressed", Theme.accentBlueMuted)
    }

    function dispatchClicked(index) {
        const config = buttonConfigAt(index)
        const callback = configValue(config, "onClicked", null)
        if (callback && typeof callback === "function")
            callback()
        buttonClicked(index, config || ({}))
    }

    implicitWidth: Math.max(footerRow.implicitWidth, stockRowWidth) + (horizontalPadding * 2)
    implicitHeight: Math.max(footerRow.implicitHeight, stockRowHeight) + (verticalPadding * 2)

    RowLayout {
        id: footerRow
        anchors.fill: parent
        anchors.leftMargin: control.horizontalPadding
        anchors.rightMargin: control.horizontalPadding
        anchors.topMargin: control.verticalPadding
        anchors.bottomMargin: control.verticalPadding
        spacing: control.spacing

        Repeater {
            model: 3

            delegate: Item {
                id: slotItem
                objectName: "listFooter_slot_" + index
                required property int index
                readonly property var slotConfig: control.buttonConfigAt(index)
                readonly property bool menuType: control.isMenuType(slotConfig)
                readonly property bool slotVisible: control.configValue(slotConfig, "visible", true)
                readonly property int stockImplicitWidth: control.stockButtonWidth(slotConfig)

                implicitWidth: Math.max(
                    stockImplicitWidth,
                    menuType ? menuButton.implicitWidth : iconButton.implicitWidth)
                implicitHeight: Math.max(
                    control.stockButtonHeight,
                    menuType ? menuButton.implicitHeight : iconButton.implicitHeight)
                visible: slotVisible

                Layout.preferredWidth: implicitWidth
                Layout.preferredHeight: implicitHeight
                Layout.alignment: Qt.AlignVCenter

                IconButton {
                    id: iconButton
                    objectName: "listFooter_iconButton_" + slotItem.index
                    anchors.fill: parent
                    visible: !slotItem.menuType
                    tone: AbstractButton.Borderless
                    onClicked: control.dispatchClicked(slotItem.index)
                }

                IconMenuButton {
                    id: menuButton
                    objectName: "listFooter_menuButton_" + slotItem.index
                    anchors.fill: parent
                    visible: slotItem.menuType
                    tone: AbstractButton.Borderless
                    onClicked: control.dispatchClicked(slotItem.index)
                }

                Component.onCompleted: {
                    control.applyCommonButtonProps(iconButton, slotConfig)
                    control.applyCommonButtonProps(menuButton, slotConfig)
                }

                onSlotConfigChanged: {
                    control.applyCommonButtonProps(iconButton, slotConfig)
                    control.applyCommonButtonProps(menuButton, slotConfig)
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ListFooter {
//     button1: { type: "icon", iconName: "general/projectStructure", onClicked: function() { console.log("first") } }
//     button2: { type: "menu", iconName: "general/projectStructure" }
//     button3: { type: "icon", iconName: "general/projectStructure" }
// }

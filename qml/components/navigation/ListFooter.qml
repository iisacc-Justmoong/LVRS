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
    property var button1: ({})
    property var button2: ({})
    property var button3: ({})
    property int horizontalPadding: Theme.gap2
    property int verticalPadding: Theme.gap2
    property int spacing: Theme.gapNone
    property bool interactive: true

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

    function applyCommonButtonProps(button, config) {
        const source = configValue(config, "iconSource", configValue(config, "url", undefined))
        button.tone = configValue(config, "tone", AbstractButton.Borderless)
        button.iconName = configValue(config, "iconName", "")
        button.iconGlyph = configValue(config, "iconGlyph", "")
        button.iconSize = configValue(config, "iconSize", Theme.iconSm)
        button.text = configValue(config, "text", "")
        button.enabled = control.interactive && configValue(config, "enabled", true)
        button.visible = configValue(config, "visible", true)
        if (source !== undefined)
            button.iconSource = source
        button.horizontalPadding = configValue(config, "horizontalPadding", Theme.gap2)
        button.verticalPadding = configValue(config, "verticalPadding", Theme.gap2)
        button.cornerRadius = configValue(config, "cornerRadius", Theme.radiusSm)
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

    implicitWidth: footerRow.implicitWidth + (horizontalPadding * 2)
    implicitHeight: footerRow.implicitHeight + (verticalPadding * 2)

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
                required property int index
                readonly property var slotConfig: control.buttonConfigAt(index)
                readonly property bool menuType: control.isMenuType(slotConfig)
                readonly property bool slotVisible: control.configValue(slotConfig, "visible", true)

                implicitWidth: menuType ? menuButton.implicitWidth : iconButton.implicitWidth
                implicitHeight: menuType ? menuButton.implicitHeight : iconButton.implicitHeight
                visible: slotVisible

                Layout.preferredWidth: implicitWidth
                Layout.preferredHeight: implicitHeight
                Layout.alignment: Qt.AlignVCenter

                IconButton {
                    id: iconButton
                    anchors.fill: parent
                    visible: !slotItem.menuType
                    tone: AbstractButton.Borderless
                    onClicked: control.dispatchClicked(slotItem.index)
                }

                IconMenuButton {
                    id: menuButton
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

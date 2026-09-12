pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    property string lastAction: "Press a control or choose a menu action."
    property color chosenColor: "#7A5AF8"
    readonly property bool buttonPage: catalogEntry.key === "help-button" || catalogEntry.key === "color-picker-button"
    implicitHeight: content.implicitHeight

    Column {
        id: content
        width: parent.width
        spacing: LV.Theme.gap16
        LV.Label {
            width: parent.width; style: body; sizeToContentHeight: true; wrapMode: Text.WordWrap
            text: root.buttonPage
                ? "Figma button sizes remain fixed while their surfaces compress and rebound. Tab and Space activate the same actions."
                : "ContextMenu: 18px rows · Inter Regular 12. Menu: 24px rows · Pretendard Medium 13."
        }
        Column {
            visible: root.buttonPage
            spacing: LV.Theme.gap16
            LV.Label { style: header2; text: "HelpButton · 21 × 21" }
            LV.HStack {
                spacing: LV.Theme.gap12
                LV.HelpButton { objectName: "parityHelp"; onClicked: root.lastAction = "Help requested" }
                LV.HelpButton { enabled: false }
                LV.Label { text: "Text glyph ? · SemiBold 12 · panelBackground04" }
            }
            LV.Label { style: header2; text: "ColorPickerButton · 22 / 28 / 36" }
            LV.HStack {
                spacing: LV.Theme.gap12
                Repeater {
                    model: [22, 28, 36]
                    LV.ColorPickerButton {
                        required property int modelData
                        buttonSize: modelData
                        currentColor: root.chosenColor
                        onClicked: {
                            root.chosenColor = root.chosenColor.r < 0.5 ? "#FF453A" : "#7A5AF8"
                            root.lastAction = "Current color changed to " + root.chosenColor
                        }
                    }
                }
                LV.ColorPickerButton { showHueRing: false; currentColor: root.chosenColor; onClicked: root.lastAction = "Ringless trigger" }
                LV.ColorPickerButton { enabled: false; currentColor: root.chosenColor }
            }
            LV.Label { style: caption; text: "Focus: 2px accent border · Disabled: 32% opacity · Padding: 4px" }
        }
        Column {
            visible: !root.buttonPage
            spacing: LV.Theme.gap12
            LV.Label { style: header2; text: "Authored row and divider sizes" }
            LV.ContextMenuItem { label: "Label"; onClicked: root.lastAction = "Compact row activated" }
            LV.ContextMenuDivider {}
            LV.MenuItem { label: "Label"; onClicked: root.lastAction = "Regular row activated" }
            LV.MenuDivider {}
            LV.HStack {
                spacing: LV.Theme.gap12
                LV.LabelButton {
                    text: "Open ContextMenu"
                    onClicked: {
                        const p = mapToItem(null, 0, height + 8)
                        compactMenu.openAt(p.x, p.y)
                    }
                }
                LV.LabelButton {
                    text: "Open Menu"
                    onClicked: {
                        const p = mapToItem(null, 0, height + 8)
                        regularMenu.openAt(p.x, p.y)
                    }
                }
            }
        }
        LV.Label { width: parent.width; style: body; text: root.lastAction; wrapMode: Text.WordWrap; sizeToContentHeight: true }
    }
    LV.ContextMenu {
        id: compactMenu
        items: [{label: "Label", key: "Key"}, {label: "Label", key: "Key"}, {type: "divider"},
            {label: "Label", key: "Key"}, {label: "Label", key: "Key"}, {label: "Label", key: "Key"}]
        onItemTriggered: (index, item) => root.lastAction = "ContextMenu action " + (index + 1)
    }
    LV.Menu {
        id: regularMenu
        items: [{label: "Label", key: "key"}, {label: "Label", key: "key"}, {label: "Label", key: "key"},
            {type: "divider"}, {label: "Label", key: "key"}, {label: "Label", key: "key"},
            {label: "Label", key: "key"}, {type: "divider"}, {label: "Label", key: "key"}, {label: "Label", key: "key"}]
        onItemTriggered: (index, item) => root.lastAction = "Menu action " + (index + 1)
    }
}

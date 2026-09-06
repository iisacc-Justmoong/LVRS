pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

FocusScope {
    id: control
    required property var listItem
    property var config: ({})
    property bool unit: false
    readonly property var options: listItem.configValue(config, "items", [])
    readonly property int currentIndex: unit ? listItem.unitIndex : listItem.selectorIndex
    readonly property int count: options ? (options.length === undefined ? options.count || 0 : options.length) : 0
    readonly property string fallbackText: unit ? "Units" : listItem.variantName === "DetailQuantity" ? "Format" : listItem.variantName === "DetailSettings" ? "Automatic" : listItem.variantName === "Task" ? "Priority" : listItem.variantName === "Form" ? "Category" : "Option"
    readonly property alias popup: menu

    implicitWidth: Theme.scaleMetric(97)
    implicitHeight: Theme.scaleMetric(20)
    enabled: listItem.effectiveEnabled && listItem.configValue(config, "enabled", true)
    activeFocusOnTab: enabled
    Accessible.role: Accessible.ComboBox
    Accessible.name: display.text

    function choose(index) {
        if (!enabled || index < 0 || index >= count)
            return false;
        const option = listItem.optionAt(options, index);
        if (option && typeof option === "object" && option.enabled === false)
            return false;
        return listItem.editValue(unit ? "unitIndex" : "selectorIndex", index);
    }

    function openMenu() {
        if (!enabled || count === 0)
            return;
        forceActiveFocus(Qt.TabFocusReason);
        menu.openFor(control, 0, height);
    }

    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            openMenu();
            event.accepted = true;
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            const direction = event.key === Qt.Key_Up ? -1 : 1;
            for (let i = currentIndex + direction; i >= 0 && i < count; i += direction) {
                if (choose(i))
                    break;
            }
            event.accepted = true;
        }
    }

    ComboBox {
        id: display
        objectName: control.unit ? "listItem_unitSelector" : "listItem_selector"
        anchors.fill: parent
        text: control.listItem.optionText(control.listItem.optionAt(control.options, control.currentIndex), control.listItem.configValue(control.config, "text", control.fallbackText))
        tone: control.listItem.configValue(control.config, "tone", ComboBox.Primary)
        arrow: control.listItem.configValue(control.config, "arrow", Stepper.Down)
        onClicked: control.openMenu()
    }

    ContextMenu {
        id: menu
        objectName: control.unit ? "listItem_unitMenu" : "listItem_selectorMenu"
        items: control.options
        selectedIndex: control.currentIndex
        showIconSlot: false
        itemWidth: control.width
        onItemTriggered: function (index, item) {
            control.choose(index);
        }
    }
}

// Internal ListItem selector: reuses LVRS ComboBox and ContextMenu.

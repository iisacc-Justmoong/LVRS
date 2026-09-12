pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    property int actionCount: 0
    property bool expanded: false
    property bool faded: false
    implicitHeight: content.implicitHeight

    Column {
        id: content
        width: parent.width
        spacing: LV.Theme.gap20
        LV.Label { style: header2; text: "One rhythm, different responses" }
        Flow {
            width: parent.width; spacing: LV.Theme.gap16
            LV.PushButton { text: "Hold & release"; onClicked: root.actionCount++ }
            LV.CheckBox { text: "Checked state" }
            LV.RadioButton { text: "Selected dot" }
            LV.ToggleSwitch { text: "Drag or tap" }
        }
        LV.Label { style: caption; text: "Actions delivered immediately: " + root.actionCount }
        LV.InputField { width: Math.min(360, parent.width); placeholderText: "Tab here to inspect focus motion" }
        LV.Slider { id: valueSlider; width: Math.min(400, parent.width); value: 0.35 }
        LV.ProgressBar { width: valueSlider.width; currentValue: valueSlider.value * 100 }
        Row {
            spacing: LV.Theme.gap12
            LV.LabelButton { text: "Reflow spacing"; onClicked: root.expanded = !root.expanded }
            LV.LabelButton { text: "Fade layers"; onClicked: root.faded = !root.faded }
            LV.LabelButton { text: "Jump value"; onClicked: valueSlider.value = valueSlider.value < 0.5 ? 0.85 : 0.15 }
        }
        LV.HStack {
            spacing: root.expanded ? 48 : 8
            LV.LabelButton { text: "A" }
            LV.Spacer { minLength: root.expanded ? 32 : 0 }
            LV.LabelButton { text: "B" }
            LV.LabelButton { text: "C" }
        }
        LV.VStack {
            alignment: Qt.AlignLeft
            spacing: root.expanded ? 32 : 8
            LV.Label { text: "Vertical spacing" }
            LV.Label { text: "The layout owns child geometry" }
        }
        LV.ZStack {
            width: Math.min(400, root.width); height: 56
            LV.AppCard {
                width: 400; height: 56
                color: root.expanded ? LV.Theme.accentMuted : LV.Theme.panelBackground05
            }
            LV.Label { text: "Layer opacity"; opacity: root.faded ? 0.2 : 1 }
        }
        LV.MenuDivider { width: Math.min(400, root.width); opacity: root.faded ? 0.15 : 1 }
    }
}

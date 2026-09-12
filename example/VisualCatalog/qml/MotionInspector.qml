pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS as LV

LV.AppCard {
    id: root
    property var entry: ({})
    property var guide: ({})
    title: "Interaction & motion"
    subtitle: entry.label || "LVRS motion system"
    implicitHeight: details.implicitHeight + 100

    Column {
        id: details
        width: parent.width
        spacing: LV.Theme.gap16

        Repeater {
            model: [
                { label: "TRY IT", value: root.guide.trigger || "Choose a component in the index." },
                { label: "RESPONSE", value: root.guide.response || "Every interactive family shares one motion policy." },
                { label: "LOOK FOR", value: root.guide.observe || "Press, release, and interrupt the return animation." },
                { label: "BEHAVIOR CONTRACT", value: root.guide.contract || "Visual motion preserves semantic state and signal timing." }
            ]
            delegate: Column {
                required property var modelData
                width: details.width
                spacing: LV.Theme.gap4
                LV.Label {
                    style: caption; color: LV.Theme.primary
                    text: modelData.label
                }
                LV.Label {
                    width: parent.width; style: body; wrapMode: Text.WordWrap
                    text: modelData.value
                }
            }
        }
        Flow {
            width: parent.width; spacing: LV.Theme.gap12
            Repeater {
                model: ["Press " + LV.Motion.duration(LV.Motion.pressDuration) + " ms",
                        "Return " + LV.Motion.duration(LV.Motion.releaseDuration) + " ms",
                        "Surface " + LV.Motion.duration(LV.Motion.surfaceDuration) + " ms",
                        "Overshoot " + LV.Motion.overshoot]
                delegate: LV.Label {
                    required property string modelData
                    style: caption; text: modelData; color: LV.Theme.descriptionColor
                }
            }
        }
    }
}

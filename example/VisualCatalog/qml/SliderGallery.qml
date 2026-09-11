pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    property string feedback: "Drag a thumb or use the arrow keys."
    readonly property var typeNames: ["Default", "Center biased", "Ticks", "Center biased ticks", "Filled", "Min/max labels", "Segmented"]
    readonly property var sizeNames: ["Mini · 22", "Small · 24", "Regular · 32", "Large · 44"]
    readonly property var stateNames: ["Idle", "Hover", "Pressed", "Disabled"]
    readonly property real examplesHeight: liveExamples.y + liveExamples.height
    implicitWidth: 1100
    implicitHeight: content.implicitHeight
    height: implicitHeight

    Column {
        id: content
        width: parent.width
        spacing: LV.Theme.gap24

        LV.Label {
            style: header
            text: "Slider / LVRS"
        }
        LV.Label {
            width: parent.width
            style: body
            text: root.feedback
            wrapMode: Text.WordWrap
        }
        Flow {
            id: liveExamples
            width: parent.width
            spacing: LV.Theme.gap16
            Repeater {
                model: 7
                delegate: Rectangle {
                    id: example
                    required property int index
                    width: (content.width - (content.width >= 850 ? 32 : content.width >= 560 ? 16 : 0)) / (content.width >= 850 ? 3 : content.width >= 560 ? 2 : 1)
                    height: 144
                    radius: LV.Theme.radiusLg
                    color: LV.Theme.surfaceSolid
                    Column {
                        x: 16
                        y: 16
                        width: parent.width - 32
                        spacing: 16
                        LV.Label { style: header2; text: root.typeNames[example.index] }
                        LV.Slider {
                            id: liveSlider
                            objectName: "slider_example_" + example.index
                            width: parent.width
                            type: example.index
                            size: example.index === LV.Slider.Filled ? LV.Slider.Large : LV.Slider.Regular
                            from: centerBiased ? -1 : 0
                            to: 1
                            value: type === LV.Slider.CenterBiasedTicks ? -0.5 : 0.5
                            Accessible.name: root.typeNames[example.index]
                            onMoved: root.feedback = root.typeNames[example.index] + " · " + value.toFixed(2)
                        }
                        LV.Label {
                            style: caption
                            text: "Value " + liveSlider.value.toFixed(2) + (liveSlider.segmented ? " · 5 stops" : " · continuous")
                        }
                    }
                }
            }
        }
        LV.Label { style: header2; text: "Endpoint icons / inactive window / keyboard focus" }
        Column {
            width: Math.min(parent.width, 480)
            spacing: LV.Theme.gap16
            LV.Slider {
                width: parent.width
                type: LV.Slider.MinMaxLabels
                showEndpointIcons: true
                minimumLabel: "Low"
                maximumLabel: "High"
                Accessible.name: "Intensity with endpoint icons"
            }
            LV.Slider {
                width: parent.width
                size: LV.Slider.Small
                active: false
                value: 0.75
                Accessible.name: "Inactive window example"
            }
            LV.Slider {
                width: parent.width
                size: LV.Slider.Small
                showFocusRing: true
                value: 0.25
                Accessible.name: "Focus ring example"
            }
        }
        LV.Label { style: header; text: "All 112 variants" }
        Repeater {
            model: 7
            delegate: Column {
                id: family
                required property int index
                width: content.width
                spacing: LV.Theme.gap12
                LV.Label { style: header2; text: root.typeNames[family.index] }
                Flow {
                    width: parent.width
                    spacing: LV.Theme.gap12
                    Repeater {
                        model: 16
                        delegate: Rectangle {
                            id: sample
                            required property int index
                            readonly property int sampleSize: Math.floor(index / 4)
                            readonly property int sampleState: index % 4
                            width: (family.width - (family.width >= 1000 ? 36 : family.width >= 500 ? 12 : 0)) / (family.width >= 1000 ? 4 : family.width >= 500 ? 2 : 1)
                            height: 86
                            radius: LV.Theme.radiusMd
                            color: LV.Theme.surfaceSolid
                            LV.Label {
                                x: 12
                                y: 10
                                style: caption
                                text: root.sizeNames[sample.sampleSize] + " / " + root.stateNames[sample.sampleState]
                            }
                            LV.Slider {
                                objectName: "slider_variant_" + family.index + "_" + sample.index
                                x: 12
                                y: 34 + (44 - height) / 2
                                width: parent.width - 24
                                type: family.index
                                size: sample.sampleSize
                                displayState: sample.sampleState + 1
                                value: 0.5
                                Accessible.name: root.typeNames[family.index] + " " + root.sizeNames[sample.sampleSize] + " " + root.stateNames[sample.sampleState]
                            }
                        }
                    }
                }
            }
        }
    }
}

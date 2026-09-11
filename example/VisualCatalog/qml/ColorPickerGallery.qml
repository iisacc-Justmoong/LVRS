pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    property var catalogEntry: ({})
    property color selectedColor: "#7a5af8"
    property color committedColor: "#0a84ff"
    property color modalStartColor: "#7a5af8"
    property string feedback: "Edit a channel, drag a color field, or choose a recent color."
    readonly property var typeNames: ["Wheel", "Hue–Saturation", "Saturation–Brightness", "Grayscale", "RGB", "CMYK"]
    implicitWidth: 1032
    implicitHeight: content.implicitHeight
    height: implicitHeight

    Column {
        id: content
        width: parent.width
        spacing: LV.Theme.gap24
        LV.Label {
            style: header
            text: "ColorPicker · reusable content view"
        }
        LV.Label {
            width: parent.width
            style: body
            text: root.feedback
            color: LV.Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        Row {
            spacing: LV.Theme.gap12
            LV.LabelButton {
                text: "Open in Modal"
                onClicked: {
                    root.modalStartColor = root.selectedColor;
                    colorModal.open = true;
                }
            }
            Rectangle {
                width: 44
                height: 22
                radius: LV.Theme.radiusSm
                color: root.selectedColor
            }
            LV.Label {
                style: body
                text: String(root.selectedColor).toUpperCase()
            }
        }
        Flow {
            width: parent.width
            spacing: LV.Theme.gap24
            Repeater {
                model: 6
                delegate: Column {
                    id: sample
                    required property int index
                    width: 320
                    spacing: LV.Theme.gap8
                    LV.Label {
                        style: header2
                        text: root.typeNames[sample.index]
                    }
                    Rectangle {
                        width: parent.width
                        height: picker.implicitHeight
                        radius: LV.Theme.radiusLg
                        color: LV.Theme.panelBackground08
                        LV.ColorPicker {
                            id: picker
                            objectName: "gallery_colorPicker_" + sample.index
                            width: parent.width
                            type: sample.index
                            currentColor: root.selectedColor
                            previousColor: root.committedColor
                            onColorEdited: function (color) {
                                root.selectedColor = color;
                            }
                            onAccepted: function (color) {
                                root.committedColor = color;
                                root.feedback = "Applied " + String(color).toUpperCase();
                            }
                            onCanceled: root.feedback = "Restored the previous color."
                            onEyedropperRequested: root.feedback = "Eyedropper requested. The host supplies a sampled document color through setColor()."
                        }
                    }
                }
            }
        }
    }

    Component {
        id: modalColorView
        LV.ColorPicker {
            objectName: "gallery_modalColorPicker"
            type: LV.ColorPicker.SaturationBrightness
            currentColor: root.selectedColor
            previousColor: root.modalStartColor
            onColorEdited: function (color) {
                root.selectedColor = color;
            }
            onAccepted: function (color) {
                root.committedColor = color;
                root.feedback = "Applied from Modal: " + String(color).toUpperCase();
                colorModal.open = false;
            }
            onCanceled: colorModal.open = false
            onEyedropperRequested: root.feedback = "The host handles document color sampling."
        }
    }

    LV.Modal {
        id: colorModal
        objectName: "gallery_colorModal"
        minWidth: 360
        maxWidth: 360
        frameMinHeight: 0
        showIcon: false
        primaryText: ""
        contentComponent: modalColorView
        onCanceled: root.selectedColor = root.modalStartColor
    }
}

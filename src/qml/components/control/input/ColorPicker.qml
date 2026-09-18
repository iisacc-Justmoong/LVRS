pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: root

    enum PickerType {
        Wheel,
        HueSaturation,
        SaturationBrightness,
        Grayscale,
        RGB,
        CMYK
    }
    property int type: ColorPicker.Wheel
    property alias currentColor: colorState.color
    property alias previousColor: colorState.previousColor
    property bool showRecentColors: true
    property bool showActions: true
    property var recentColors: ["#7A5AF8", "#0A84FF", "#32D7E8", "#38D996", "#FFD65A", "#FF9F45", "#FF647C", "#C28AFF"]
    property string applyText: qsTr("Apply")
    property string cancelText: qsTr("Cancel")
    property real padding: Theme.gap12
    readonly property string hex: colorState.hex
    readonly property int effectiveType: Math.max(ColorPicker.Wheel, Math.min(ColorPicker.CMYK, type))
    readonly property bool hasPlane: effectiveType === ColorPicker.HueSaturation || effectiveType === ColorPicker.SaturationBrightness
    readonly property var channels: effectiveType === ColorPicker.Wheel ? [0, 1, 2] : effectiveType === ColorPicker.Grayscale ? [11] : effectiveType === ColorPicker.RGB ? [3, 4, 5] : effectiveType === ColorPicker.CMYK ? [6, 7, 8, 9] : []
    readonly property var channelLabels: ["H", "S", "B", "R", "G", "B", "C", "M", "Y", "K", "A", "K"]
    readonly property var channelNames: [qsTr("Hue"), qsTr("Saturation"), qsTr("Brightness"), qsTr("Red"), qsTr("Green"), qsTr("Blue"), qsTr("Cyan"), qsTr("Magenta"), qsTr("Yellow"), qsTr("Black"), qsTr("Alpha"), qsTr("Grayscale")]

    signal colorEdited(color color)
    signal accepted(color color)
    signal canceled
    signal eyedropperRequested

    // These edit the native property without removing a caller's QML binding.
    function setColor(color: color) {
        colorState.editColor(color);
    }
    function setHex(text: string): bool {
        return colorState.editHex(text);
    }
    function accept() {
        root.forceActiveFocus();
        colorState.accept();
        root.accepted(root.currentColor);
    }
    function cancel() {
        root.forceActiveFocus();
        colorState.cancel();
        root.canceled();
    }

    implicitWidth: Theme.scaleMetric(320)
    implicitHeight: content.implicitHeight + padding * 2
    opacity: enabled ? 1 : 0.32
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Color picker")
    Keys.onEscapePressed: root.cancel()

    ColorPickerModel {
        id: colorState
        objectName: "colorPicker_state"
        onColorEdited: function (color) {
            root.colorEdited(color);
        }
    }

    component ColorInput: InputField {
        id: entry
        property string displayText: ""
        property bool edited: false
        signal committed(string value)
        style: inlineStyle
        clearButtonVisible: false
        height: Theme.controlHeightSm
        function commit() {
            if (edited) {
                edited = false;
                committed(text);
            }
            text = displayText;
        }
        onTextEdited: edited = true
        Component.onCompleted: text = displayText
        onDisplayTextChanged: {
            if (!inputItem.activeFocus)
                text = displayText;
        }
        onAccepted: entry.commit()
        Connections {
            target: entry.inputItem
            function onActiveFocusChanged() {
                if (!entry.inputItem.activeFocus)
                    entry.commit();
            }
        }
    }

    component Swatch: Canvas {
        id: swatch
        property color swatchColor: "transparent"
        property color previous: swatchColor
        property bool split: false
        onSwatchColorChanged: requestPaint()
        onPreviousChanged: requestPaint()
        onSplitChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            ctx.beginPath();
            ctx.roundedRect(0, 0, width, height, 4, 4);
            ctx.clip();
            for (let y = 0; y < height; y += 4) {
                for (let x = 0; x < width; x += 4) {
                    ctx.fillStyle = ((x / 4 + y / 4) % 2) ? "#707070" : "#B0B0B0";
                    ctx.fillRect(x, y, 4, 4);
                }
            }
            ctx.fillStyle = swatchColor;
            ctx.fillRect(split ? width / 2 : 0, 0, split ? width / 2 : width, height);
            if (split) {
                ctx.fillStyle = previous;
                ctx.fillRect(0, 0, width / 2, height);
            }
        }
    }

    component ChannelRow: Item {
        id: channelRow
        required property int channel
        readonly property real maximum: channel === 0 ? 360 : channel >= 3 && channel <= 5 ? 255 : 100
        readonly property real channelValue: {
            const revision = colorState.revision;
            return colorState.channelValue(channel);
        }
        height: Theme.controlHeightSm
        implicitHeight: height
        Label {
            width: 12
            anchors.verticalCenter: parent.verticalCenter
            style: body
            color: Theme.descriptionColor
            text: root.channelLabels[channelRow.channel]
        }
        Slider {
            id: channelSlider
            objectName: "colorPicker_channel_" + channelRow.channel
            x: 20
            width: Math.max(16, channelRow.width - 100)
            size: Slider.Mini
            from: 0
            to: channelRow.maximum
            value: channelRow.channelValue
            stepSize: 1
            Accessible.name: root.channelNames[channelRow.channel]
            onMoved: colorState.editChannel(channelRow.channel, value)
            function rampColor(position: real): color {
                const revision = colorState.revision;
                return colorState.channelColor(channelRow.channel, position);
            }
            background: Rectangle {
                x: channelSlider.rangeInset
                y: (channelSlider.height - height) / 2
                width: channelSlider.rangeWidth
                height: channelSlider.trackHeight
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0
                        color: channelSlider.rampColor(0)
                    }
                    GradientStop {
                        position: 1 / 6
                        color: channelSlider.rampColor(1 / 6)
                    }
                    GradientStop {
                        position: 2 / 6
                        color: channelSlider.rampColor(2 / 6)
                    }
                    GradientStop {
                        position: 3 / 6
                        color: channelSlider.rampColor(3 / 6)
                    }
                    GradientStop {
                        position: 4 / 6
                        color: channelSlider.rampColor(4 / 6)
                    }
                    GradientStop {
                        position: 5 / 6
                        color: channelSlider.rampColor(5 / 6)
                    }
                    GradientStop {
                        position: 1
                        color: channelSlider.rampColor(1)
                    }
                }
            }
        }
        ColorInput {
            objectName: "colorPicker_channelInput_" + channelRow.channel
            x: channelRow.width - 72
            width: 52
            displayText: String(Math.round(channelRow.channelValue))
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            Accessible.name: root.channelNames[channelRow.channel]
            onCommitted: function (value) {
                if (value.trim().length && isFinite(Number(value)))
                    colorState.editChannel(channelRow.channel, Number(value));
            }
        }
        Label {
            x: parent.width - 12
            width: 12
            anchors.verticalCenter: parent.verticalCenter
            style: caption
            color: Theme.descriptionColor
            text: channelRow.channel === 0 ? "°" : channelRow.channel >= 3 && channelRow.channel <= 5 ? "" : "%"
        }
    }

    component ColorField: ColorPickerSurface {
        model: colorState
        Accessible.role: Accessible.Slider
        Accessible.name: field === ColorPickerSurface.Wheel ? qsTr("Hue wheel and saturation brightness triangle") : field === ColorPickerSurface.HueSaturation ? qsTr("Hue and saturation") : field === ColorPickerSurface.SaturationBrightness ? qsTr("Saturation and brightness") : field === ColorPickerSurface.Spectrum ? qsTr("Color spectrum") : field === ColorPickerSurface.HueRail ? qsTr("Hue") : qsTr("Brightness")
        Accessible.description: "#" + root.hex
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: Theme.radiusSm
            border.width: parent.activeFocus ? 1 : 0
            border.color: Theme.accent
        }
    }

    Column {
        id: content
        x: root.padding
        y: root.padding
        width: Math.max(0, root.width - root.padding * 2)
        spacing: Theme.gap12

        Column {
            width: parent.width
            spacing: Theme.gap4
            visible: root.channels.length > 0
            Repeater {
                model: root.channels
                delegate: ChannelRow {
                    required property int modelData
                    width: content.width
                    channel: modelData
                }
            }
        }

        Item {
            objectName: "colorPicker_domain"
            width: parent.width
            height: root.hasPlane ? Math.max(0, width - 36) : root.effectiveType === ColorPicker.Wheel ? Math.min(width, 232) : 184
            ColorField {
                objectName: "colorPicker_field"
                width: root.hasPlane ? Math.max(0, parent.width - 36) : parent.width
                height: parent.height
                field: root.effectiveType === ColorPicker.Wheel ? ColorPickerSurface.Wheel : root.effectiveType === ColorPicker.HueSaturation ? ColorPickerSurface.HueSaturation : root.effectiveType === ColorPicker.SaturationBrightness ? ColorPickerSurface.SaturationBrightness : ColorPickerSurface.Spectrum
            }
            ColorField {
                objectName: "colorPicker_rail"
                visible: root.hasPlane
                anchors.right: parent.right
                width: 24
                height: parent.height
                field: root.effectiveType === ColorPicker.HueSaturation ? ColorPickerSurface.BrightnessRail : ColorPickerSurface.HueRail
            }
        }

        ChannelRow {
            width: parent.width
            visible: root.hasPlane
            channel: root.effectiveType === ColorPicker.HueSaturation ? ColorPickerModel.Brightness : ColorPickerModel.Hue
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Qt.rgba(1, 1, 1, 0.11)
        }

        Item {
            id: values
            objectName: "colorPicker_values"
            width: parent.width
            height: 28
            Swatch {
                objectName: "colorPicker_comparison"
                width: 40
                height: 28
                swatchColor: root.currentColor
                previous: root.previousColor
                split: true
            }
            Label {
                x: 48
                width: 25
                anchors.verticalCenter: parent.verticalCenter
                style: caption
                color: Theme.descriptionColor
                text: qsTr("Hex")
            }
            ColorInput {
                objectName: "colorPicker_hex"
                x: 81
                width: Math.max(40, values.width - 187)
                anchors.verticalCenter: parent.verticalCenter
                displayText: root.hex
                maximumLength: 9
                Accessible.name: qsTr("Hex color, RRGGBB or RRGGBBAA")
                onCommitted: function (value) {
                    colorState.editHex(value);
                }
            }
            Label {
                x: values.width - 98
                width: 10
                anchors.verticalCenter: parent.verticalCenter
                style: caption
                color: Theme.descriptionColor
                text: "A"
            }
            ColorInput {
                objectName: "colorPicker_alpha"
                x: values.width - 80
                width: 56
                anchors.verticalCenter: parent.verticalCenter
                displayText: String(Math.round(root.currentColor.a * 100))
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                Accessible.name: qsTr("Opacity percent")
                onCommitted: function (value) {
                    if (value.trim().length && isFinite(Number(value)))
                        colorState.editChannel(ColorPickerModel.Alpha, Number(value));
                }
            }
            Label {
                x: values.width - 16
                width: 16
                anchors.verticalCenter: parent.verticalCenter
                style: caption
                color: Theme.descriptionColor
                text: "%"
            }
        }

        Item {
            objectName: "colorPicker_recent"
            width: parent.width
            height: 20
            visible: root.showRecentColors
            Label {
                width: 40
                anchors.verticalCenter: parent.verticalCenter
                style: caption
                color: Theme.descriptionColor
                text: qsTr("Recent")
            }
            Row {
                x: 48
                spacing: Theme.gap8
                Repeater {
                    model: root.recentColors.slice(0, 8)
                    delegate: AbstractButton {
                        id: recentButton
                        required property color modelData
                        required property int index
                        objectName: "colorPicker_recent_" + index
                        width: Math.max(1, (content.width - 48 - 56) / 8)
                        height: 20
                        Accessible.name: qsTr("Recent color %1").arg(String(modelData))
                        onClicked: colorState.editColor(modelData)
                        background: Swatch {
                            swatchColor: recentButton.modelData
                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.radiusSm
                                color: "transparent"
                                border.width: recentButton.activeFocus || Qt.colorEqual(root.currentColor, recentButton.modelData) ? 1.5 : 0.5
                                border.color: recentButton.activeFocus ? Theme.accent : Qt.colorEqual(root.currentColor, recentButton.modelData) ? Qt.rgba(1, 1, 1, .8) : Qt.rgba(1, 1, 1, .12)
                            }
                        }
                        contentItem: Item {}
                    }
                }
            }
        }

        Item {
            objectName: "colorPicker_actions"
            width: parent.width
            height: Theme.controlHeightSm
            visible: root.showActions
            IconButton {
                objectName: "colorPicker_eyedropper"
                width: 22
                height: 22
                iconName: "imagecolorPicker"
                tone: AbstractButton.Default
                Accessible.name: qsTr("Pick a color from the document")
                onClicked: root.eyedropperRequested()
            }
            Label {
                x: 30
                width: Math.max(0, actionButtons.x - 38)
                anchors.verticalCenter: parent.verticalCenter
                style: caption
                color: Theme.descriptionColor
                text: "sRGB"
            }
            Row {
                id: actionButtons
                anchors.right: parent.right
                spacing: Theme.gap8
                LabelButton {
                    objectName: "colorPicker_cancel"
                    text: root.cancelText
                    tone: AbstractButton.Default
                    onClicked: root.cancel()
                }
                LabelButton {
                    objectName: "colorPicker_apply"
                    text: root.applyText
                    tone: AbstractButton.Primary
                    onClicked: root.accept()
                }
            }
        }
    }
}

// API usage (external):
// Component { id: colorView; LV.ColorPicker { currentColor: document.color; onColorEdited: color => document.color = color } }
// LV.Modal { contentComponent: colorView; showIcon: false; primaryText: "" }

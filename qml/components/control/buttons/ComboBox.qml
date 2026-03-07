import QtQuick
import LVRS 1.0

Item {
    id: control

    enum Tone {
        Primary,
        Borderless
    }

    property int tone: ComboBox.Primary
    property int arrow: Stepper.UpDown
    property alias text: label.text

    signal clicked()
    signal pressed()
    signal released()
    signal canceled()

    readonly property int figmaComboWidth: 97
    readonly property int figmaComboHeight: 18
    readonly property int figmaComboLeftPadding: Theme.gap8
    readonly property int figmaComboRightPadding: 1
    readonly property int figmaComboVerticalPadding: 1
    readonly property int figmaComboCornerRadius: Theme.radiusControl
    readonly property int figmaIndicatorSize: Theme.iconSm
    readonly property int figmaLabelLineHeight: Theme.textBodyLineHeight
    readonly property int resolvedTone: tone === ComboBox.Borderless
        ? ComboBox.Borderless
        : ComboBox.Primary
    readonly property int resolvedArrow: arrow === Stepper.Up
        ? Stepper.Up
        : arrow === Stepper.Down
            ? Stepper.Down
            : Stepper.UpDown

    readonly property color backgroundColor: Theme.panelBackground10
    readonly property color backgroundColorHover: Theme.panelBackground11
    readonly property color backgroundColorPressed: Theme.panelBackground12
    readonly property color resolvedBackgroundColor: interactionArea.pressed
        ? backgroundColorPressed
        : interactionArea.containsMouse
            ? backgroundColorHover
            : backgroundColor
    readonly property real indicatorX: width - figmaComboRightPadding - figmaIndicatorSize
    readonly property real indicatorY: Math.round((height - figmaIndicatorSize) * 0.5)
    readonly property real labelX: figmaComboLeftPadding
    readonly property real labelAvailableWidth: Math.max(0, indicatorX - figmaComboLeftPadding)
    readonly property real labelY: Math.round((height - figmaLabelLineHeight) * 0.5)
    readonly property rect indicatorBounds: Qt.rect(indicator.x, indicator.y, indicator.width, indicator.height)
    readonly property rect labelBounds: Qt.rect(label.x, label.y, label.width, label.height)

    implicitWidth: figmaComboWidth
    implicitHeight: figmaComboHeight
    width: figmaComboWidth
    height: figmaComboHeight
    clip: true

    Rectangle {
        anchors.fill: parent
        radius: control.figmaComboCornerRadius
        color: control.resolvedBackgroundColor
        antialiasing: true
    }

    Label {
        id: label
        x: control.labelX
        y: control.labelY
        width: control.labelAvailableWidth
        height: control.figmaLabelLineHeight
        style: body
        text: "Label"
        color: Theme.accentWhite
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        lineHeight: control.figmaLabelLineHeight
        lineHeightMode: Text.FixedHeight
    }

    Stepper {
        id: indicator
        x: control.indicatorX
        y: control.indicatorY
        tone: control.resolvedTone === ComboBox.Borderless
            ? AbstractButton.Borderless
            : AbstractButton.Primary
        arrow: control.resolvedArrow
    }

    MouseArea {
        id: interactionArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        preventStealing: true

        onPressed: control.pressed()
        onReleased: control.released()
        onCanceled: control.canceled()
        onClicked: control.clicked()
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ComboBox { tone: LV.ComboBox.Borderless; arrow: LV.Stepper.Down }

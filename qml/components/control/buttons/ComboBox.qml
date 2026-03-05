import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    enum ComboTone {
        Primary,
        Borderless
    }

    property int tone: ComboBox.Primary
    property int arrow: Stepper.UpDown

    signal clicked()
    signal pressed()
    signal released()
    signal canceled()

    readonly property int figmaComboWidth: 97
    readonly property int figmaComboHeight: 20
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

    implicitWidth: figmaComboWidth
    implicitHeight: figmaComboHeight
    width: figmaComboWidth
    height: figmaComboHeight

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusBase
        color: control.resolvedBackgroundColor
        antialiasing: true
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.gap8
        anchors.rightMargin: Theme.gap2
        anchors.topMargin: Theme.gap2
        anchors.bottomMargin: Theme.gap2
        spacing: Theme.gapNone

        Label {
            style: body
            text: "Label"
            color: Theme.accentWhite
            lineHeight: 12
            lineHeightMode: Text.FixedHeight
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            Layout.fillWidth: true
        }

        Stepper {
            id: indicator
            tone: control.resolvedTone === ComboBox.Borderless
                ? AbstractButton.Borderless
                : AbstractButton.Primary
            arrow: control.resolvedArrow
            Layout.alignment: Qt.AlignVCenter
        }
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

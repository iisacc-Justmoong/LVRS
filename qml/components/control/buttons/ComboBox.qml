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
        radius: control.figmaComboCornerRadius
        color: control.resolvedBackgroundColor
        antialiasing: true
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: control.figmaComboLeftPadding
        anchors.rightMargin: control.figmaComboRightPadding
        anchors.topMargin: control.figmaComboVerticalPadding
        anchors.bottomMargin: control.figmaComboVerticalPadding
        spacing: Theme.gapNone

        Label {
            id: label
            style: body
            text: "Label"
            color: Theme.accentWhite
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            lineHeight: 12
            lineHeightMode: Text.FixedHeight
            Layout.alignment: Qt.AlignVCenter
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

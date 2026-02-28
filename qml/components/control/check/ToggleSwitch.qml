import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Switch {
    id: control

    text: ""
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1

    property int shapeStyle: shapeRoundRect
    property int trackWidth: Theme.toggleTrackWidth
    property int trackHeight: Theme.controlHeightSm
    property int trackPadding: Theme.gap2
    property int knobSize: Theme.controlIndicatorSize
    property int transitionDuration: Theme.toggleTransitionDuration
    property real trackCornerRadius: trackHeight / 2

    property color onColor: Theme.accent
    property color offColor: Theme.panelBackground12
    property color onColorHover: Qt.darker(onColor, 1.08)
    property color onColorPressed: Qt.darker(onColor, 1.18)
    property color offColorHover: Qt.lighter(offColor, 1.08)
    property color offColorPressed: Qt.darker(offColor, 1.12)
    property color disabledTrackColor: Theme.surfaceAlt
    property color trackShadowColor: Theme.shadowStrong
    property color knobFillColor: Theme.textPrimary
    readonly property int knobXOff: trackPadding
    readonly property int knobXOn: Math.max(trackPadding, trackWidth - knobSize - trackPadding)
    readonly property color resolvedTrackColor: !control.enabled
        ? control.disabledTrackColor
        : control.checked
            ? (control.down ? control.onColorPressed
                            : (control.hovered ? control.onColorHover : control.onColor))
            : (control.down ? control.offColorPressed
                            : (control.hovered ? control.offColorHover : control.offColor))

    function resolvedTrackRadius(rectWidth, rectHeight) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return trackCornerRadius
    }

    spacing: text.length > 0 ? Theme.gap8 : Theme.gapNone
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    implicitWidth: indicator.implicitWidth
                   + (text.length > 0 ? spacing + contentItem.implicitWidth : 0)
    implicitHeight: Math.max(indicator.implicitHeight, contentItem.implicitHeight)

    indicator: Item {
        implicitWidth: control.trackWidth
        implicitHeight: control.trackHeight

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 1
            radius: control.resolvedTrackRadius(width, height)
            color: control.trackShadowColor
            opacity: 0.3
        }

        Rectangle {
            id: track
            anchors.fill: parent
            radius: control.resolvedTrackRadius(width, height)
            color: control.resolvedTrackColor
            antialiasing: true
        }

        Item {
            id: knob
            width: control.knobSize
            height: control.knobSize
            y: (track.height - height) / 2
            x: control.checked ? control.knobXOn : control.knobXOff

            Behavior on x {
                NumberAnimation {
                    duration: control.transitionDuration
                    easing.type: Easing.OutCubic
                }
            }

            Canvas {
                id: knobCanvas
                anchors.fill: parent
                opacity: control.enabled ? 1.0 : 0.55
                antialiasing: true

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    const radius = Math.max(0, Math.min(width, height) * 0.5 - 0.75)
                    ctx.beginPath()
                    ctx.arc(width * 0.5, height * 0.5, radius, 0, Math.PI * 2, false)
                    ctx.fillStyle = control.knobFillColor
                    ctx.fill()
                }
            }
        }
    }

    onKnobFillColorChanged: knobCanvas.requestPaint()

    contentItem: Label {
        style: body
        text: control.text
        color: control.enabled ? Theme.textPrimary : Theme.textOctonary
        verticalAlignment: Text.AlignVCenter
        visible: control.text.length > 0
        elide: Text.ElideRight
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ToggleSwitch { checked: true }

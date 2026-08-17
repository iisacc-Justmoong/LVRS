import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Effects
import LVRS 1.0

Controls.Switch {
    id: control

    text: ""
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1

    property bool state: checked
    property int shapeStyle: shapeRoundRect
    property int trackWidth: Theme.toggleTrackWidth
    property int trackHeight: Theme.controlHeightSm
    property int trackPadding: Theme.gap2
    property int knobSize: Theme.controlIndicatorSize
    property int transitionDuration: Theme.toggleTransitionDuration
    property real trackCornerRadius: Theme.scaleRealMetric(20)
    property real knobCornerRadius: knobSize / 2

    property color onColor: Theme.accent
    property color offColor: Theme.panelBackground12
    property color onColorHover: Qt.darker(onColor, 1.08)
    property color onColorPressed: Qt.darker(onColor, 1.18)
    property color offColorHover: Qt.lighter(offColor, 1.08)
    property color offColorPressed: Qt.darker(offColor, 1.12)
    property color disabledTrackColor: Theme.surfaceAlt
    property bool trackShadowEnabled: true
    property color trackShadowColor: Theme.shadowStrong
    property real trackShadowOpacity: 1.0
    property real trackShadowBlur: Theme.scaleRealMetric(4)
    property real trackShadowHorizontalOffset: 0
    property real trackShadowVerticalOffset: Theme.scaleRealMetric(4)
    property color knobFillColor: Theme.textPrimary
    // Compatibility metrics retained for consumers of the previous Canvas-backed knob.
    readonly property real knobSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real knobHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property real knobRasterScale: Math.max(1.0, knobSupersampleScale * knobHiDpiScale)
    readonly property int knobXOff: trackPadding
    readonly property int knobXOn: Math.max(trackPadding, trackWidth - knobSize - trackPadding)
    readonly property color resolvedTrackColor: !control.enabled
        ? control.disabledTrackColor
        : control.checked
            ? (control.down ? control.onColorPressed
                            : (control.hovered ? control.onColorHover : control.onColor))
            : (control.down ? control.offColorPressed
                            : (control.hovered ? control.offColorHover : control.offColor))

    onStateChanged: {
        if (checked !== state)
            checked = state
    }

    onCheckedChanged: {
        if (state !== checked)
            state = checked
    }

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
        objectName: control.objectName.length > 0 ? control.objectName + "_indicator" : ""
        implicitWidth: control.trackWidth
        implicitHeight: control.trackHeight

        MultiEffect {
            id: trackShadowEffectItem
            objectName: control.objectName.length > 0 ? control.objectName + "_trackShadowEffect" : ""
            anchors.fill: track
            z: -1
            source: track
            visible: control.trackShadowEnabled
            autoPaddingEnabled: true
            blurMax: Math.max(1, Math.ceil(control.trackShadowBlur))
            shadowEnabled: control.trackShadowEnabled
            shadowOpacity: control.trackShadowOpacity
            shadowBlur: control.trackShadowBlur > 0 ? 1.0 : 0.0
            shadowHorizontalOffset: control.trackShadowHorizontalOffset
            shadowVerticalOffset: control.trackShadowVerticalOffset
            shadowColor: control.trackShadowColor
        }

        Rectangle {
            id: track
            objectName: control.objectName.length > 0 ? control.objectName + "_track" : ""
            anchors.fill: parent
            radius: control.resolvedTrackRadius(width, height)
            color: control.resolvedTrackColor
            antialiasing: true
        }

        Rectangle {
            id: knob
            objectName: control.objectName.length > 0 ? control.objectName + "_knob" : ""
            width: control.knobSize
            height: control.knobSize
            y: (track.height - height) / 2
            x: control.checked ? control.knobXOn : control.knobXOff
            radius: Math.max(0,
                             Math.min(control.knobCornerRadius,
                                      Math.min(width, height) * 0.5))
            color: control.knobFillColor
            opacity: control.enabled ? 1.0 : 0.55
            antialiasing: true

            Behavior on x {
                NumberAnimation {
                    duration: control.transitionDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    contentItem: Label {
        objectName: control.objectName.length > 0 ? control.objectName + "_label" : ""
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

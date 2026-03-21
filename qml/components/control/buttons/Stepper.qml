import QtQuick
import LVRS 1.0

Item {
    id: control

    enum StepperArrow {
        UpDown,
        Up,
        Down
    }

    property int tone: AbstractButton.Primary
    property int arrow: Stepper.UpDown
    property bool enabled: true
    property int cornerRadius: Theme.radiusSm
    property color textColorDisabled: Theme.textOctonary
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property real iconRasterScale: Math.max(1.0, iconSupersampleScale * iconHiDpiScale)
    readonly property real devicePixelRatio: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property bool usesPlatformSnapshotImage: true

    signal clicked()
    signal pressed()
    signal released()
    signal canceled()

    readonly property bool effectiveEnabled: enabled && tone !== AbstractButton.Disabled
    readonly property bool hovered: interactionArea.containsMouse && effectiveEnabled
    readonly property bool down: interactionArea.pressed && effectiveEnabled
    readonly property real figmaStepperSize: Theme.iconSm
    readonly property real figmaChevronWidth: Theme.scaleRealMetric(10)
    readonly property real figmaChevronHeight: Theme.scaleRealMetric(6)
    readonly property real figmaUpDownChevronWidth: Theme.scaleRealMetric(6.43604)
    readonly property real figmaUpDownChevronHeight: Theme.scaleRealMetric(11.1455)
    readonly property real iconWidth: control.arrow === Stepper.UpDown ? figmaUpDownChevronWidth : figmaChevronWidth
    readonly property real iconHeight: control.arrow === Stepper.UpDown ? figmaUpDownChevronHeight : figmaChevronHeight
    readonly property int iconSourceWidth: Math.max(1, Math.ceil(figmaStepperSize * iconRasterScale))
    readonly property int iconSourceHeight: Math.max(1, Math.ceil(figmaStepperSize * iconRasterScale))
    readonly property rect iconBounds: Qt.rect(control.snapToDevicePixel((control.width - control.iconWidth) * 0.5),
                                               control.snapToDevicePixel((control.height - control.iconHeight) * 0.5),
                                               control.iconWidth,
                                               control.iconHeight)
    readonly property color backgroundColor: control.tone === AbstractButton.Borderless ? "transparent" : Theme.primary
    readonly property color backgroundColorHover: control.tone === AbstractButton.Borderless
        ? Theme.surfaceAlt
        : Qt.darker(Theme.primary, 1.12)
    readonly property color backgroundColorPressed: control.tone === AbstractButton.Borderless
        ? Theme.accentBlueMuted
        : Qt.darker(Theme.primary, 1.2)
    readonly property color backgroundColorDisabled: Theme.panelBackground04
    readonly property color resolvedBackgroundColor: !control.effectiveEnabled
        ? control.backgroundColorDisabled
        : control.down
            ? control.backgroundColorPressed
            : control.hovered
                ? control.backgroundColorHover
                : control.backgroundColor
    readonly property color resolvedIconColor: !control.effectiveEnabled
        ? control.textColorDisabled
        : Theme.accentWhite
    readonly property color primaryStateOverlayColor: !control.effectiveEnabled
        ? Theme.overlayBackdrop
        : control.down
            ? Qt.rgba(0, 0, 0, 0.12)
            : control.hovered
                ? Qt.rgba(0, 0, 0, 0.06)
                : "transparent"
    readonly property real iconOpacity: control.effectiveEnabled ? 1.0 : 0.38
    readonly property string resolvedIconName: {
        const tonePrefix = control.tone === AbstractButton.Borderless ? "Borderless" : "Primary"
        if (control.arrow === Stepper.Up)
            return "StepperUp" + tonePrefix
        if (control.arrow === Stepper.Down)
            return "StepperDown" + tonePrefix
        return "StepperUpDown" + tonePrefix
    }
    readonly property url iconSource: Theme.iconPath(control.resolvedIconName)
    readonly property url renderedIconSource: RenderQuality.resolveTextureSource(control.iconSource)

    function snapToDevicePixel(value) {
        return Math.round(value * devicePixelRatio) / devicePixelRatio
    }

    implicitWidth: figmaStepperSize
    implicitHeight: figmaStepperSize
    width: figmaStepperSize
    height: figmaStepperSize
    clip: true

    Rectangle {
        anchors.fill: parent
        visible: control.tone === AbstractButton.Borderless
        radius: control.cornerRadius
        antialiasing: true
        color: control.resolvedBackgroundColor
    }

    Image {
        id: iconImage
        anchors.fill: parent
        objectName: control.objectName.length > 0 ? control.objectName + "_iconSnapshot" : ""
        source: control.renderedIconSource
        sourceSize.width: control.iconSourceWidth
        sourceSize.height: control.iconSourceHeight
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: RenderQuality.mipmapEnabled
        cache: true
        opacity: control.iconOpacity
    }

    Rectangle {
        anchors.fill: parent
        visible: control.tone !== AbstractButton.Borderless && control.primaryStateOverlayColor.a > 0
        radius: control.cornerRadius
        antialiasing: true
        color: control.primaryStateOverlayColor
    }

    MouseArea {
        id: interactionArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: control.effectiveEnabled
        preventStealing: true
        enabled: control.effectiveEnabled

        onPressed: control.pressed()
        onReleased: control.released()
        onCanceled: control.canceled()
        onClicked: control.clicked()
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Stepper { tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }

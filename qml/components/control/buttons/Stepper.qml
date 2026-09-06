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
    property alias method: methodRegistry.method
    property alias methods: methodRegistry.methods
    readonly property alias hasInjectedMethods: methodRegistry.hasInjectedMethods
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property real iconRasterScale: Math.max(1.0, iconSupersampleScale * iconHiDpiScale)
    readonly property real devicePixelRatio: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property bool usesPlatformSnapshotImage: true

    signal clicked()
    // UpDown resolves the clicked half; single-arrow variants have one direction.
    signal stepped(int direction)
    signal pressed()
    signal released()
    signal canceled()

    readonly property bool effectiveEnabled: enabled && tone !== AbstractButton.Disabled
    readonly property bool hovered: interactionArea.containsMouse && effectiveEnabled
    readonly property bool down: interactionArea.pressed && effectiveEnabled
    readonly property real figmaStepperSize: Theme.iconSm
    readonly property real figmaChevronWidth: figmaStepperSize * (10.0 / 18.0)
    readonly property real figmaChevronHeight: figmaStepperSize * (6.0 / 18.0)
    readonly property real figmaUpDownChevronWidth: figmaStepperSize * (6.43604 / 18.0)
    readonly property real figmaUpDownChevronHeight: figmaStepperSize * (11.1455 / 18.0)
    readonly property real iconWidth: control.arrow === Stepper.UpDown ? figmaUpDownChevronWidth : figmaChevronWidth
    readonly property real iconHeight: control.arrow === Stepper.UpDown ? figmaUpDownChevronHeight : figmaChevronHeight
    readonly property int iconSourceWidth: Math.max(1, Math.ceil(iconWidth * iconRasterScale))
    readonly property int iconSourceHeight: Math.max(1, Math.ceil(iconHeight * iconRasterScale))
    readonly property rect iconBounds: Qt.rect((control.width - control.iconWidth) * 0.5,
                                               (control.height - control.iconHeight) * 0.5,
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
    readonly property color renderedBackgroundColor: control.tone === AbstractButton.Borderless
        ? control.resolvedBackgroundColor
        : (!control.effectiveEnabled ? control.backgroundColorDisabled : control.backgroundColor)
    readonly property color resolvedIconColor: !control.effectiveEnabled
        ? control.textColorDisabled
        : Theme.accentWhite
    readonly property color primaryStateOverlayColor: !control.effectiveEnabled
        ? "transparent"
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
    readonly property string resolvedIconAssetName: control.arrow === Stepper.UpDown
        ? "StepperUpDownChevron"
        : "StepperChevron"
    readonly property real iconRotation: control.arrow === Stepper.Up ? 180 : 0
    readonly property url iconSource: Theme.iconPath(control.resolvedIconAssetName)
    readonly property url renderedIconSource: RenderQuality.resolveTextureSource(control.iconSource)

    function snapToDevicePixel(value) {
        return Math.round(value * devicePixelRatio) / devicePixelRatio
    }

    implicitWidth: figmaStepperSize
    implicitHeight: figmaStepperSize
    width: figmaStepperSize
    height: figmaStepperSize
    clip: true

    function createMethodEvent(triggerName) {
        return methodRegistry.createEvent(triggerName)
    }

    function invokeMethod(candidate, eventData) {
        return methodRegistry.invokeMethod(candidate, eventData)
    }

    function invokeMethods(eventData) {
        return methodRegistry.invokeMethods(eventData)
    }

    ButtonMethodRegistry {
        id: methodRegistry
        owner: control
        defaultTrigger: "clicked"
    }

    Connections {
        target: control
        function onClicked() {
            control.invokeMethods(control.createMethodEvent("clicked"))
        }
    }

    Rectangle {
        anchors.fill: parent
        objectName: control.objectName.length > 0 ? control.objectName + "_background" : ""
        radius: control.cornerRadius
        antialiasing: true
        color: control.renderedBackgroundColor
    }

    Image {
        id: iconImage
        x: control.iconBounds.x
        y: control.iconBounds.y
        width: control.iconBounds.width
        height: control.iconBounds.height
        objectName: control.objectName.length > 0 ? control.objectName + "_iconSnapshot" : ""
        source: control.renderedIconSource
        sourceSize.width: control.iconSourceWidth
        sourceSize.height: control.iconSourceHeight
        fillMode: Image.PreserveAspectFit
        rotation: control.iconRotation
        transformOrigin: Item.Center
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
        onClicked: function(mouse) {
            control.clicked()
            control.stepped(control.arrow === Stepper.Up ? 1
                : control.arrow === Stepper.Down ? -1
                : mouse.y < height / 2 ? 1 : -1)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Stepper { tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown; method: function(eventData) { ... } }

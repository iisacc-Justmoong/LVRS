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
    readonly property string snapshotProfile: Theme.mobileTarget ? "mobile" : "desktop"
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
    readonly property int iconSourceWidth: Math.max(1, Math.ceil(iconWidth * iconRasterScale))
    readonly property int iconSourceHeight: Math.max(1, Math.ceil(iconHeight * iconRasterScale))
    readonly property rect iconBounds: Qt.rect(iconFrame.x, iconFrame.y, iconFrame.width, iconFrame.height)
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
        : control.tone === AbstractButton.Borderless
            ? Theme.primary
            : Theme.accentWhite
    readonly property url iconSnapshotSource: "data:image/svg+xml;utf8," + encodeURIComponent(iconSnapshotSvgMarkup)
    readonly property string iconSnapshotSvgMarkup: control.buildSnapshotSvgMarkup()

    function snapToDevicePixel(value) {
        return Math.round(value * devicePixelRatio) / devicePixelRatio
    }

    function svgColor(value) {
        return "rgba(" + Math.round(value.r * 255)
            + "," + Math.round(value.g * 255)
            + "," + Math.round(value.b * 255)
            + "," + value.a.toFixed(3) + ")"
    }

    function svgRoot(width, height, body) {
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + width + "\" height=\"" + height
            + "\" viewBox=\"0 0 " + width + " " + height + "\" fill=\"none\">" + body + "</svg>"
    }

    function chevronStrokeWidth(width, height) {
        return Math.max(1.0, Math.min(width, height) * 0.18)
    }

    function buildChevronSnapshotBody(width, height, direction) {
        const strokeWidth = chevronStrokeWidth(width, height)
        const insetX = strokeWidth * 0.55
        const tipInsetY = strokeWidth * 0.6
        const leftX = insetX
        const rightX = width - insetX
        const midX = width * 0.5
        const topY = tipInsetY
        const bottomY = height - tipInsetY
        const pathData = direction === "up"
            ? "M " + leftX + " " + bottomY + " L " + midX + " " + topY + " L " + rightX + " " + bottomY
            : "M " + leftX + " " + topY + " L " + midX + " " + bottomY + " L " + rightX + " " + topY
        return "<path d=\"" + pathData + "\" stroke=\"" + svgColor(control.resolvedIconColor)
            + "\" stroke-width=\"" + strokeWidth + "\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    }

    function buildUpDownSnapshotBody(width, height) {
        const strokeWidth = chevronStrokeWidth(width, height)
        const insetX = strokeWidth * 0.6
        const leftX = insetX
        const rightX = width - insetX
        const midX = width * 0.5
        const topTipY = strokeWidth * 0.55
        const upperBaseY = height * 0.38
        const lowerBaseY = height * 0.62
        const bottomTipY = height - strokeWidth * 0.55
        const upperPath = "M " + leftX + " " + upperBaseY + " L " + midX + " " + topTipY + " L " + rightX + " " + upperBaseY
        const lowerPath = "M " + leftX + " " + lowerBaseY + " L " + midX + " " + bottomTipY + " L " + rightX + " " + lowerBaseY
        const stroke = " stroke=\"" + svgColor(control.resolvedIconColor)
            + "\" stroke-width=\"" + strokeWidth + "\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
        return "<path d=\"" + upperPath + "\"" + stroke + "<path d=\"" + lowerPath + "\"" + stroke
    }

    function buildSnapshotSvgMarkup() {
        if (control.arrow === Stepper.UpDown)
            return svgRoot(control.iconWidth, control.iconHeight,
                           buildUpDownSnapshotBody(control.iconWidth, control.iconHeight))
        if (control.arrow === Stepper.Up)
            return svgRoot(control.iconWidth, control.iconHeight,
                           buildChevronSnapshotBody(control.iconWidth, control.iconHeight, "up"))
        return svgRoot(control.iconWidth, control.iconHeight,
                       buildChevronSnapshotBody(control.iconWidth, control.iconHeight, "down"))
    }

    implicitWidth: figmaStepperSize
    implicitHeight: figmaStepperSize
    width: figmaStepperSize
    height: figmaStepperSize
    clip: true

    Rectangle {
        anchors.fill: parent
        radius: control.cornerRadius
        antialiasing: true
        color: control.resolvedBackgroundColor
    }

    Item {
        id: iconFrame
        x: control.snapToDevicePixel((control.width - width) * 0.5)
        y: control.snapToDevicePixel((control.height - height) * 0.5)
        width: control.iconWidth
        height: control.iconHeight
        implicitWidth: width
        implicitHeight: height

        Image {
            id: iconImage
            anchors.fill: parent
            objectName: control.objectName.length > 0 ? control.objectName + "_iconSnapshot" : ""
            source: control.iconSnapshotSource
            sourceSize.width: control.iconSourceWidth
            sourceSize.height: control.iconSourceHeight
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: RenderQuality.mipmapEnabled
            cache: true
        }
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

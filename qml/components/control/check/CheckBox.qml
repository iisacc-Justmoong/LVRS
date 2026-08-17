import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    checkable: true
    tone: AbstractButton.Borderless

    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int boxSize: Theme.scaleMetric(17)
    property real framePadding: Theme.scaleRealMetric(0.5)
    property real boxRadius: boxSize * (3.5 / 17.0)
    property bool useFigmaCheckedAssets: true
    property url checkedAssetSourceEnabled: Theme.iconPath("checkboxCheckedEnabled")
    property url checkedAssetSourceDisabled: Theme.iconPath("checkboxCheckedDisabled")
    readonly property url resolvedCheckedAssetSource: control.enabled
        ? control.checkedAssetSourceEnabled
        : control.checkedAssetSourceDisabled
    readonly property bool usingFigmaCheckedAsset: control.checked
        && control.useFigmaCheckedAssets
        && control.shapeStyle === control.shapeRoundRect
    property color checkColor: Theme.bodyColor
    property color checkedColor: Theme.accent
    property color uncheckedColor: Theme.bodyColor
    property color disabledCheckedColor: Theme.panelBackground12
    property color disabledUncheckedColor: Theme.panelBackground12
    property color checkMarkColorDisabled: Theme.disabledColor
    property real checkMarkStrokeWidth: boxSize * (2.0 / 17.0)
    property real boxBorderWidthCheckedEnabled: 0
    property real boxBorderWidthCheckedDisabled: boxSize * (0.5 / 17.0)
    property real boxBorderWidthUncheckedEnabled: boxSize * (0.5 / 17.0)
    property real boxBorderWidthUncheckedDisabled: 0
    property color boxBorderColorCheckedEnabled: "transparent"
    property color boxBorderColorCheckedDisabled: Theme.panelBackground12
    property color boxBorderColorUncheckedEnabled: Theme.bodyColor
    property color boxBorderColorUncheckedDisabled: "transparent"
    property color innerShadowSoftColor: "#14000000"
    property color innerShadowStrongColor: "#1A000000"
    readonly property real checkmarkSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real checkmarkHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property real checkmarkRasterScale: Math.max(1.0, checkmarkSupersampleScale * checkmarkHiDpiScale)

    readonly property bool showInnerShadow: !(control.checked && control.enabled)
    readonly property color resolvedCheckedFillColor: !control.enabled
        ? control.disabledCheckedColor
        : control.down
            ? Qt.darker(control.checkedColor, 1.2)
            : control.hovered
                ? Qt.darker(control.checkedColor, 1.08)
                : control.checkedColor
    readonly property color resolvedUncheckedFillColor: !control.enabled
        ? control.disabledUncheckedColor
        : control.down
            ? Qt.darker(control.uncheckedColor, 1.18)
            : control.hovered
                ? Qt.darker(control.uncheckedColor, 1.06)
                : control.uncheckedColor
    readonly property real resolvedBoxRadius: shapeStyle === shapeCylinder
        ? Math.max(0, boxSize / 2)
        : boxRadius
    readonly property real resolvedBoxBorderWidth: control.checked
        ? (control.enabled ? control.boxBorderWidthCheckedEnabled : control.boxBorderWidthCheckedDisabled)
        : (control.enabled ? control.boxBorderWidthUncheckedEnabled : control.boxBorderWidthUncheckedDisabled)
    readonly property color resolvedBoxBorderColor: control.checked
        ? (control.enabled ? control.boxBorderColorCheckedEnabled : control.boxBorderColorCheckedDisabled)
        : (control.enabled ? control.boxBorderColorUncheckedEnabled : control.boxBorderColorUncheckedDisabled)

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.gapNone
    backgroundColor: "transparent"
    backgroundColorHover: "transparent"
    backgroundColorPressed: "transparent"
    backgroundColorDisabled: "transparent"
    implicitHeight: contentItem.implicitHeight
    implicitWidth: contentItem.implicitWidth

    background: Item { }

    contentItem: Item {
        id: contentLayout

        property real spacing: control.text.length > 0 ? Theme.gap6 : Theme.gapNone
        readonly property real labelWidth: label.visible ? Math.ceil(label.implicitWidth) : 0

        implicitWidth: control.framePadding
            + control.boxSize
            + (label.visible ? spacing + labelWidth : 0)
            + control.framePadding
        implicitHeight: control.boxSize + control.framePadding * 2

        Rectangle {
            id: indicator
            objectName: control.objectName.length > 0 ? control.objectName + "_indicator" : ""
            x: control.framePadding
            y: control.framePadding
            width: control.boxSize
            height: control.boxSize
            implicitWidth: control.boxSize
            implicitHeight: control.boxSize
            radius: control.resolvedBoxRadius
            color: control.checked ? control.resolvedCheckedFillColor : control.resolvedUncheckedFillColor
            border.width: control.resolvedBoxBorderWidth
            border.color: control.resolvedBoxBorderColor
            antialiasing: true

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.width: control.boxSize * (1.0 / 17.0)
                border.color: control.innerShadowSoftColor
                visible: control.showInnerShadow
                antialiasing: true
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: parent.height * 0.45
                radius: parent.radius
                color: control.innerShadowStrongColor
                opacity: control.showInnerShadow ? 0.5 : 0
                antialiasing: true
                clip: true
            }

            Image {
                id: checkedAsset
                objectName: control.objectName.length > 0 ? control.objectName + "_checkedAsset" : ""
                anchors.fill: parent
                visible: control.usingFigmaCheckedAsset
                source: RenderQuality.resolveTextureSource(control.resolvedCheckedAssetSource)
                sourceSize.width: Math.max(1, Math.ceil(control.boxSize * control.checkmarkRasterScale))
                sourceSize.height: Math.max(1, Math.ceil(control.boxSize * control.checkmarkRasterScale))
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: RenderQuality.mipmapEnabled
            }

            Canvas {
                id: checkmarkCanvas
                anchors.fill: parent
                visible: control.checked && !control.usingFigmaCheckedAsset
                objectName: control.objectName.length > 0 ? control.objectName + "_checkmarkCanvas" : ""
                antialiasing: true
                canvasSize: Qt.size(Math.max(1, Math.ceil(width * control.checkmarkRasterScale)),
                                    Math.max(1, Math.ceil(height * control.checkmarkRasterScale)))
                readonly property real rasterScaleX: width > 0 ? canvasSize.width / width : 1.0
                readonly property real rasterScaleY: height > 0 ? canvasSize.height / height : 1.0

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, canvasSize.width, canvasSize.height)
                    if (!control.checked)
                        return

                    ctx.save()
                    ctx.scale(checkmarkCanvas.rasterScaleX, checkmarkCanvas.rasterScaleY)
                    ctx.beginPath()
                    ctx.moveTo(width * 0.25, height * 0.52)
                    ctx.lineTo(width * 0.43, height * 0.70)
                    ctx.lineTo(width * 0.69, height * 0.34)
                    ctx.lineWidth = control.checkMarkStrokeWidth
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.strokeStyle = control.enabled ? control.checkColor : control.checkMarkColorDisabled
                    ctx.stroke()
                    ctx.restore()
                }

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onCanvasSizeChanged: requestPaint()
            }
        }

        Label {
            id: label
            objectName: control.objectName.length > 0 ? control.objectName + "_label" : ""
            x: indicator.x + indicator.width + contentLayout.spacing
            y: (contentLayout.height - height) / 2
            width: visible ? contentLayout.labelWidth : 0
            height: Theme.textBodyLineHeight
            style: body
            text: control.text
            color: control.enabled ? Theme.bodyColor : Theme.disabledColor
            visible: control.text.length > 0
            verticalAlignment: Text.AlignVCenter
        }
    }

    onCheckedChanged: checkmarkCanvas.requestPaint()
    onEnabledChanged: checkmarkCanvas.requestPaint()
    onCheckColorChanged: checkmarkCanvas.requestPaint()
    onCheckMarkColorDisabledChanged: checkmarkCanvas.requestPaint()
    onCheckMarkStrokeWidthChanged: checkmarkCanvas.requestPaint()
    onCheckmarkRasterScaleChanged: checkmarkCanvas.requestPaint()

    Component.onCompleted: checkmarkCanvas.requestPaint()

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.CheckBox { text: "Remember me"; checked: true }

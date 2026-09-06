pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Effects
import LVRS 1.0

AbstractInputBar {
    id: control

    readonly property int defaultMode: 0
    readonly property int searchMode: 1
    readonly property int roundedStyle: 0
    readonly property int filledStyle: roundedStyle
    readonly property int inlineStyle: 1
    readonly property int cylinderStyle: 2

    property int mode: defaultMode
    property bool search: mode === searchMode
    property int style: filledStyle
    property alias placeholder: control.placeholderText
    property bool clearButtonVisible: true
    readonly property bool searchIconVisible: search || mode === searchMode
    property color searchIconColor: Theme.accentGrayLight
    // Deprecated compatibility knob kept for older callers after the canvas icon removal.
    property real searchIconStrokeWidth: Theme.scaleRealMetric(1.5)
    property color clearIconBackgroundColor: Theme.descriptionColor
    property color clearIconBackgroundColorHover: Qt.lighter(clearIconBackgroundColor, 1.08)
    property color clearIconBackgroundColorPressed: Qt.darker(clearIconBackgroundColor, 1.14)
    property color clearIconBackgroundColorDisabled: Theme.disabledColor
    property color clearIconForegroundColor: Theme.panelBackground10
    readonly property real searchIconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real searchIconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property real searchIconRasterScale: Math.max(1.0, searchIconSupersampleScale * searchIconHiDpiScale)
    readonly property int searchIconSize: Theme.scaleMetric(12)
    readonly property url searchIconSource: Theme.iconPath("inputFieldSearch")
    readonly property url renderedSearchIconSource: RenderQuality.resolveTextureSource(searchIconSource)
    readonly property int searchIconSourceSize: Math.max(1, Math.ceil(searchIconSize * searchIconRasterScale))
    readonly property int clearIconSize: Theme.scaleMetric(12)
    readonly property real clearIconMarkLength: clearIconSize * (8.0 / 14.0)
    readonly property real clearIconMarkThickness: clearIconSize * (1.4 / 14.0)

    readonly property int resolvedStyle: style === inlineStyle
        ? inlineStyle
        : style === cylinderStyle
            ? cylinderStyle
            : roundedStyle
    readonly property color frameFillColor: resolvedStyle === inlineStyle
        ? Theme.inputFieldGlassTintInline
        : Theme.inputFieldGlassTint
    readonly property color frameFillColorHover: frameFillColor
    readonly property color frameFillColorPressed: frameFillColor
    readonly property color frameFillColorDisabled: resolvedStyle === inlineStyle
        ? Theme.inputFieldGlassTintInline
        : Theme.inputFieldGlassTintDisabled
    property bool glassEnabled: true
    property real glassBlurRadius: Theme.scaleRealMetric(resolvedStyle === inlineStyle ? 6 : 8)
    // The window background excludes its content. Custom surfaces may provide a sibling.
    property Item backdropSource: Controls.ApplicationWindow.window
        ? Controls.ApplicationWindow.window.background : null
    readonly property Item resolvedBackdropSource: {
        const source = control.backdropSource
        if (!source)
            return null
        // Neither the input nor any part of it may enter its own capture.
        for (let ancestor = control; ancestor; ancestor = ancestor.parent) {
            if (ancestor === source)
                return null
        }
        for (let ancestor = source; ancestor; ancestor = ancestor.parent) {
            if (ancestor === control)
                return null
        }
        return source
    }
    readonly property bool glassActive: visible && enabled && glassEnabled
        && glassBlurRadius > 0 && resolvedBackdropSource !== null
    readonly property bool showClearButton: clearButtonVisible
        && enabled
        && !readOnly
        && text.length > 0

    implicitWidth: Theme.inputWidthMd
    fieldMinHeight: Theme.scaleMetric(19)
    insetHorizontal: Theme.gap7
    // Keep the clear affordance 8 logical pixels from the edge on every target.
    insetRight: showClearButton ? 8 : insetHorizontal
    insetVertical: Theme.gap3
    sideSpacing: Theme.gap2
    centeredTextHeight: Theme.textBodyLineHeight
    shapeStyle: resolvedStyle === roundedStyle ? shapeRoundRect : shapeCylinder
    cornerRadius: Theme.radiusControl

    textColor: Theme.titleHeaderColor
    textColorDisabled: Theme.disabledColor
    placeholderColor: Theme.titleHeaderColor
    placeholderColorDisabled: Theme.disabledColor
    placeholderOpacity: 1.0

    backgroundColor: frameFillColor
    backgroundColorHover: frameFillColorHover
    backgroundColorPressed: frameFillColorPressed
    backgroundColorFocused: frameFillColor
    backgroundColorDisabled: frameFillColorDisabled

    backgroundComponent: MaterialSurface {
        tintColor: control.resolvedBackgroundColor
        cornerRadius: control.resolvedCornerRadius
        inlineStyle: control.resolvedStyle === control.inlineStyle
        recessed: control.enabled
        backdropSource: control.resolvedBackdropSource
        blurRadius: control.glassBlurRadius
        blurActive: control.glassActive
    }

    selectionColor: Theme.accent
    selectedTextColor: Theme.textPrimary

    component MaterialSurface: Item {
        id: surface

        required property color tintColor
        property real cornerRadius: 0
        property bool inlineStyle: false
        property bool recessed: true
        property Item backdropSource: null
        property real blurRadius: 8
        property bool blurActive: false
        readonly property real effectScale: Theme.metricScaleFactor
        readonly property real capturePadding: Math.max(0, Math.min(64, blurRadius))
        // Canvas.Image already accounts for the window DPR; this adds only LVRS supersampling.
        readonly property real rasterScale: Math.max(1,
            RenderQuality.enabled ? RenderQuality.effectiveSupersampleScaleValue : 1)
        readonly property real gradientTopAlpha: inlineStyle ? 0.06 : recessed ? 0.20 : 0.045
        readonly property real gradientBottomAlpha: inlineStyle || !recessed ? 0.015 : 0.035

        // Explicitly read ancestor transforms so scrolling/repositioning invalidates the crop.
        function mappedCaptureRect(): rect {
            const source = surface.backdropSource
            if (!source)
                return Qt.rect(0, 0, glassCapture.width, glassCapture.height)
            for (const item of [surface, source]) {
                for (let ancestor = item; ancestor; ancestor = ancestor.parent) {
                    const transform = [ancestor.x, ancestor.y, ancestor.width, ancestor.height,
                                       ancestor.scale, ancestor.rotation, ancestor.transformOrigin]
                }
            }
            const padding = surface.capturePadding
            return surface.mapToItem(source, -padding, -padding,
                                     surface.width + padding * 2, surface.height + padding * 2)
        }

        ShaderEffectSource {
            id: glassCapture
            objectName: "inputFieldGlassCapture"
            width: surface.width + surface.capturePadding * 2
            height: surface.height + surface.capturePadding * 2
            sourceItem: surface.blurActive ? surface.backdropSource : null
            sourceRect: surface.mappedCaptureRect()
            live: surface.blurActive
            hideSource: false
            recursive: false
            visible: false
        }

        Item {
            id: glassMask
            width: glassCapture.width
            height: glassCapture.height
            visible: false
            layer.enabled: surface.blurActive

            Rectangle {
                x: surface.capturePadding
                y: surface.capturePadding
                width: surface.width
                height: surface.height
                radius: surface.cornerRadius
                color: "white"
                antialiasing: true
            }
        }

        MultiEffect {
            objectName: "inputFieldGlassEffect"
            x: -surface.capturePadding
            y: -surface.capturePadding
            width: glassCapture.width
            height: glassCapture.height
            source: glassCapture
            visible: surface.blurActive
            blurEnabled: true
            blurMax: Math.max(2, Math.min(64, Math.ceil(surface.blurRadius)))
            blur: 1
            autoPaddingEnabled: false
            maskEnabled: true
            maskSource: glassMask
        }

        // A cached canvas keeps the subpixel inset strokes/shadows consistent on both
        // RHI and software renderers. It repaints only when appearance or size changes.
        Canvas {
            id: material
            objectName: "inputFieldMaterial"
            width: surface.width * surface.rasterScale
            height: surface.height * surface.rasterScale
            scale: 1 / surface.rasterScale
            transformOrigin: Item.TopLeft
            antialiasing: true
            renderTarget: Canvas.Image
            canvasSize: Qt.size(Math.max(1, Math.ceil(width)), Math.max(1, Math.ceil(height)))

            function innerShadow(ctx, w, h, radius, color, dx, dy, blur) {
                // Shadow of the area outside the rounded field, clipped to its inside.
                ctx.save()
                ctx.beginPath()
                ctx.roundedRect(0, 0, w, h, radius, radius)
                ctx.clip()
                ctx.shadowColor = color
                ctx.shadowOffsetX = dx
                ctx.shadowOffsetY = dy
                ctx.shadowBlur = blur
                ctx.fillStyle = "black"
                ctx.fillRule = Qt.OddEvenFill
                const pad = Math.ceil(blur * 2 + Math.max(Math.abs(dx), Math.abs(dy)) + 2)
                ctx.beginPath()
                ctx.rect(-pad, -pad, w + pad * 2, h + pad * 2)
                ctx.roundedRect(0, 0, w, h, radius, radius)
                ctx.fill()
                ctx.restore()
            }

            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                const w = canvasSize.width
                const h = canvasSize.height
                const scale = surface.rasterScale
                const metric = surface.effectScale * scale
                const radius = Math.min(w / 2, h / 2, surface.cornerRadius * scale)
                ctx.clearRect(0, 0, w, h)
                ctx.beginPath()
                ctx.roundedRect(0, 0, w, h, radius, radius)
                // Preserve the Figma paint stack: concave gradient, reflection, tint.
                const fill = ctx.createLinearGradient(0, 0, 0, h)
                fill.addColorStop(0, Qt.rgba(0, 0, 0, surface.gradientTopAlpha))
                fill.addColorStop(0.52, Qt.rgba(0, 0, 0, 0))
                fill.addColorStop(1, Qt.rgba(1, 1, 1, surface.gradientBottomAlpha))
                ctx.fillStyle = fill
                ctx.fill()
                ctx.fillStyle = Theme.inputFieldGlassReflection
                ctx.fill()
                ctx.fillStyle = surface.tintColor
                ctx.fill()

                if (surface.recessed) {
                    material.innerShadow(ctx, w, h, radius,
                                         Qt.rgba(0, 0, 0, surface.inlineStyle ? 0.12 : 0.30),
                                         (surface.inlineStyle ? 0 : 0.5) * metric,
                                         (surface.inlineStyle ? 0.65 : 1.2) * metric,
                                         (surface.inlineStyle ? 1 : 2) * metric)
                    material.innerShadow(ctx, w, h, radius,
                                         Qt.rgba(1, 1, 1, surface.inlineStyle ? 0.055 : 0.16),
                                         0, -0.5 * metric, (surface.inlineStyle ? 0.6 : 0.8) * metric)
                    if (!surface.inlineStyle) {
                        const stroke = 0.5 * metric
                        const edge = ctx.createLinearGradient(0, 0, 0, h)
                        edge.addColorStop(0, Qt.rgba(0, 0, 0, 0.28))
                        edge.addColorStop(0.5, Qt.rgba(1, 1, 1, 0.015))
                        edge.addColorStop(1, Qt.rgba(1, 1, 1, 0.17))
                        ctx.beginPath()
                        ctx.roundedRect(stroke / 2, stroke / 2, w - stroke, h - stroke,
                                        Math.max(0, radius - stroke / 2), Math.max(0, radius - stroke / 2))
                        ctx.lineWidth = stroke
                        ctx.strokeStyle = edge
                        ctx.stroke()
                    }
                }
            }

            onCanvasSizeChanged: requestPaint()
            onVisibleChanged: if (visible) requestPaint()
        }

        onTintColorChanged: material.requestPaint()
        onCornerRadiusChanged: material.requestPaint()
        onInlineStyleChanged: material.requestPaint()
        onRecessedChanged: material.requestPaint()
        onEffectScaleChanged: material.requestPaint()
        onRasterScaleChanged: material.requestPaint()
        Component.onCompleted: material.requestPaint()
    }

    leadingInternalItems: Item {
        id: searchIconHost
        width: control.searchIconVisible ? control.searchIconSize : 0
        height: control.searchIconSize
        visible: width > 0

        Image {
            id: searchIcon
            anchors.fill: parent
            objectName: control.objectName.length > 0 ? control.objectName + "_searchIconImage" : ""
            source: control.renderedSearchIconSource
            sourceSize.width: control.searchIconSourceSize
            sourceSize.height: control.searchIconSourceSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: RenderQuality.mipmapEnabled
            cache: true
        }
    }

    trailingInternalItems: Item {
        id: clearButton
        objectName: control.objectName.length > 0 ? control.objectName + "_clearButton" : ""
        width: control.showClearButton ? control.clearIconSize : 0
        height: control.clearIconSize
        visible: width > 0
        readonly property bool hovered: clearInteractionArea.containsMouse && clearInteractionArea.enabled
        readonly property bool pressed: clearInteractionArea.pressed && clearInteractionArea.enabled
        readonly property color backgroundColor: !control.enabled
            ? control.clearIconBackgroundColorDisabled
            : clearButton.pressed
                ? control.clearIconBackgroundColorPressed
                : clearButton.hovered
                    ? control.clearIconBackgroundColorHover
                    : control.clearIconBackgroundColor

        Rectangle {
            id: clearIconBubble
            anchors.centerIn: parent
            width: control.clearIconSize
            height: control.clearIconSize
            radius: control.clearIconSize * 0.5
            color: clearButton.backgroundColor
            antialiasing: true

            Rectangle {
                width: control.clearIconMarkLength
                height: control.clearIconMarkThickness
                radius: control.clearIconMarkThickness * 0.5
                color: control.clearIconForegroundColor
                anchors.centerIn: parent
                rotation: 45
                antialiasing: true
            }

            Rectangle {
                width: control.clearIconMarkLength
                height: control.clearIconMarkThickness
                radius: control.clearIconMarkThickness * 0.5
                color: control.clearIconForegroundColor
                anchors.centerIn: parent
                rotation: -45
                antialiasing: true
            }
        }

        MouseArea {
            id: clearInteractionArea
            anchors.fill: parent
            enabled: control.showClearButton
            acceptedButtons: Qt.LeftButton
            hoverEnabled: enabled
            onClicked: {
                control.text = ""
                control.forceInputFocus()
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.InputField { placeholderText: "Search"; search: true; style: cylinderStyle }

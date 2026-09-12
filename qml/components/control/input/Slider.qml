pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import LVRS 1.0

T.Slider {
    id: control
    property bool motionEnabled: true
    property real displayedPosition: visualPosition
    SpringBehavior on displayedPosition {
        motionEnabled: control.motionEnabled && control.enabled && !control.pressed
    }

    enum SliderType { Default, CenterBiased, Ticks, CenterBiasedTicks, Filled, MinMaxLabels, Segmented }
    enum SliderSize { Mini, Small, Regular, Large }
    enum DisplayState { Automatic, IdleState, HoverState, PressedState, DisabledState }

    property int type: Slider.Default
    property int size: Slider.Regular
    property int displayState: Slider.Automatic
    property bool active: true
    property int segmentCount: 5
    property int tickCount: 11
    property bool showTicks: type === Slider.Ticks || type === Slider.CenterBiasedTicks || segmented
    property bool showThumb: !filled || segmented
    property bool showFocusRing: false
    property bool showMinMax: type === Slider.MinMaxLabels || segmented
    property bool showLabels: true
    property bool showEndpointIcons: false
    property string minimumLabel: "0%"
    property string maximumLabel: "100%"
    property string minimumIconName: "sun"
    property string maximumIconName: "sun"
    property url minimumIconSource: ""
    property url maximumIconSource: ""
    property bool showSymbol: true
    property string symbolName: "sun"
    property url symbolSource: ""

    property color trackColor: Theme.panelBackground12
    property color pressedTrackColor: Theme.surfaceSolid
    property color fillColor: Theme.accent
    property color inactiveFillColor: Theme.disabledColor
    property color thumbColor: Theme.titleHeaderColor
    property color labelColor: Theme.bodyColor
    property color tickColor: Theme.captionColor

    readonly property bool filled: type === Slider.Filled || type === Slider.MinMaxLabels || segmented
    readonly property bool segmented: type === Slider.Segmented
    readonly property bool centerBiased: type === Slider.CenterBiased || type === Slider.CenterBiasedTicks
    readonly property int resolvedSize: Math.max(Slider.Mini, Math.min(Slider.Large, size))
    readonly property int resolvedSegmentCount: Math.max(2, segmentCount)
    readonly property int resolvedTickCount: segmented ? resolvedSegmentCount : Math.max(2, tickCount)
    readonly property real controlHeight: Theme.scaleRealMetric([22, 24, 32, 44][resolvedSize])
    readonly property real trackHeight: Theme.scaleRealMetric((filled ? [12, 16, 28, 44] : [4, 6, 6, 8])[resolvedSize])
    readonly property real thumbDiameter: filled ? Math.max(Theme.gap8, trackHeight - Theme.gap8)
                                               : Theme.scaleRealMetric([12, 14, 18, 22][resolvedSize])
    readonly property real rangeInset: filled ? trackHeight / 2 : Math.max(Theme.gap8, thumbDiameter / 2)
    readonly property real rangeWidth: Math.max(0, availableWidth - rangeInset * 2)
    readonly property bool effectivePressed: enabled && (displayState === Slider.Automatic ? pressed : displayState === Slider.PressedState)
    readonly property bool effectiveHovered: enabled && (displayState === Slider.Automatic ? hovered : displayState === Slider.HoverState)
    readonly property bool effectiveFocusRing: enabled && (showFocusRing || visualFocus || effectiveHovered || effectivePressed)
    readonly property bool effectiveThumbVisible: showThumb || effectiveHovered || effectivePressed || (enabled && visualFocus)
    readonly property real focusRingWidth: effectivePressed || visualFocus ? Theme.gap2 : Theme.scaleRealMetric(1)
    readonly property color resolvedFillColor: active ? fillColor : inactiveFillColor
    readonly property real fillStart: centerBiased ? Math.min(0.5, visualPosition) : mirrored ? visualPosition : 0
    readonly property real fillLength: centerBiased ? Math.abs(position - 0.5) : position
    readonly property real endpointMaxWidth: Math.max(0, (width - Math.max(Theme.scaleMetric(40), rangeInset * 2) - spacing * 2) / 2)

    from: 0
    to: 1
    value: 0.5
    stepSize: segmented ? Math.abs(to - from) / (resolvedSegmentCount - 1) : 0
    snapMode: segmented ? T.Slider.SnapAlways : T.Slider.NoSnap
    orientation: Qt.Horizontal
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true
    enabled: displayState !== Slider.DisabledState
    opacity: enabled ? 1 : 0.32
    // Figma applies disabled opacity to the composed control, not to each overlap.
    layer.enabled: !enabled
    spacing: Theme.gap8
    leftPadding: mirrored ? maximumEndpoint.extent : minimumEndpoint.extent
    rightPadding: mirrored ? minimumEndpoint.extent : maximumEndpoint.extent
    topPadding: 0
    bottomPadding: 0
    implicitWidth: Theme.scaleMetric(320)
    implicitHeight: controlHeight

    // Qt handles arrows, drag, touch, snapping and accessibility. Add endpoint keys.
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Home || event.key === Qt.Key_End) {
            const next = event.key === Qt.Key_Home ? from : to
            if (value !== next) {
                value = next
                moved()
            }
            event.accepted = true
        }
    }

    component Endpoint: Row {
        id: endpoint
        required property string label
        required property url iconUrl
        readonly property bool hasIcon: control.showEndpointIcons && iconUrl.toString().length > 0
        readonly property bool hasLabel: control.showLabels && label.length > 0
        readonly property bool shown: control.showMinMax && (hasIcon || hasLabel)
        readonly property real extent: shown ? width + control.spacing : 0
        visible: shown
        spacing: Theme.gap4
        width: (hasIcon ? Theme.iconSm : 0) + (hasLabel ? labelItem.width : 0)
               + (hasIcon && hasLabel ? spacing : 0)
        layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
        height: Math.max(icon.height, labelItem.height)

        Image {
            id: icon
            visible: endpoint.hasIcon
            width: Theme.iconSm
            height: Theme.iconSm
            y: (endpoint.height - height) / 2
            source: endpoint.iconUrl
            sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
        }
        Label {
            id: labelItem
            visible: endpoint.hasLabel
            style: body
            text: endpoint.label
            width: Math.min(implicitWidth, Math.max(0, control.endpointMaxWidth - (endpoint.hasIcon ? Theme.iconSm + endpoint.spacing : 0)))
            y: (endpoint.height - height) / 2
            color: control.labelColor
            elide: Text.ElideRight
        }
    }

    // Cached material matches InputField's inner-shadow technique on software and RHI.
    component Material: Canvas {
        id: material
        property color fillColor
        property bool thumb: false
        readonly property real inset: thumb ? Theme.scaleRealMetric(4) : 0
        antialiasing: true
        renderTarget: Canvas.Image
        canvasSize: Qt.size(Math.max(1, Math.ceil(width)), Math.max(1, Math.ceil(height)))

        function innerShadow(ctx, w, h, color, offset, blur) {
            ctx.save()
            ctx.beginPath()
            ctx.roundedRect(0, 0, w, h, h / 2, h / 2)
            ctx.clip()
            ctx.shadowColor = color
            ctx.shadowOffsetY = offset
            ctx.shadowBlur = blur
            ctx.fillStyle = "black"
            ctx.fillRule = Qt.OddEvenFill
            ctx.beginPath()
            ctx.rect(-8, -8, w + 16, h + 16)
            ctx.roundedRect(0, 0, w, h, h / 2, h / 2)
            ctx.fill()
            ctx.restore()
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            const w = Math.max(0, width - inset * 2)
            const h = Math.max(0, height - inset * 2)
            if (w <= 0 || h <= 0)
                return
            ctx.translate(inset, inset)
            ctx.save()
            if (thumb) {
                ctx.shadowColor = Qt.rgba(0, 0, 0, 0.3)
                ctx.shadowOffsetY = Theme.scaleRealMetric(1)
                ctx.shadowBlur = Theme.scaleRealMetric(3)
            }
            ctx.beginPath()
            ctx.roundedRect(0, 0, w, h, h / 2, h / 2)
            ctx.fillStyle = fillColor
            ctx.fill()
            ctx.restore()
            if (thumb) {
                innerShadow(ctx, w, h, Qt.rgba(1, 1, 1, 0.15), Theme.scaleRealMetric(1), 0)
            } else {
                innerShadow(ctx, w, h, Qt.rgba(0, 0, 0, 0.12), Theme.scaleRealMetric(0.65), Theme.scaleRealMetric(1))
                innerShadow(ctx, w, h, Qt.rgba(1, 1, 1, 0.055), Theme.scaleRealMetric(-0.5), Theme.scaleRealMetric(0.6))
            }
        }
        onCanvasSizeChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onFillColorChanged: requestPaint()
        onThumbChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
    }

    background: Item {
        x: 0
        y: 0
        width: control.width
        height: control.height

        Endpoint {
            id: minimumEndpoint
            objectName: "slider_minimum"
            label: control.minimumLabel
            iconUrl: control.minimumIconSource.toString().length ? control.minimumIconSource : Theme.iconPath(control.minimumIconName)
            x: control.mirrored ? control.width - width : 0
            y: (control.height - height) / 2
        }
        Endpoint {
            id: maximumEndpoint
            objectName: "slider_maximum"
            label: control.maximumLabel
            iconUrl: control.maximumIconSource.toString().length ? control.maximumIconSource : Theme.iconPath(control.maximumIconName)
            x: control.mirrored ? 0 : control.width - width
            y: (control.height - height) / 2
        }
        Rectangle {
            id: track
            objectName: "slider_track"
            x: control.leftPadding + (control.filled ? 0 : control.rangeInset)
            y: (control.height - height) / 2
            width: control.filled ? control.availableWidth : control.rangeWidth
            height: control.trackHeight
            radius: height / 2
            color: control.effectivePressed ? control.pressedTrackColor : control.trackColor
            antialiasing: true

            Material {
                anchors.fill: parent
                visible: control.filled
                fillColor: track.color
            }
        }
        Rectangle {
            objectName: "slider_fill"
            x: control.leftPadding + (control.filled ? 0 : control.rangeInset) + control.fillStart * control.rangeWidth
            y: track.y
            width: control.fillLength * control.rangeWidth + (control.filled ? Math.min(control.trackHeight, control.availableWidth) : 0)
            height: control.trackHeight
            radius: height / 2
            color: control.resolvedFillColor
            visible: width > 0
            antialiasing: true
        }
        Item {
            objectName: "slider_ticks"
            visible: control.showTicks
            Repeater {
                model: control.showTicks ? control.resolvedTickCount : 0
                delegate: Rectangle {
                    required property int index
                    width: control.segmented ? Math.min(Theme.gap4, control.trackHeight / 4) : Theme.gap2
                    height: width
                    radius: width / 2
                    x: control.leftPadding + control.rangeInset + control.rangeWidth * index / (control.resolvedTickCount - 1) - width / 2
                    y: control.segmented ? (control.height - height) / 2 : (control.height + control.trackHeight) / 2 + Theme.scaleRealMetric(3)
                    color: control.tickColor
                    antialiasing: true
                }
            }
        }
        Rectangle {
            objectName: "slider_center"
            visible: control.centerBiased
            x: control.leftPadding + control.availableWidth / 2 - width / 2
            y: (control.height - height) / 2
            width: Theme.gap2
            height: control.trackHeight + Theme.gap4
            radius: width / 2
            color: control.tickColor
            antialiasing: true
        }
    }

    // Figma places the symbol above the value geometry, including the thumb at 0%.
    Image {
        objectName: "slider_symbol"
        parent: control
        z: 2
        visible: control.type === Slider.Filled && control.resolvedSize >= Slider.Regular && control.showSymbol
        width: Theme.iconSm
        height: Theme.iconSm
        x: control.leftPadding + (control.mirrored ? control.availableWidth - control.trackHeight : 0) + (control.trackHeight - width) / 2
        y: (control.height - height) / 2
        source: control.symbolSource.toString().length ? control.symbolSource : Theme.iconPath(control.symbolName)
        sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
    }

    handle: Item {
        objectName: "slider_handle"
        // Native pointer mapping uses this extent, independently of the visible thumb.
        implicitWidth: control.rangeInset * 2
        implicitHeight: control.controlHeight
        width: Math.min(implicitWidth, control.availableWidth)
        height: control.height
        x: control.leftPadding + Math.max(0, Math.min(1, control.displayedPosition)) * control.rangeWidth
        y: 0

        Rectangle {
            objectName: "slider_focus"
            anchors.centerIn: parent
            width: control.thumbDiameter + Theme.gap6
            height: width
            radius: width / 2
            color: "transparent"
            border.color: control.fillColor
            border.width: control.focusRingWidth
            visible: control.effectiveFocusRing
            antialiasing: true
        }
        Material {
            id: thumbVisual
            objectName: "slider_thumb"
            transform: InteractionMotion {
                target: thumbVisual
                pressed: control.pressed
                hovered: control.hovered
                focused: control.activeFocus
                motionEnabled: control.motionEnabled
                strength: 1.8
            }
            anchors.centerIn: parent
            width: control.thumbDiameter + inset * 2
            height: width
            fillColor: control.thumbColor
            thumb: true
            visible: control.effectiveThumbVisible
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Slider { type: LV.Slider.CenterBiasedTicks; from: -1; to: 1; value: 0 }
// LV.Slider { type: LV.Slider.Segmented; segmentCount: 5; minimumLabel: "Low"; maximumLabel: "High" }

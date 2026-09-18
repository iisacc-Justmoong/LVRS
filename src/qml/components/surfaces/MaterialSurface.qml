pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import QtQuick.Effects
import LVRS 1.0

Item {
    id: root
    property bool motionEnabled: true

    enum Density { Dense75, Glass25 }

    property int density: MaterialSurface.Glass25
    property color primaryColor: Theme.primary
    property color color: Theme.materialTint
    // Optional base for a captured backdrop; transparent preserves standalone materials.
    property color backdropBaseColor: "transparent"
    property real tintOpacity: density === MaterialSurface.Dense75 ? Theme.materialDenseOpacity : Theme.materialGlassOpacity
    property real blurRadius: density === MaterialSurface.Dense75 ? Theme.materialDenseBlur : Theme.materialGlassBlur
    property bool blurEnabled: true
    property real radius: Theme.materialPanelRadius
    property real borderWidth: Theme.scaleRealMetric(1)
    property color borderColor: density === MaterialSurface.Dense75 ? Theme.materialDenseEdge : Theme.materialGlassEdge
    property real innerHighlightOpacity: density === MaterialSurface.Dense75 ? 0.06 : 0.14
    property real innerHighlightOffsetY: Theme.scaleRealMetric(1)
    property real intenseOpacity: Theme.materialIntenseOpacity
    property real faintOpacity: Theme.materialFaintOpacity
    property real shadowRadius: Theme.scaleRealMetric(24)
    property real shadowOffsetY: Theme.scaleRealMetric(8)
    property real shadowOpacity: 0.22
    // A popup's content and window background are siblings of its overlay.
    // Explicit sources must never include this material or its descendants.
    property Item backdropSource: null
    property Item backdropBackground: null
    property string outlinePath: ""

    readonly property bool rendererAvailable: GraphicsInfo.api !== GraphicsInfo.Software
        && GraphicsInfo.api !== GraphicsInfo.Unknown && GraphicsInfo.api !== GraphicsInfo.Null
    readonly property bool effectsActive: visible && width > 0 && height > 0 && rendererAvailable
    readonly property Item resolvedBackdropSource: safeSource(backdropSource)
    readonly property Item resolvedBackdropBackground: safeSource(backdropBackground)
    readonly property bool captureActive: effectsActive && (resolvedBackdropSource !== null || resolvedBackdropBackground !== null)
    readonly property real capturePadding: Math.max(2, Math.min(64, Math.ceil(blurRadius)))
    readonly property string resolvedOutline: outlinePath.length > 0 ? outlinePath : roundedOutline()
    property color displayedTint: resolvedTint
    property color displayedPrimaryColor: primaryColor
    StateColorBehavior on displayedTint { motionEnabled: root.motionEnabled }
    StateColorBehavior on displayedPrimaryColor { motionEnabled: root.motionEnabled }
    readonly property color resolvedTint: Qt.rgba(color.r, color.g, color.b, color.a * Math.max(0, Math.min(1, tintOpacity)))

    function safeSource(source: Item): Item {
        if (!source)
            return null
        for (let ancestor = root; ancestor; ancestor = ancestor.parent)
            if (ancestor === source) return null
        for (let ancestor = source; ancestor; ancestor = ancestor.parent)
            if (ancestor === root) return null
        return source
    }

    function mappedCaptureRect(source: Item): rect {
        if (!source)
            return Qt.rect(0, 0, composite.width, composite.height)
        // Reading both ancestor chains invalidates the crop on popup animation,
        // scrolling, window supersampling, or transformed target movement.
        for (const item of [root, source]) {
            for (let ancestor = item; ancestor; ancestor = ancestor.parent) {
                const transform = [ancestor.x, ancestor.y, ancestor.width, ancestor.height,
                    ancestor.scale, ancestor.rotation, ancestor.transformOrigin]
            }
        }
        return root.mapToItem(source, -capturePadding, -capturePadding,
                              width + capturePadding * 2, height + capturePadding * 2)
    }

    function roundedOutline(): string {
        const w = Math.max(0, width), h = Math.max(0, height)
        const r = Math.max(0, Math.min(radius, w / 2, h / 2))
        return `M ${r} 0 H ${w-r} Q ${w} 0 ${w} ${r} V ${h-r} Q ${w} ${h} ${w-r} ${h} H ${r} Q 0 ${h} 0 ${h-r} V ${r} Q 0 0 ${r} 0 Z`
    }

    // Fixed edge offsets survive window resizing; center accents keep their
    // original positions and strength. Radii are logical pixels, as in Figma.
    readonly property var gradientLayout: [
        { name: "centerIntense", cx: width / 2 - Theme.scaleRealMetric(129), cy: height / 2 - Theme.scaleRealMetric(28), rx: 200, ry: 200, strong: true },
        { name: "centerFaint", cx: width / 2 + Theme.scaleRealMetric(136), cy: height / 2 + Theme.scaleRealMetric(62), rx: 180, ry: 180, strong: false },
        { name: "topLeft", cx: Theme.scaleRealMetric(48), cy: Theme.scaleRealMetric(32), rx: 200, ry: 200, strong: true },
        { name: "topRight", cx: width - Theme.scaleRealMetric(64), cy: Theme.scaleRealMetric(40), rx: 260, ry: 150, strong: false },
        { name: "leftEdge", cx: 0, cy: height / 2, rx: 140, ry: 300, strong: false },
        { name: "rightEdge", cx: width, cy: height / 2, rx: 160, ry: 300, strong: true },
        { name: "bottomEdge", cx: width / 2, cy: height, rx: 380, ry: 150, strong: false },
        { name: "bottomLeft", cx: Theme.scaleRealMetric(72), cy: height - Theme.scaleRealMetric(40), rx: 180, ry: 180, strong: true }
    ]

    component RadialLayer: Item {
        id: radials
        required property color accent
        required property real intense
        required property real faint
        required property string outline
        property real inset: 0

        Repeater {
            model: radials.intense > 0 || radials.faint > 0 ? root.gradientLayout : []
            delegate: Shape {
                id: blob
                required property var modelData
                objectName: "materialGradient_" + modelData.name
                readonly property real stretchY: modelData.ry / modelData.rx
                readonly property real strength: modelData.strong ? radials.intense : radials.faint
                width: radials.width
                height: radials.height / stretchY
                antialiasing: true
                transform: Scale { yScale: blob.stretchY }
                ShapePath {
                    strokeWidth: -1
                    // Counter-scale only the silhouette; the radial fill becomes
                    // elliptical without requiring Qt 6.8's fillTransform API.
                    scale: Qt.size(1, 1 / blob.stretchY)
                    fillGradient: RadialGradient {
                        centerX: radials.inset + blob.modelData.cx
                        centerY: (radials.inset + blob.modelData.cy) / blob.stretchY
                        focalX: centerX; focalY: centerY
                        centerRadius: Theme.scaleRealMetric(blob.modelData.rx)
                        GradientStop { position: 0; color: Qt.rgba(radials.accent.r, radials.accent.g, radials.accent.b, radials.accent.a * blob.strength) }
                        GradientStop { position: 0.38; color: Qt.rgba(radials.accent.r, radials.accent.g, radials.accent.b, radials.accent.a * blob.strength * 0.55) }
                        GradientStop { position: 0.72; color: Qt.rgba(radials.accent.r, radials.accent.g, radials.accent.b, radials.accent.a * blob.strength * 0.13) }
                        GradientStop { position: 1; color: "transparent" }
                    }
                    PathSvg { path: radials.outline }
                }
            }
        }
    }

    Item {
        id: composite
        width: root.width + root.capturePadding * 2
        height: root.height + root.capturePadding * 2
        visible: false
        Rectangle {
            anchors.fill: parent
            color: root.backdropBaseColor
            StateColorBehavior on color { motionEnabled: root.motionEnabled }
        }
        ShaderEffectSource {
            objectName: "materialBackgroundCapture"
            anchors.fill: parent
            sourceItem: root.captureActive ? root.resolvedBackdropBackground : null
            sourceRect: root.mappedCaptureRect(sourceItem)
            live: root.captureActive
            hideSource: false
            recursive: false
        }
        ShaderEffectSource {
            objectName: "materialContentCapture"
            anchors.fill: parent
            sourceItem: root.captureActive ? root.resolvedBackdropSource : null
            sourceRect: root.mappedCaptureRect(sourceItem)
            live: root.captureActive
            hideSource: false
            recursive: false
        }
        RadialLayer {
            anchors.fill: parent
            accent: root.displayedPrimaryColor; intense: root.intenseOpacity; faint: root.faintOpacity
            inset: root.capturePadding
            outline: `M 0 0 H ${width} V ${height} H 0 Z`
        }
    }

    Item {
        id: mask
        width: composite.width; height: composite.height
        visible: false
        layer.enabled: root.effectsActive
        Shape {
            x: root.capturePadding; y: root.capturePadding
            width: root.width; height: root.height
            antialiasing: true
            ShapePath { fillColor: "white"; strokeWidth: -1; PathSvg { path: root.resolvedOutline } }
        }
    }

    MultiEffect {
        objectName: "materialShadow"
        x: -root.capturePadding; y: -root.capturePadding + root.shadowOffsetY
        width: composite.width; height: composite.height
        source: mask
        visible: root.effectsActive && root.shadowOpacity > 0
        opacity: root.shadowOpacity
        colorization: 1; colorizationColor: "black"
        blurEnabled: true
        blurMax: Math.max(2, Math.min(64, Math.ceil(root.shadowRadius)))
        blur: 1
        autoPaddingEnabled: false
    }

    MultiEffect {
        objectName: "materialDiffusion"
        x: -root.capturePadding; y: -root.capturePadding
        width: composite.width; height: composite.height
        source: composite
        visible: root.effectsActive
        blurEnabled: root.blurEnabled
        blurMax: Math.max(2, Math.min(64, Math.ceil(root.blurRadius)))
        blur: 1
        autoPaddingEnabled: false
        maskEnabled: true
        maskSource: mask
    }

    // Software scene graphs keep the same color, alpha and silhouette without
    // requesting GPU textures. Only native RHI paths apply backdrop diffusion.
    RadialLayer {
        objectName: "materialSoftwareRadials"
        anchors.fill: parent
        visible: !root.rendererAvailable
        accent: root.displayedPrimaryColor; intense: root.intenseOpacity; faint: root.faintOpacity
        outline: root.resolvedOutline
    }

    Shape {
        objectName: "materialTint"
        anchors.fill: parent
        antialiasing: true
        ShapePath {
            fillColor: root.displayedTint
            strokeColor: root.borderColor
            strokeWidth: root.borderWidth > 0 ? root.borderWidth : -1
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.resolvedOutline }
        }
    }

    // Figma's zero-blur inset shadow is the original silhouette minus the
    // silhouette shifted down by 1px. The existing mask also supports tails.
    Item {
        id: inverseHighlight
        width: composite.width; height: composite.height
        visible: false
        layer.enabled: root.effectsActive && root.innerHighlightOpacity > 0
        Shape {
            x: root.capturePadding
            y: root.capturePadding + root.innerHighlightOffsetY
            width: root.width; height: root.height
            antialiasing: true
            ShapePath {
                fillColor: "white"
                strokeWidth: -1
                fillRule: ShapePath.OddEvenFill
                PathSvg {
                    path: `M ${-root.capturePadding} ${-root.capturePadding}
                        H ${root.width + root.capturePadding} V ${root.height + root.capturePadding}
                        H ${-root.capturePadding} Z ` + root.resolvedOutline
                }
            }
        }
    }
    MultiEffect {
        objectName: "materialInnerHighlight"
        x: -root.capturePadding; y: -root.capturePadding
        width: composite.width; height: composite.height
        visible: root.effectsActive && root.innerHighlightOpacity > 0
        source: inverseHighlight
        opacity: root.innerHighlightOpacity
        maskEnabled: true
        maskSource: mask
        autoPaddingEnabled: false
    }
}

// API usage (external):
// LV.PanelMaterial { primaryColor: LV.Theme.primary; backdropSource: siblingContent }
// LV.WindowMaterial { density: LV.MaterialSurface.Dense75 }

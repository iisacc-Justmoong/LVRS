pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

AbstractInputBar {
    id: control

    readonly property int defaultMode: 0
    readonly property int searchMode: 1
    readonly property int filledStyle: 0
    readonly property int inlineStyle: 1

    property int mode: defaultMode
    property int style: filledStyle
    property alias placeholder: control.placeholderText
    property bool clearButtonVisible: true
    property bool searchIconVisible: mode === searchMode
    property color searchIconColor: Theme.descriptionColor
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
    readonly property real searchIconLensRadius: Theme.scaleRealMetric(4)
    readonly property int searchIconSourceSize: Math.max(1, Math.ceil(Theme.iconSm * searchIconRasterScale))
    readonly property string searchIconSnapshotProfile: Theme.mobileTarget ? "mobile" : "desktop"
    readonly property bool searchIconUsesPlatformSnapshot: true
    readonly property url searchIconSnapshotSource: "data:image/svg+xml;utf8," + encodeURIComponent(searchIconSnapshotSvgMarkup)
    readonly property string searchIconSnapshotSvgMarkup: control.buildSearchSnapshotSvg()

    readonly property int resolvedStyle: style === inlineStyle ? inlineStyle : filledStyle
    readonly property color frameFillColor: resolvedStyle === inlineStyle
        ? Theme.accentTransparent
        : Theme.panelBackground10
    readonly property color frameFillColorHover: resolvedStyle === inlineStyle
        ? Theme.accentTransparent
        : Theme.panelBackground11
    readonly property color frameFillColorPressed: resolvedStyle === inlineStyle
        ? Theme.accentTransparent
        : Theme.panelBackground12
    readonly property bool showClearButton: clearButtonVisible
        && enabled
        && !readOnly
        && text.length > 0

    function svgColor(value) {
        return "rgba(" + Math.round(value.r * 255)
            + "," + Math.round(value.g * 255)
            + "," + Math.round(value.b * 255)
            + "," + value.a.toFixed(3) + ")"
    }

    function buildSearchSnapshotSvg() {
        const size = Theme.iconSm
        const lensCenter = size * 0.42
        const stroke = Math.max(1.0, searchIconStrokeWidth)
        const handleStart = size * 0.63
        const handleEnd = size * 0.84
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + size + "\" height=\"" + size
            + "\" viewBox=\"0 0 " + size + " " + size + "\" fill=\"none\">"
            + "<circle cx=\"" + lensCenter + "\" cy=\"" + lensCenter + "\" r=\"" + searchIconLensRadius
            + "\" stroke=\"" + svgColor(searchIconColor) + "\" stroke-width=\"" + stroke + "\"/>"
            + "<path d=\"M " + handleStart + " " + handleStart + " L " + handleEnd + " " + handleEnd
            + "\" stroke=\"" + svgColor(searchIconColor)
            + "\" stroke-width=\"" + stroke + "\" stroke-linecap=\"round\"/>"
            + "</svg>"
    }

    implicitWidth: Theme.inputWidthMd
    fieldMinHeight: Theme.controlHeightSm
    insetHorizontal: Theme.gap7
    insetVertical: Theme.gap3
    sideSpacing: Theme.gap5
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
    backgroundColorDisabled: frameFillColor

    selectionColor: Theme.accent
    selectedTextColor: Theme.textPrimary

    leadingInternalItems: Item {
        id: searchIconHost
        width: control.searchIconVisible ? Theme.iconSm : 0
        height: Theme.iconSm
        visible: width > 0

        Image {
            id: searchIcon
            anchors.fill: parent
            objectName: control.objectName.length > 0 ? control.objectName + "_searchIconSnapshot" : ""
            source: control.searchIconSnapshotSource
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
        width: control.showClearButton ? Theme.iconSm : 0
        height: Theme.iconSm
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
            width: Theme.scaleMetric(14)
            height: Theme.scaleMetric(14)
            radius: Theme.scaleMetric(7)
            color: clearButton.backgroundColor
            antialiasing: true

            Rectangle {
                width: Theme.scaleMetric(8)
                height: Theme.scaleRealMetric(1.4)
                radius: Theme.scaleRealMetric(0.7)
                color: control.clearIconForegroundColor
                anchors.centerIn: parent
                rotation: 45
                antialiasing: true
            }

            Rectangle {
                width: Theme.scaleMetric(8)
                height: Theme.scaleRealMetric(1.4)
                radius: Theme.scaleRealMetric(0.7)
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
// LV.InputField { placeholderText: "Search"; mode: searchMode; style: inlineStyle }

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
    readonly property int searchIconSize: Theme.iconSm
    readonly property url searchIconSource: Theme.iconPath("generalsearch")
    readonly property url renderedSearchIconSource: RenderQuality.resolveTextureSource(searchIconSource)
    readonly property int searchIconSourceSize: Math.max(1, Math.ceil(searchIconSize * searchIconRasterScale))
    readonly property int clearIconSize: Theme.iconSm
    readonly property real clearIconMarkLength: clearIconSize * (8.0 / 14.0)
    readonly property real clearIconMarkThickness: clearIconSize * (1.4 / 14.0)

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

    implicitWidth: Theme.inputWidthMd
    fieldMinHeight: Theme.controlHeightSm
    insetHorizontal: Theme.gap7
    insetVertical: Theme.gap3
    sideSpacing: Theme.gap2
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
// LV.InputField { placeholderText: "Search"; search: true; style: inlineStyle }

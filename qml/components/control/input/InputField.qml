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
    property real searchIconStrokeWidth: 1.5
    property color clearIconBackgroundColor: Theme.descriptionColor
    property color clearIconBackgroundColorHover: Qt.lighter(clearIconBackgroundColor, 1.08)
    property color clearIconBackgroundColorPressed: Qt.darker(clearIconBackgroundColor, 1.14)
    property color clearIconBackgroundColorDisabled: Theme.disabledColor
    property color clearIconForegroundColor: Theme.panelBackground10

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

        Canvas {
            id: searchIcon
            anchors.fill: parent
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!control.searchIconVisible)
                    return

                ctx.beginPath()
                ctx.arc(width * 0.42, height * 0.42, 4.0, 0, Math.PI * 2, false)
                ctx.lineWidth = control.searchIconStrokeWidth
                ctx.strokeStyle = control.searchIconColor
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(width * 0.63, height * 0.63)
                ctx.lineTo(width * 0.84, height * 0.84)
                ctx.lineWidth = control.searchIconStrokeWidth
                ctx.lineCap = "round"
                ctx.strokeStyle = control.searchIconColor
                ctx.stroke()
            }
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
            width: 14
            height: 14
            radius: 7
            color: clearButton.backgroundColor
            antialiasing: true

            Rectangle {
                width: 8
                height: 1.4
                radius: 0.7
                color: control.clearIconForegroundColor
                anchors.centerIn: parent
                rotation: 45
                antialiasing: true
            }

            Rectangle {
                width: 8
                height: 1.4
                radius: 0.7
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

    onModeChanged: searchIcon.requestPaint()
    onSearchIconVisibleChanged: searchIcon.requestPaint()
    onSearchIconColorChanged: searchIcon.requestPaint()
    onSearchIconStrokeWidthChanged: searchIcon.requestPaint()
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.InputField { placeholderText: "Search"; mode: searchMode; style: inlineStyle }

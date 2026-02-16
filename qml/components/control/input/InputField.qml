pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

AbstractInputBar {
    id: control

    readonly property int defaultMode: 0
    readonly property int searchMode: 1

    property int mode: defaultMode
    property alias placeholder: control.placeholderText
    property bool clearButtonVisible: true
    property bool searchIconVisible: mode === searchMode
    property color searchIconColor: Theme.textOctonary
    property real searchIconStrokeWidth: 1.5
    property color clearIconBackgroundColor: Theme.textTertiary
    property color clearIconBackgroundColorDisabled: Theme.textOctonary
    property color clearIconForegroundColor: Theme.subSurface

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

    textColor: Theme.textPrimary
    textColorDisabled: Theme.textOctonary
    placeholderColor: Theme.textPrimary
    placeholderColorDisabled: Theme.textPrimary
    placeholderOpacity: 0.33

    backgroundColor: Theme.subSurface
    backgroundColorFocused: Theme.subSurface
    backgroundColorDisabled: Theme.subSurface

    selectionColor: Theme.accent
    selectedTextColor: Theme.textPrimary

    cursorDelegate: Rectangle {
        id: cursorHandle
        width: Theme.strokeThin
        height: Theme.iconSm
        radius: Theme.radiusHairline
        color: Theme.textTertiary
        visible: control.focused && control.enabled && !control.readOnly
        opacity: visible ? 1.0 : 0.0

        SequentialAnimation {
            running: cursorHandle.visible
            loops: Animation.Infinite

            NumberAnimation {
                target: cursorHandle
                property: "opacity"
                to: 1.0
                duration: 520
            }

            NumberAnimation {
                target: cursorHandle
                property: "opacity"
                to: 0.0
                duration: 520
            }
        }

        onVisibleChanged: opacity = visible ? 1.0 : 0.0
    }

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
        readonly property color backgroundColor: control.enabled
            ? control.clearIconBackgroundColor
            : control.clearIconBackgroundColorDisabled

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
            anchors.fill: parent
            enabled: control.showClearButton
            acceptedButtons: Qt.LeftButton
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
// LV.InputField { placeholderText: "Search"; mode: searchMode }

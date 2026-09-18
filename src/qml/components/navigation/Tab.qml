pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control
    enum TabStyle { Underline, Surface }
    enum DisplayState { StateAutomatic, StateDefault, StateHover, StatePressed, StateSelected, StateFocus, StateDisabled }
    property int tabStyle: Tab.Underline
    property int displayState: Tab.StateAutomatic
    property bool selected: false
    property string iconName: ""
    property url iconSource: Theme.iconPath(iconName)
    property string badge: ""
    property bool showIcon: iconSource.toString().length > 0
    property bool showBadge: badge.length > 0
    property int iconSize: 18
    property string labelObjectName: "tabLabel"
    property var navigationBar: null
    property int tabIndex: -1
    readonly property bool effectiveSelected: selected || displayState === Tab.StateSelected
    readonly property bool effectiveHovered: displayState === Tab.StateHover || (displayState === Tab.StateAutomatic && hovered)
    readonly property bool effectivePressed: displayState === Tab.StatePressed || (displayState === Tab.StateAutomatic && down)
    readonly property bool effectiveFocus: displayState === Tab.StateFocus || visualFocus
    effectiveEnabled: enabled && displayState !== Tab.StateDisabled
    enabled: displayState !== Tab.StateDisabled
    showFocusRing: false
    horizontalPadding: 12
    verticalPadding: 0
    bottomPadding: tabStyle === Tab.Underline ? 2 : 0
    cornerRadius: tabStyle === Tab.Surface ? 4 : 0
    implicitHeight: 32
    implicitWidth: Math.max(80, contentItem.implicitWidth + leftPadding + rightPadding)
    textColor: !effectiveEnabled ? Theme.disabledColor : effectiveSelected ? Theme.titleHeaderColor : Theme.bodyColor
    backgroundColor: effectiveSelected && tabStyle === Tab.Surface ? Theme.panelBackground10 : "transparent"
    backgroundColorHover: Theme.panelBackground06
    backgroundColorPressed: Theme.panelBackground08
    backgroundColorDisabled: "transparent"
    Accessible.role: Accessible.PageTab
    Accessible.name: text
    Accessible.selectable: true
    Accessible.selected: effectiveSelected
    Accessible.onPressAction: if (effectiveEnabled) clicked()

    Keys.onPressed: function(event) {
        if (!effectiveEnabled) return
        if (navigationBar && (event.key === Qt.Key_Left || event.key === Qt.Key_Right)) {
            navigationBar.moveFocus(tabIndex, (event.key === Qt.Key_Right ? 1 : -1) * (mirrored ? -1 : 1))
            event.accepted = true
        } else if (navigationBar && (event.key === Qt.Key_Home || event.key === Qt.Key_End)) {
            navigationBar.focusBoundary(event.key === Qt.Key_End)
            event.accepted = true
        } else if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !event.isAutoRepeat) {
            clicked()
            event.accepted = true
        }
    }

    contentItem: RowLayout {
        spacing: 8
        Image {
            visible: control.showIcon
            source: RenderQuality.resolveTextureSource(control.iconSource)
            sourceSize: Qt.size(control.iconSize * Screen.devicePixelRatio, control.iconSize * Screen.devicePixelRatio)
            Layout.preferredWidth: control.iconSize
            Layout.preferredHeight: control.iconSize
            Layout.minimumWidth: control.iconSize
            Layout.maximumWidth: control.iconSize
            fillMode: Image.PreserveAspectFit
            opacity: control.effectiveEnabled ? 1 : 0.3
            Accessible.ignored: true
        }
        Label {
            objectName: control.labelObjectName
            style: body
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: control.text
            color: control.textColor
            elide: Text.ElideRight
            Accessible.ignored: true
        }
        Label {
            visible: control.showBadge
            style: caption
            text: control.badge
            color: Theme.descriptionColor
            Accessible.ignored: true
        }
    }
    background: Rectangle {
        objectName: "tabSurface"
        radius: control.resolvedCornerRadius
        color: !control.effectiveEnabled ? control.backgroundColorDisabled
            : control.effectivePressed ? control.backgroundColorPressed
            : control.effectiveHovered ? control.backgroundColorHover : control.backgroundColor
        StateColorBehavior on color { motionEnabled: control.motionEnabled }
        Rectangle {
            objectName: "tabIndicator"
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            color: Theme.primary
            visible: control.effectiveSelected && control.tabStyle === Tab.Underline
        }
    }
    FocusRing {
        anchors.fill: parent
        active: control.effectiveEnabled && control.effectiveFocus
        radius: control.resolvedCornerRadius
        border.width: 2
        motionEnabled: control.motionEnabled
    }
}

// LV.Tab { text: "Overview"; selected: true; tabStyle: LV.Tab.Underline }

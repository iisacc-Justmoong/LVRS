import QtQml

// Nonvisual state, created and owned by each complete control instance.
QtObject {
    property bool enabled: true
    property bool pressed: false
    property bool hovered: false
    property bool focused: false
    property bool releasing: false

    readonly property string phase: !enabled ? "disabled"
        : pressed ? "press"
        : releasing ? "release"
        : focused ? "focus"
        : hovered ? "hover"
        : "default"
    readonly property string input: focused ? "keyboard" : "pointer"
    readonly property bool focusVisible: enabled && focused
    // Focus/release retain the existing hover/default surface policy.
    readonly property string surfacePhase: !enabled ? "disabled"
        : pressed ? "press"
        : hovered ? "hover"
        : "default"
}

// API usage (external):
// LV.InteractionState { pressed: control.down; hovered: control.hovered; focused: control.visualFocus }

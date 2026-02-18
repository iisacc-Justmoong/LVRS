import QtQuick
import LVRS 1.0

IconButton {
    id: control

    readonly property bool __isToolbarButton: true
    property var buttonId: -1
    property var toolbar: null

    readonly property bool active: toolbar && toolbar.activeButton === control

    tone: active ? AbstractButton.Default : AbstractButton.Borderless

    onClicked: {
        if (toolbar && toolbar.requestActivate)
            toolbar.requestActivate(control)
    }

    onToolbarChanged: {
        if (toolbar && toolbar.registerButton)
            toolbar.registerButton(control)
    }

    QtObject {
        Component.onCompleted: {
            if (control.toolbar && control.toolbar.registerButton)
                control.toolbar.registerButton(control)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ToolbarButton { buttonId: 1; iconName: "viewMoreSymbolicDefault" }

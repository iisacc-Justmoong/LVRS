import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // SwiftUI-like API: minLength applies along the stack axis.
    property int minLength: 0
    // Used by VStack/HStack to force axis without relying on parent meta info.
    property string stackAxis: ""

    implicitWidth: 0
    implicitHeight: 0

    Layout.fillWidth: root._fillWidth
    Layout.fillHeight: root._fillHeight
    Layout.minimumWidth: root._minWidth
    Layout.minimumHeight: root._minHeight

    property bool _fillWidth: false
    property bool _fillHeight: false
    property int _minWidth: 0
    property int _minHeight: 0

    onMinLengthChanged: updateLayout()
    onStackAxisChanged: updateLayout()
    onParentChanged: updateLayout()
    Component.onCompleted: updateLayout()

    function hasStackFlag(item, flagName) {
        return !!(item && item.property && item.property(flagName) === true)
    }

    function updateLayout() {
        _fillWidth = false
        _fillHeight = false
        _minWidth = 0
        _minHeight = 0
        anchors.fill = null

        if (!parent)
            return

        var className = String(parent || "")
        var inLayout = className.indexOf("RowLayout") !== -1 || className.indexOf("ColumnLayout") !== -1
        var inVStack = hasStackFlag(parent, "__isVStack")
        var inHStack = hasStackFlag(parent, "__isHStack")
        if (!(inVStack || inHStack || inLayout) && stackAxis !== "")
            stackAxis = ""

        if (hasStackFlag(parent, "__isZStack")) {
            anchors.fill = parent
            return
        }

        var resolvedAxis = stackAxis
        if (resolvedAxis === "") {
            var parentParent = parent.parent
            if (inVStack || hasStackFlag(parentParent, "__isVStack"))
                resolvedAxis = "vertical"
            else if (inHStack || hasStackFlag(parentParent, "__isHStack"))
                resolvedAxis = "horizontal"
        }

        if (resolvedAxis === "vertical") {
            _fillHeight = true
            _minHeight = minLength
            return
        }
        if (resolvedAxis === "horizontal") {
            _fillWidth = true
            _minWidth = minLength
            return
        }

        if (className.indexOf("RowLayout") !== -1) {
            _fillWidth = true
            _minWidth = minLength
        } else if (className.indexOf("ColumnLayout") !== -1) {
            _fillHeight = true
            _minHeight = minLength
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Spacer { minLength: 12 }

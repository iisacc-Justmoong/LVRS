import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: root

    // SwiftUI-like API: spacing == -1 uses defaultSpacing; alignment is horizontal only.
    property int spacing: -1
    property int defaultSpacing: Theme.gap8
    property int alignment: Qt.AlignHCenter
    // alignmentName supports SwiftUI-style names: leading, center, trailing.
    property string alignmentName: ""
    property bool __isVStack: true
    property var _managedAlignmentChildren: []

    default property alias content: contentColumn.data

    implicitWidth: contentColumn.implicitWidth
    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        spacing: root.spacing < 0 ? root.defaultSpacing : root.spacing
    }

    onAlignmentChanged: updateAlignment()
    onAlignmentNameChanged: updateAlignment()
    Component.onCompleted: {
        updateAlignment()
        updateStackAxis()
    }

    function updateAlignment() {
        var normalized = normalizeAlignment(root.alignment, root.alignmentName)
        var current = contentColumn.children
        var kept = []
        for (var i = 0; i < current.length; i++) {
            var child = current[i]
            if (child && child.Layout !== undefined)
                kept.push(child)
        }
        root._managedAlignmentChildren = root._managedAlignmentChildren.filter(function(item) {
            return kept.indexOf(item) !== -1
        })

        for (var i = 0; i < current.length; i++) {
            var child = contentColumn.children[i]
            if (!child || child.Layout === undefined)
                continue
            var idx = root._managedAlignmentChildren.indexOf(child)
            if (child.Layout.alignment === 0) {
                child.Layout.alignment = normalized
                if (idx === -1)
                    root._managedAlignmentChildren.push(child)
                continue
            }
            if (idx !== -1) {
                if (child.Layout.alignment !== normalized) {
                    root._managedAlignmentChildren.splice(idx, 1)
                    continue
                }
                child.Layout.alignment = normalized
            }
        }
    }

    function updateStackAxis() {
        for (var i = 0; i < contentColumn.children.length; i++) {
            var child = contentColumn.children[i]
            if (child && child.stackAxis !== undefined)
                child.stackAxis = "vertical"
        }
    }

    function normalizeAlignment(value, name) {
        var named = normalizeAlignmentName(name)
        if (named !== 0)
            return named
        if (value & Qt.AlignRight)
            return Qt.AlignRight
        if (value & Qt.AlignHCenter)
            return Qt.AlignHCenter
        return Qt.AlignLeft
    }

    function normalizeAlignmentName(name) {
        var token = String(name || "").toLowerCase()
        if (!token)
            return 0
        if (token === "leading")
            return Qt.AlignLeft
        if (token === "center")
            return Qt.AlignHCenter
        if (token === "trailing")
            return Qt.AlignRight
        return 0
    }

    Connections {
        target: contentColumn
        function onChildrenChanged() {
            root.updateAlignment()
            root.updateStackAxis()
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.VStack { spacing: 8; LV.Label { text: "Title"; style: body } }

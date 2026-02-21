import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: root

    // SwiftUI-like API: spacing == -1 uses defaultSpacing; alignment is vertical only.
    property int spacing: -1
    property int defaultSpacing: Theme.gap8
    property int alignment: Qt.AlignVCenter
    // alignmentName supports SwiftUI-style names: top, center, bottom.
    property string alignmentName: ""
    property bool __isHStack: true
    property var _managedAlignmentChildren: []

    default property alias content: contentRow.data

    implicitWidth: contentRow.implicitWidth
    implicitHeight: contentRow.implicitHeight

    RowLayout {
        id: contentRow
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
        var current = contentRow.children
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
            var child = contentRow.children[i]
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
        for (var i = 0; i < contentRow.children.length; i++) {
            var child = contentRow.children[i]
            if (child && child.stackAxis !== undefined)
                child.stackAxis = "horizontal"
        }
    }

    function normalizeAlignment(value, name) {
        var named = normalizeAlignmentName(name)
        if (named !== 0)
            return named
        if (value & Qt.AlignBottom)
            return Qt.AlignBottom
        if (value & Qt.AlignVCenter)
            return Qt.AlignVCenter
        return Qt.AlignTop
    }

    function normalizeAlignmentName(name) {
        var token = String(name || "").toLowerCase()
        if (!token)
            return 0
        if (token === "top")
            return Qt.AlignTop
        if (token === "center")
            return Qt.AlignVCenter
        if (token === "bottom")
            return Qt.AlignBottom
        return 0
    }

    Connections {
        target: contentRow
        function onChildrenChanged() {
            root.updateAlignment()
            root.updateStackAxis()
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.HStack { spacing: 12; LV.Label { text: "Left"; style: body }; LV.Label { text: "Right"; style: body } }

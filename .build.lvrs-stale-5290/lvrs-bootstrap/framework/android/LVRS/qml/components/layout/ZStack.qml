import QtQuick

Item {
    id: root

    // SwiftUI-like API: alignment defaults to center.
    property int alignment: Qt.AlignCenter
    property bool __isZStack: true
    // alignmentName supports SwiftUI-style names:
    // topLeading, top, topTrailing, leading, center, trailing, bottomLeading, bottom, bottomTrailing.
    property string alignmentName: ""

    default property alias content: contentLayer.data

    // Keep implicit size stable to avoid self-referential loops with anchor-managed children.
    implicitWidth: 0
    implicitHeight: 0

    Item {
        id: contentLayer
        anchors.fill: parent
    }

    onAlignmentChanged: updateAlignment()
    onAlignmentNameChanged: updateAlignment()
    Component.onCompleted: updateAlignment()

    function updateAlignment() {
        var normalized = normalizeAlignment(root.alignment, root.alignmentName)
        for (var i = 0; i < contentLayer.children.length; i++) {
            var child = contentLayer.children[i]
            if (!child || !child.anchors)
                continue
            var managed = child.__zstackManaged === true
            if (!managed && hasAnyAnchor(child))
                continue
            applyAlignment(child, normalized)
            child.__zstackManaged = true
        }
    }

    function hasAnyAnchor(child) {
        return child.anchors.left || child.anchors.right || child.anchors.top
            || child.anchors.bottom || child.anchors.horizontalCenter || child.anchors.verticalCenter
    }

    function applyAlignment(child, alignmentValue) {
        child.anchors.left = undefined
        child.anchors.right = undefined
        child.anchors.top = undefined
        child.anchors.bottom = undefined
        child.anchors.horizontalCenter = undefined
        child.anchors.verticalCenter = undefined

        if (alignmentValue & Qt.AlignHCenter)
            child.anchors.horizontalCenter = contentLayer.horizontalCenter
        else if (alignmentValue & Qt.AlignRight)
            child.anchors.right = contentLayer.right
        else
            child.anchors.left = contentLayer.left

        if (alignmentValue & Qt.AlignVCenter)
            child.anchors.verticalCenter = contentLayer.verticalCenter
        else if (alignmentValue & Qt.AlignBottom)
            child.anchors.bottom = contentLayer.bottom
        else
            child.anchors.top = contentLayer.top
    }

    function normalizeAlignment(value, name) {
        var named = normalizeAlignmentName(name)
        if (named !== 0)
            return named
        var horizontal = Qt.AlignLeft
        var vertical = Qt.AlignTop

        if (value & Qt.AlignRight)
            horizontal = Qt.AlignRight
        else if (value & Qt.AlignHCenter)
            horizontal = Qt.AlignHCenter

        if (value & Qt.AlignBottom)
            vertical = Qt.AlignBottom
        else if (value & Qt.AlignVCenter)
            vertical = Qt.AlignVCenter

        return horizontal | vertical
    }

    function normalizeAlignmentName(name) {
        var token = String(name || "").toLowerCase()
        if (!token)
            return 0
        if (token === "center")
            return Qt.AlignHCenter | Qt.AlignVCenter
        if (token === "leading")
            return Qt.AlignLeft | Qt.AlignVCenter
        if (token === "trailing")
            return Qt.AlignRight | Qt.AlignVCenter
        if (token === "top")
            return Qt.AlignHCenter | Qt.AlignTop
        if (token === "bottom")
            return Qt.AlignHCenter | Qt.AlignBottom
        if (token === "topleading")
            return Qt.AlignLeft | Qt.AlignTop
        if (token === "toptrailing")
            return Qt.AlignRight | Qt.AlignTop
        if (token === "bottomleading")
            return Qt.AlignLeft | Qt.AlignBottom
        if (token === "bottomtrailing")
            return Qt.AlignRight | Qt.AlignBottom
        return 0
    }

    Connections {
        target: contentLayer
        function onChildrenChanged() {
            root.updateAlignment()
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ZStack { Rectangle { width: 40; height: 40 } }

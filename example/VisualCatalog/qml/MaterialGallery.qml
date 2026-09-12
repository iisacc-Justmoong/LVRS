pragma ComponentBehavior: Bound
import QtQuick
import LVRS as LV

Item {
    id: root
    objectName: "materialGallery"
    implicitHeight: column.implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: LV.Theme.gap16
        LV.Label {
            width: parent.width; style: body; wrapMode: Text.WordWrap; sizeToContentHeight: true
            text: "Window: a uniform #0B0B0B fill at 50%, without gradients. Menus: 12% tint and 64px blur. Tooltips and popovers: Glass 25. Choose the app accent below."
        }
        Row {
            spacing: LV.Theme.gap8
            Repeater {
                model: [LV.Theme.defaultPrimary, LV.Theme.accentPurple, LV.Theme.accentGreen, LV.Theme.accentOrangeMuted]
                LV.LabelButton {
                    required property color modelData
                    text: String(modelData).toUpperCase()
                    tone: LV.AbstractButton.Primary
                    backgroundColor: modelData
                    onClicked: LV.Theme.primaryColor = modelData
                }
            }
        }
        LV.WindowMaterial {
            width: parent.width; height: 260
            LV.Label { x: 24; y: 24; style: header2; text: "Window · Solid 50" }
            LV.Label { x: 24; y: 58; style: body; text: "50% near-black fill · no gradients · 64 px blur" }
            Row {
                x: 24; y: 112; spacing: LV.Theme.gap12
                LV.LabelButton { id: menuButton; text: "Context menu"; onClicked: menu.openAt(40, 160) }
                LV.LabelButton { id: tipButton; text: "Tooltip"; onClicked: tip.open() }
                LV.LabelButton { text: "Popover"; onClicked: popover.open() }
            }
        }
        LV.PanelMaterial {
            width: parent.width; height: 190
            LV.Label { x: 24; y: 24; style: header2; text: "Panel · Glass 25" }
            LV.Label { x: 24; y: 58; style: body; text: "25% tint · 16 px blur" }
            LV.Label { x: 24; y: 100; style: caption; text: "8 central and edge gradients: 40% intense / 11% faint. Content stays opaque." }
        }
    }
    LV.ContextMenu {
        id: menu
        objectName: "galleryMaterialMenu"
        items: [{label: "Open", shortcut: "⌘ O"}, {label: "Duplicate", shortcut: "⌘ D"}, {label: "Show details"}]
    }
    LV.Tooltip {
        id: tip
        objectName: "galleryMaterialTooltip"
        target: tipButton
        automatic: false; delay: 0; timeout: -1
        text: "The tooltip inherits the application accent and Glass 25 material."
    }
    LV.Popover {
        id: popover
        objectName: "galleryMaterialPopover"
        x: Math.max(8, ((parent ? parent.width : 640) - width) / 2)
        y: 160
        width: 300; height: 168
        contentItem: Column {
            spacing: LV.Theme.gap12
            LV.Label { style: header2; text: "Workspace details" }
            LV.Label { width: parent.width; style: body; wrapMode: Text.WordWrap; text: "Any content can sit above the shared Glass 25 background." }
            LV.LabelButton { text: "Done"; tone: LV.AbstractButton.Primary; onClicked: popover.close() }
        }
    }
}

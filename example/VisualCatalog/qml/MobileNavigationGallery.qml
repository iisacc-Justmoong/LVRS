pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

ColumnLayout {
    id: root
    implicitWidth: 920
    spacing: 12
    property int searchCount: 0
    readonly property var destinations: [
        {iconName:"home-1", accessibleName:"Home"},
        {iconName:"nodesfolder", accessibleName:"Library"},
        {iconName:"toolwindownotifications", accessibleName:"Activity"},
        {iconName:"loggedInUser", accessibleName:"Profile"},
        {iconName:"generalsettings", accessibleName:"Settings"}
    ]
    LV.Label { style: header; text: "LVRS Mobile Navigation" }
    LV.Label { text: "iOS / Android · 1–5 tabs · optional Search · elastic selection" }
    Repeater {
        model: [1,2,3,4,5]
        RowLayout {
            id: pair
            required property int modelData
            Layout.fillWidth: true
            spacing: 24
            Repeater {
                model: [LV.MobileTab.IOS, LV.MobileTab.Android]
                LV.MobileNavigationBar {
                    required property int modelData
                    Layout.preferredWidth: Math.min(modelData === LV.MobileTab.IOS ? 402 : 412, (root.width - 24) / 2)
                    platformStyle: modelData
                    model: root.destinations.slice(0,pair.modelData)
                    onSearchRequested: root.searchCount++
                }
            }
        }
    }
    LV.Label { text: "Optional Search · omitted (default) · optional labels" }
    RowLayout {
        spacing: 24
        Repeater {
            model: [LV.MobileTab.IOS, LV.MobileTab.Android]
            LV.MobileNavigationBar {
                required property int modelData
                Layout.preferredWidth: Math.min(modelData === LV.MobileTab.IOS ? 402 : 412, (root.width - 24) / 2)
                platformStyle: modelData
                model: root.destinations.slice(0,3).map(item => ({iconName:item.iconName,text:item.accessibleName}))
            }
        }
    }
    LV.Label { text: "Optional Search · configured · tap destinations to move the shared selection surface" }
    RowLayout {
        spacing: 24
        Repeater {
            model: [LV.MobileTab.IOS, LV.MobileTab.Android]
            LV.MobileNavigationBar {
                required property int modelData
                Layout.preferredWidth: Math.min(modelData === LV.MobileTab.IOS ? 402 : 412, (root.width - 24) / 2)
                platformStyle: modelData
                model: root.destinations.slice(0,3).map(item => ({iconName:item.iconName,text:item.accessibleName}))
                search: ({text:"Search"})
                onSearchRequested: root.searchCount++
            }
        }
    }
    LV.Label { text: "320 px · scrollable tabs / explicit device radius 6" }
    LV.MobileNavigationBar {
        Layout.preferredWidth: 320
        model: root.destinations
        search: ({})
        deviceCornerRadius: 6
        onSearchRequested: root.searchCount++
    }
    LV.Label { text: "Search actions: " + root.searchCount }
}

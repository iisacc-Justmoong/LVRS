pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

Flickable {
    id: root
    implicitWidth: 960
    implicitHeight: 940
    contentWidth: width
    contentHeight: examples.implicitHeight
    clip: true
    readonly property var destinations: [
        {text: "Home", iconName: "home"}, {text: "Library", iconName: "nodesfolder"},
        {text: "Activity", iconName: "generalsettings", badgeDot: true},
        {text: "Profile", iconName: "loggedInUser"}, {text: "Search", iconName: "inputFieldSearch"}
    ]
    ColumnLayout {
        id: examples
        width: root.width
        spacing: 24
        MobileNavigationGallery { Layout.fillWidth: true }
        LV.Label { style: header; text: "Tabs · Desktop / iOS / Android" }
        Repeater {
            model: [LV.Tab.Underline, LV.Tab.Surface]
            ColumnLayout {
                id: states
                required property int modelData
                Layout.fillWidth: true
                LV.Label { text: states.modelData === LV.Tab.Underline ? "Underline" : "Surface" }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: ["Default", "Hover", "Pressed", "Selected", "Focus", "Disabled"]
                        LV.Tab {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            text: modelData
                            tabStyle: states.modelData
                            displayState: index + 1
                        }
                    }
                }
            }
        }
        LV.Label { text: "Content / Equal / Scrollable · arrows, Home/End, Enter/Space" }
        Repeater {
            model: [LV.TabBar.Content, LV.TabBar.Equal, LV.TabBar.Scrollable]
            LV.TabBar {
                required property int modelData
                Layout.fillWidth: true
                widthPolicy: modelData
                model: [{text: "Overview", iconName: "nodesfolder"}, {text: "Activity", badge: "8"}, {text: "Members"}, {text: "Settings"}]
            }
        }
        LV.Label { text: "iOS · Expanded / Separate Search / Minimized" }
        Repeater {
            model: [LV.MobileTabBar.Expanded, LV.MobileTabBar.Search, LV.MobileTabBar.Minimized]
            LV.MobileTabBar {
                required property int modelData
                Layout.preferredWidth: Math.min(402, root.width)
                model: root.destinations
                platformStyle: LV.MobileTab.IOS
                presentation: modelData
                onExpandRequested: presentation = LV.MobileTabBar.Expanded
            }
        }
        LV.Label { text: "Android · 3 / 4 / 5 destinations" }
        Repeater {
            model: [3, 4, 5]
            LV.MobileTabBar {
                required property int modelData
                Layout.preferredWidth: Math.min(412, root.width)
                platformStyle: LV.MobileTab.Android
                model: root.destinations.slice(0, modelData)
            }
        }
    }
}

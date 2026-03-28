pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    visible: true
    width: 920
    height: 920
    autoAttachRuntimeEvents: true
    title: "EventListener Examples"
    subtitle: "Real-time monitor enabled"
    navigationEnabled: false

    property int maxHistory: 100
    property int totalEvents: 0
    property string lastSection: "-"
    property string lastType: "-"
    property string lastDetail: "-"
    property var eventHistory: []
    readonly property rect exampleViewport: layoutSafeAreaBounds
    property bool viewportContractReady: exampleViewport.width > 0 && exampleViewport.height > 0
    property var eventCounters: ({
        "clicked": 0,
        "pressed": 0,
        "released": 0,
        "entered": 0,
        "exited": 0,
        "hoverChanged": 0,
        "keyPressed": 0,
        "keyReleased": 0
    })

    readonly property var sections: [
        {
            "title": "Basic Click",
            "subtitle": "Click trigger on a LVRS button",
            "source": "BasicClick.qml"
        },
        {
            "title": "Hover Card",
            "subtitle": "Hover state change tracking",
            "source": "HoverCard.qml"
        },
        {
            "title": "Press / Release",
            "subtitle": "Pressed and released trigger pair",
            "source": "PressRelease.qml"
        },
        {
            "title": "Right Click",
            "subtitle": "Right-button-only click trigger",
            "source": "RightClick.qml"
        },
        {
            "title": "Key Press",
            "subtitle": "Keyboard trigger example",
            "source": "KeyPress.qml"
        },
        {
            "title": "Multi Trigger",
            "subtitle": "Multiple listeners on one surface",
            "source": "MultiTrigger.qml"
        }
    ]

    function emptyCounters() {
        return {
            "clicked": 0,
            "pressed": 0,
            "released": 0,
            "entered": 0,
            "exited": 0,
            "hoverChanged": 0,
            "keyPressed": 0,
            "keyReleased": 0
        }
    }

    function bumpCounter(eventType) {
        const next = Object.assign({}, eventCounters)
        const currentValue = next[eventType] === undefined ? 0 : next[eventType]
        next[eventType] = currentValue + 1
        eventCounters = next
    }

    function pushEvent(sectionTitle, eventType, detail) {
        totalEvents += 1
        lastSection = sectionTitle
        lastType = eventType
        lastDetail = detail
        bumpCounter(eventType)

        const entry = {
            "id": totalEvents,
            "time": Qt.formatTime(new Date(), "hh:mm:ss"),
            "section": sectionTitle,
            "type": eventType,
            "detail": detail
        }

        const nextHistory = [entry].concat(eventHistory)
        if (nextHistory.length > maxHistory)
            nextHistory.length = maxHistory
        eventHistory = nextHistory
    }

    function clearMonitor() {
        totalEvents = 0
        lastSection = "-"
        lastType = "-"
        lastDetail = "-"
        eventHistory = []
        eventCounters = emptyCounters()
    }

    Item {
        x: root.exampleViewport.x + LV.Theme.gap24
        y: root.exampleViewport.y + LV.Theme.gap24
        width: Math.max(1, root.exampleViewport.width - (LV.Theme.gap24 * 2))
        height: Math.max(1, root.exampleViewport.height - (LV.Theme.gap24 * 2))

        LV.AppCard {
            id: monitorCard
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            title: "Event Monitor"
            subtitle: "Sample interactions are logged here in real time"

            Column {
                width: monitorCard.width - (monitorCard.cardPadding * 2)
                spacing: LV.Theme.gap10

                LV.Label {
                    width: parent.width
                    style: body
                    wrapMode: Text.WordWrap
                    text: "Total: " + root.totalEvents
                          + " | Last Trigger: " + root.lastType
                          + " | Last Section: " + root.lastSection
                }

                LV.Label {
                    width: parent.width
                    style: description
                    wrapMode: Text.WordWrap
                    text: "Last Detail: " + root.lastDetail
                }

                Row {
                    spacing: LV.Theme.gap8

                    LV.LabelButton {
                        text: "Clear Monitor"
                        tone: LV.AbstractButton.Default
                        onClicked: root.clearMonitor()
                    }
                }

                Row {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    LV.Label {
                        style: caption
                        text: "clicked: " + (root.eventCounters.clicked || 0)
                    }

                    LV.Label {
                        style: caption
                        text: "pressed: " + (root.eventCounters.pressed || 0)
                    }

                    LV.Label {
                        style: caption
                        text: "released: " + (root.eventCounters.released || 0)
                    }

                    LV.Label {
                        style: caption
                        text: "hoverChanged: " + (root.eventCounters.hoverChanged || 0)
                    }
                }

                Row {
                    width: parent.width
                    spacing: LV.Theme.gap8

                    LV.Label {
                        style: caption
                        text: "entered: " + (root.eventCounters.entered || 0)
                    }

                    LV.Label {
                        style: caption
                        text: "exited: " + (root.eventCounters.exited || 0)
                    }

                    LV.Label {
                        style: caption
                        text: "keyPressed: " + (root.eventCounters.keyPressed || 0)
                    }

                    LV.Label {
                        style: caption
                        text: "keyReleased: " + (root.eventCounters.keyReleased || 0)
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 210
                    radius: LV.Theme.radiusSm
                    color: LV.Theme.subSurface
                    clip: true

                    Flickable {
                        id: monitorScroll
                        anchors.fill: parent
                        anchors.margins: LV.Theme.gap6
                        clip: true
                        contentWidth: width
                        contentHeight: monitorColumn.implicitHeight

                        Column {
                            id: monitorColumn
                            width: monitorScroll.width
                            spacing: LV.Theme.gap4

                            LV.Label {
                                width: parent.width
                                visible: root.eventHistory.length === 0
                                style: body
                                text: "No events yet. Interact with the samples below."
                            }

                            Repeater {
                                model: root.eventHistory

                                delegate: Rectangle {
                                    id: historyRow
                                    required property var modelData
                                    width: monitorColumn.width
                                    height: logRow.implicitHeight + (LV.Theme.gap4 * 2)
                                    radius: LV.Theme.radiusSm
                                    color: LV.Theme.surfaceSolid

                                    Row {
                                        id: logRow
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.margins: LV.Theme.gap4
                                        spacing: LV.Theme.gap6

                                        LV.Label {
                                            width: 62
                                            style: caption
                                            text: historyRow.modelData.time
                                        }

                                        LV.Label {
                                            width: 230
                                            style: description
                                            elide: Text.ElideRight
                                            text: historyRow.modelData.section + " / " + historyRow.modelData.type
                                        }

                                        LV.Label {
                                            width: Math.max(90, logRow.width - 62 - 230 - (LV.Theme.gap6 * 2))
                                            style: body
                                            elide: Text.ElideRight
                                            text: historyRow.modelData.detail
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Flickable {
            id: sampleScroll
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: monitorCard.bottom
            anchors.topMargin: LV.Theme.gap12
            anchors.bottom: parent.bottom
            clip: true
            contentWidth: width
            contentHeight: sectionColumn.implicitHeight + LV.Theme.gap24

            Column {
                id: sectionColumn
                x: 0
                y: 0
                width: sampleScroll.width
                spacing: LV.Theme.gap12

                Repeater {
                    model: root.sections

                    delegate: LV.AppCard {
                        id: sectionCard
                        required property var modelData
                        width: sectionColumn.width
                        title: modelData.title
                        subtitle: modelData.subtitle

                        Loader {
                            id: sampleLoader
                            width: sectionCard.width - (sectionCard.cardPadding * 2)
                            source: sectionCard.modelData.source
                        }

                        Connections {
                            target: sampleLoader.item
                            ignoreUnknownSignals: true

                            function onEventRaised(triggerName, detail) {
                                root.pushEvent(sectionCard.modelData.title, triggerName, detail)
                            }
                        }
                    }
                }
            }
        }
    }
}

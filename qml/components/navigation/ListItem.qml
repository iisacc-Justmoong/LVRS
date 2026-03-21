import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    property string label: ""
    // Legacy compatibility fields kept for existing callers.
    property string detail: ""
    property string iconName: ""
    property bool selected: false
    property bool showChevron: false
    property bool inputable: false
    property string inputResult: control.label

    property int rowHorizontalPadding: Theme.gap4
    property int rowVerticalPadding: Theme.gap2
    property int separatorHeight: Theme.scaleMetric(1)
    property int separatorTopSpacing: Theme.scaleMetric(1)
    property int minItemWidth: Theme.scaleMetric(170)
    property color listBackgroundColor: "transparent"
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5

    signal inputEdited(string text)
    signal inputSubmitted(string text)

    function normalizedText(value) {
        if (value === undefined || value === null)
            return ""
        return String(value)
    }

    function applyInputResult(value) {
        const normalized = normalizedText(value)
        if (control.label !== normalized)
            control.label = normalized
        if (control.inputResult !== normalized)
            control.inputResult = normalized
        return normalized
    }

    tone: AbstractButton.Borderless
    horizontalPadding: control.rowHorizontalPadding
    verticalPadding: control.rowVerticalPadding
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm

    implicitHeight: contentColumn.implicitHeight + topPadding + bottomPadding
    implicitWidth: Math.max(control.minItemWidth, contentColumn.implicitWidth + leftPadding + rightPadding)

    backgroundColor: control.selected ? Theme.accentOverlay : control.listBackgroundColor
    backgroundColorHover: control.selected ? Theme.accentOverlay : control.listBackgroundColor
    backgroundColorPressed: control.selected ? Theme.accentOverlay : control.listBackgroundColor
    backgroundColorDisabled: listBackgroundColor
    textColor: Theme.bodyColor
    textColorDisabled: Theme.disabledColor

    contentItem: Column {
        id: contentColumn
        spacing: control.separatorTopSpacing

        Item {
            width: parent.width
            implicitHeight: labelItem.implicitHeight

            Label {
                id: labelItem
                anchors.fill: parent
                style: body
                text: control.inputResult
                color: control.effectiveEnabled ? Theme.bodyColor : Theme.disabledColor
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                visible: !control.inputable
            }

            Loader {
                id: overlayInputLoader
                anchors.fill: parent
                active: control.inputable
                visible: control.inputable
                sourceComponent: Component {
                    InputField {
                        enabled: control.effectiveEnabled
                        backgroundColor: "transparent"
                        backgroundColorHover: "transparent"
                        backgroundColorPressed: "transparent"
                        backgroundColorFocused: "transparent"
                        backgroundColorDisabled: "transparent"
                        placeholderText: ""
                        clearButtonVisible: false
                        fieldMinHeight: Theme.scaleMetric(16)
                        centeredTextHeight: Theme.scaleTextMetric(16)
                        insetHorizontal: 0
                        insetVertical: 0
                        sideSpacing: 0
                        cornerRadius: 0
                        textColor: Theme.bodyColor
                        textColorDisabled: Theme.disabledColor
                        placeholderColor: Theme.disabledColor
                        placeholderColorDisabled: Theme.disabledColor

                        onTextEdited: {
                            const value = control.applyInputResult(text)
                            control.inputEdited(value)
                        }
                        onAccepted: control.inputSubmitted(control.applyInputResult(text))
                    }
                }

                onLoaded: {
                    if (item)
                        item.text = control.inputResult
                }
            }
        }

        Rectangle {
            width: parent.width
            height: control.separatorHeight
            color: control.separatorColor
            opacity: control.separatorOpacity
        }
    }

    onLabelChanged: {
        const normalized = control.normalizedText(control.label)
        if (control.inputResult !== normalized)
            control.inputResult = normalized
        if (overlayInputLoader.status === Loader.Ready
            && overlayInputLoader.item
            && !overlayInputLoader.item.activeFocus
            && overlayInputLoader.item.text !== normalized) {
            overlayInputLoader.item.text = normalized
        }
    }
    onInputableChanged: {
        if (control.inputable
            && overlayInputLoader.status === Loader.Ready
            && overlayInputLoader.item
            && !overlayInputLoader.item.activeFocus) {
            overlayInputLoader.item.text = control.inputResult
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ListItem { label: "Label"; inputable: true }

import QtQuick
import LVRS 1.0

Item {
    id: control

    property var itemData: null
    property string text: "Text"
    property int cellHeight: 24
    property int contentSpacing: Theme.gap8
    property color dividerColor: Theme.panelBackground03
    property color textColor: Theme.bodyColor
    property bool showDivider: true
    property bool clipContent: true
    property int textStyle: body
    property bool inputable: false
    property string inputResult: control.resolvedText
    property bool _hasInputOverride: false

    signal inputEdited(string text)
    signal inputSubmitted(string text)

    readonly property int title: 0
    readonly property int title2: 1
    readonly property int header: 2
    readonly property int header2: 3
    readonly property int body: 4
    readonly property int description: 5
    readonly property int caption: 6
    readonly property int disabled: 7

    readonly property string resolvedText: {
        const entry = control.itemData
        if (entry && typeof entry === "object") {
            if (entry.label !== undefined && entry.label !== null)
                return String(entry.label)
            if (entry.text !== undefined && entry.text !== null)
                return String(entry.text)
            if (entry.title !== undefined && entry.title !== null)
                return String(entry.title)
        }
        return control.text
    }
    readonly property int resolvedCellHeight: {
        const value = control.itemValue("cellHeight", control.cellHeight)
        const parsed = Number(value)
        return isNaN(parsed) || parsed <= 0 ? control.cellHeight : parsed
    }
    readonly property int resolvedContentSpacing: {
        const value = control.itemValue("contentSpacing", control.contentSpacing)
        const parsed = Number(value)
        return isNaN(parsed) || parsed < 0 ? control.contentSpacing : parsed
    }
    readonly property color resolvedDividerColor: control.itemValue("dividerColor", control.dividerColor)
    readonly property color resolvedTextColor: control.itemValue("textColor", control.textColor)
    readonly property bool resolvedShowDivider: {
        const value = control.itemValue("showDivider", control.showDivider)
        return !!value
    }
    readonly property bool resolvedClipContent: {
        const value = control.itemValue("clipContent", control.clipContent)
        return !!value
    }
    readonly property int resolvedTextStyle: {
        const value = control.itemValue("textStyle", control.textStyle)
        const parsed = Number(value)
        return isNaN(parsed) ? control.textStyle : parsed
    }

    function itemValue(key, fallbackValue) {
        const entry = control.itemData
        if (!entry || typeof entry !== "object")
            return fallbackValue
        if (Object.prototype.hasOwnProperty.call(entry, key))
            return entry[key]
        return fallbackValue
    }

    function normalizedText(value) {
        if (value === undefined || value === null)
            return ""
        return String(value)
    }

    function applyInputResult(value) {
        const normalized = normalizedText(value)
        control._hasInputOverride = true
        if (control.text !== normalized)
            control.text = normalized
        if (control.inputResult !== normalized)
            control.inputResult = normalized
        return normalized
    }

    implicitWidth: 234
    implicitHeight: resolvedCellHeight
    clip: resolvedClipContent

    Rectangle {
        id: dividerNode
        visible: control.resolvedShowDivider
        width: Theme.strokeThin
        height: parent.height
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: control.resolvedDividerColor
        antialiasing: false
    }

    Item {
        anchors.left: dividerNode.visible ? dividerNode.right : parent.left
        anchors.leftMargin: dividerNode.visible ? control.resolvedContentSpacing : 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: labelNode.implicitHeight

        Label {
            id: labelNode
            anchors.fill: parent
            style: control.resolvedTextStyle
            text: control.inputResult
            color: control.resolvedTextColor
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
                    enabled: control.enabled
                    backgroundColor: "transparent"
                    backgroundColorHover: "transparent"
                    backgroundColorPressed: "transparent"
                    backgroundColorFocused: "transparent"
                    backgroundColorDisabled: "transparent"
                    placeholderText: ""
                    clearButtonVisible: false
                    fieldMinHeight: 16
                    centeredTextHeight: 16
                    insetHorizontal: 0
                    insetVertical: 0
                    sideSpacing: 0
                    cornerRadius: 0
                    textColor: control.resolvedTextColor
                    textColorDisabled: control.resolvedTextColor
                    placeholderColor: control.resolvedTextColor
                    placeholderColorDisabled: control.resolvedTextColor

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

    onResolvedTextChanged: {
        if (control._hasInputOverride)
            return
        const normalized = control.normalizedText(control.resolvedText)
        if (control.inputResult !== normalized)
            control.inputResult = normalized
        if (overlayInputLoader.status === Loader.Ready
            && overlayInputLoader.item
            && !overlayInputLoader.item.activeFocus
            && overlayInputLoader.item.text !== normalized) {
            overlayInputLoader.item.text = normalized
        }
    }
    onItemDataChanged: {
        control._hasInputOverride = false
        const normalized = control.normalizedText(control.resolvedText)
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
// LV.TableCellItem { itemData: ({ text: "Text" }); inputable: true }

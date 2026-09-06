pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Column {
    id: root
    required property var listItem
    spacing: listItem.sectionSpacing
    readonly property string variant: listItem.variantName

    function is() {
        return Array.prototype.indexOf.call(arguments, variant) >= 0;
    }

    component BodyText: Label {
        style: body
        color: root.listItem.effectiveEnabled ? Theme.bodyColor : Theme.disabledColor
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: Theme.textBodyLineHeight
        Layout.maximumHeight: Theme.textBodyLineHeight
    }

    component CaptionText: Label {
        property int lines: 1
        style: caption
        color: root.listItem.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
        elide: Text.ElideRight
        wrapMode: lines > 1 ? Text.Wrap : Text.NoWrap
        maximumLineCount: lines
        verticalAlignment: Text.AlignTop
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: Theme.textCaptionLineHeight * lines
        Layout.maximumHeight: Theme.textCaptionLineHeight * lines
    }

    component Glyph: Image {
        property string iconName: ""
        property url iconSource: ""
        implicitWidth: root.listItem.iconSize
        implicitHeight: root.listItem.iconSize
        source: RenderQuality.resolveTextureSource(iconSource.toString().length > 0 ? iconSource : root.listItem.resolvedAsset(iconName))
        sourceSize.width: root.listItem.iconSourceSize
        sourceSize.height: root.listItem.iconSourceSize
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: RenderQuality.mipmapEnabled
        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight
    }

    component Slot: Loader {
        property bool shown: true
        property var config: ({})
        property Component fallback: null
        sourceComponent: root.listItem.configValue(config, "component", fallback)
        active: shown
        visible: shown
        Layout.preferredWidth: implicitWidth
        Layout.minimumWidth: implicitWidth
        Layout.maximumWidth: implicitWidth
        Layout.preferredHeight: implicitHeight
        Layout.alignment: Qt.AlignVCenter
        onLoaded: {
            if (item && "listItem" in item)
                item.listItem = Qt.binding(function () {
                    return root.listItem;
                });
        }
    }

    component ContentText: ColumnLayout {
        property int lines: 1
        spacing: root.listItem.textSpacing
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        BodyText {
            objectName: "listItem_label"
            text: root.listItem.label
        }
        CaptionText {
            objectName: "listItem_description"
            lines: parent.lines
            text: root.listItem.description
            visible: root.listItem.showDescription
        }
    }

    component ActionSlot: Slot {
        id: actionSlot
        property string actionName: "primary"
        config: actionName === "primary" ? root.listItem.primaryAction : root.listItem.secondaryAction
        shown: actionName === "primary" ? root.listItem.showPrimaryAction : root.listItem.showSecondaryAction
        fallback: Component {
            PushButton {
                objectName: "listItem_" + actionSlot.actionName + "Action"
                implicitWidth: root.listItem.actionWidth
                text: root.listItem.configValue(actionSlot.config, "text", root.listItem.actionText(actionSlot.actionName))
                tone: root.listItem.configValue(actionSlot.config, "tone", actionSlot.actionName === "primary" ? AbstractButton.Primary : AbstractButton.Default)
                iconMode: root.listItem.configValue(actionSlot.config, "iconMode", false)
                iconName: root.listItem.configValue(actionSlot.config, "iconName", "")
                enabled: root.listItem.effectiveEnabled && root.listItem.configValue(actionSlot.config, "enabled", true)
                onClicked: root.listItem.triggerAction(actionSlot.actionName)
            }
        }
    }

    component MoreSlot: Slot {
        config: root.listItem.moreMenu
        shown: root.listItem.showMoreMenu
        fallback: Component {
            DropdownButton {
                id: moreButton
                objectName: "listItem_moreMenu"
                implicitWidth: root.listItem.actionWidth
                text: root.listItem.configValue(root.listItem.moreMenu, "text", "More")
                tone: root.listItem.configValue(root.listItem.moreMenu, "tone", AbstractButton.Default)
                iconMode: root.listItem.configValue(root.listItem.moreMenu, "iconMode", false)
                iconName: root.listItem.configValue(root.listItem.moreMenu, "iconName", "")
                enabled: root.listItem.effectiveEnabled && root.listItem.configValue(root.listItem.moreMenu, "enabled", true)
                onClicked: {
                    if (root.listItem.triggerAction("menu") && actionMenu.entryCount > 0)
                        actionMenu.openFor(moreButton, 0, moreButton.height);
                }
                ContextMenu {
                    id: actionMenu
                    objectName: "listItem_actionMenu"
                    items: root.listItem.configValue(root.listItem.moreMenu, "items", [])
                    onItemTriggered: function (index, item) {
                        root.listItem.actionTriggered("menuItem", {
                            index: index,
                            item: item
                        });
                    }
                }
            }
        }
    }

    component ToggleSlot: Slot {
        shown: root.listItem.showToggle
        fallback: Component {
            ToggleSwitch {
                objectName: "listItem_toggle"
                checked: root.listItem.checked
                enabled: root.listItem.effectiveEnabled && root.listItem.toggleEnabled
                onToggled: root.listItem.editValue("checked", checked)
            }
        }
    }

    component SelectionSlot: Slot {
        shown: root.listItem.showSelection
        fallback: Component {
            CheckBox {
                objectName: "listItem_selection"
                text: ""
                checked: root.listItem.checked
                enabled: root.listItem.effectiveEnabled && root.listItem.selectionEnabled
                onToggled: root.listItem.editValue("checked", checked)
            }
        }
    }

    component SelectorSlot: Slot {
        id: selectorSlot
        property bool unit: false
        shown: unit ? root.listItem.showUnitSelector : root.listItem.showSelector
        config: unit ? root.listItem.unitSelector : root.listItem.selector
        fallback: Component {
            ListItemSelector {
                listItem: root.listItem
                config: selectorSlot.config
                unit: selectorSlot.unit
            }
        }
    }

    component QuantityControl: FocusScope {
        objectName: "listItem_quantityControl"
        implicitWidth: root.listItem.previewSize + (root.listItem.showStepper ? root.listItem.textSpacing + root.listItem.iconSize : 0)
        implicitHeight: root.listItem.iconSize
        Layout.preferredWidth: implicitWidth
        Layout.minimumWidth: implicitWidth
        Layout.maximumWidth: implicitWidth
        Layout.preferredHeight: implicitHeight
        activeFocusOnTab: root.listItem.effectiveEnabled && root.listItem.showStepper && root.listItem.configValue(root.listItem.stepper, "enabled", true)
        Accessible.role: Accessible.SpinBox
        Accessible.name: root.listItem.quantityLabel
        Keys.onUpPressed: root.listItem.stepQuantity(1)
        Keys.onDownPressed: root.listItem.stepQuantity(-1)
        Label {
            objectName: "listItem_quantity"
            width: root.listItem.previewSize
            height: Theme.textBodyLineHeight
            style: body
            text: String(root.listItem.quantity)
            color: root.listItem.effectiveEnabled ? Theme.bodyColor : Theme.disabledColor
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
        }
        Stepper {
            objectName: "listItem_stepper"
            x: root.listItem.previewSize + root.listItem.textSpacing
            visible: root.listItem.showStepper
            enabled: root.listItem.effectiveEnabled && root.listItem.configValue(root.listItem.stepper, "enabled", true)
            tone: root.listItem.configValue(root.listItem.stepper, "tone", AbstractButton.Primary)
            arrow: root.listItem.configValue(root.listItem.stepper, "arrow", Stepper.UpDown)
            onPressed: parent.forceActiveFocus(Qt.MouseFocusReason)
            onStepped: function (direction) {
                root.listItem.stepQuantity(direction);
            }
        }
    }

    component InputSlot: Slot {
        id: inputSlot
        property int field: 1
        shown: field === 1 ? root.listItem.showInput1 : root.listItem.showInput2
        config: field === 1 ? root.listItem.input1 : root.listItem.input2
        fallback: Component {
            InputField {
                objectName: "listItem_input" + inputSlot.field
                implicitWidth: root.listItem.inputWidth
                text: inputSlot.field === 1 ? root.listItem.inputText1 : root.listItem.inputText2
                placeholderText: root.listItem.configValue(inputSlot.config, "placeholderText", "")
                readOnly: root.listItem.configValue(inputSlot.config, "readOnly", false)
                enabled: root.listItem.effectiveEnabled && root.listItem.configValue(inputSlot.config, "enabled", true)
                style: root.listItem.configValue(inputSlot.config, "style", roundedStyle)
                clearButtonVisible: root.listItem.configValue(inputSlot.config, "clearButtonVisible", true)
                maximumLength: root.listItem.configValue(inputSlot.config, "maximumLength", 32767)
                validator: root.listItem.configValue(inputSlot.config, "validator", null)
                onTextEdited: function (text) {
                    root.listItem.editValue("inputText" + inputSlot.field, text);
                }
                onAccepted: function (text) {
                    root.listItem.actionTriggered("inputSubmitted", {
                        field: "inputText" + inputSlot.field,
                        text: text
                    });
                }
            }
        }
    }

    component QuantitySlot: Slot {
        fallback: Component {
            QuantityControl {}
        }
    }

    component SegmentsSlot: Slot {
        shown: root.listItem.showSegments
        fallback: Component {
            LabelSegmentedControl {
                objectName: "listItem_segments"
                forceBorderlessTone: false
                cornerRadius: Theme.scaleMetric(12)
                Repeater {
                    model: root.listItem.segments
                    delegate: PushButton {
                        required property int index
                        required property var modelData
                        objectName: "listItem_segment_" + index
                        implicitWidth: Theme.scaleMetric(56)
                        text: root.listItem.optionText(modelData, "")
                        enabled: root.listItem.effectiveEnabled && root.listItem.configValue(modelData, "enabled", true)
                        tone: index === root.listItem.segmentIndex ? AbstractButton.Primary : AbstractButton.Borderless
                        onClicked: {
                            root.listItem.editValue("segmentIndex", index);
                            const method = root.listItem.configValue(modelData, "method", null);
                            if (method)
                                root.listItem.invokeMethod(method, root.listItem.createMethodEvent("segment"));
                        }
                    }
                }
            }
        }
    }

    component PreviewSlot: Slot {
        shown: root.listItem.showPreview
        config: ({
                component: root.listItem.previewComponent
            })
        fallback: Component {
            Rectangle {
                objectName: "listItem_preview"
                implicitWidth: root.listItem.previewSize
                implicitHeight: root.listItem.previewSize
                color: Theme.panelBackground06
                radius: Theme.scaleMetric(6)
                clip: true
                Glyph {
                    anchors.centerIn: parent
                    width: root.listItem.previewSource.toString().length ? parent.width : root.listItem.iconSize
                    height: width
                    iconName: root.listItem.previewIconName
                    iconSource: root.listItem.previewSource
                    sourceSize.width: Math.ceil(width * root.listItem.iconHiDpiScale * root.listItem.iconSupersampleScale)
                    sourceSize.height: sourceSize.width
                }
            }
        }
    }

    // The header is shared by single rows and compound presets.
    RowLayout {
        objectName: "listItem_header"
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        Slot {
            shown: root.is("Mini", "Navigation", "Toggle", "Action", "ActionGroup", "Stepper", "Select", "InlineEdit", "DetailActions", "DetailSettings") && root.listItem.showLeadingIcon
            config: ({
                    component: root.listItem.leadingComponent
                })
            fallback: Component {
                Glyph {
                    objectName: "listItem_leadingIcon"
                    iconName: root.listItem.iconName
                    iconSource: root.listItem.iconSource
                }
            }
        }
        SelectionSlot {
            shown: root.is("Checkable", "Task") && root.listItem.showSelection
        }
        PreviewSlot {
            shown: root.is("Resource", "Media") && root.listItem.showPreview
        }
        ContentText {
            lines: root.is("DetailActions", "Resource") ? 2 : 1
            visible: !root.is("Task")
        }
        BodyText {
            objectName: "listItem_taskLabel"
            text: root.listItem.label
            visible: root.is("Task")
        }
        Glyph {
            objectName: "listItem_bookmark"
            iconName: root.listItem.bookmarkIconName
            visible: root.is("DetailActions") && root.listItem.showBookmark
        }
        ColumnLayout {
            visible: root.is("Media")
            spacing: root.listItem.textSpacing
            Layout.preferredWidth: root.listItem.actionWidth
            Layout.minimumWidth: root.listItem.actionWidth
            Layout.maximumWidth: root.listItem.actionWidth
            CaptionText {
                objectName: "listItem_duration"
                text: root.listItem.durationText
            }
            CaptionText {
                objectName: "listItem_mediaDetail"
                text: root.listItem.mediaDetail
            }
        }
        Slot {
            shown: root.listItem.trailingComponent !== null && !root.listItem.isCompound
            config: ({
                    component: root.listItem.trailingComponent
                })
        }
        RowLayout {
            visible: root.is("Navigation", "Checkable") && root.listItem.trailingComponent === null
            spacing: root.listItem.textSpacing
            CaptionText {
                objectName: "listItem_value"
                text: root.listItem.value
                horizontalAlignment: Text.AlignRight
                visible: root.listItem.showValue
                Layout.preferredWidth: root.listItem.actionWidth
                Layout.minimumWidth: root.listItem.actionWidth
                Layout.maximumWidth: root.listItem.actionWidth
            }
            Glyph {
                objectName: "listItem_trailingIcon"
                iconName: root.listItem.trailingIconName
                visible: root.is("Navigation") && root.listItem.showTrailingIcon
            }
        }
        ToggleSlot {
            shown: root.is("Toggle", "DetailSettings") && root.listItem.showToggle && (root.listItem.isCompound || root.listItem.trailingComponent === null)
        }
        QuantitySlot {
            shown: root.is("Stepper") && root.listItem.trailingComponent === null
        }
        SelectorSlot {
            shown: root.is("Select", "DetailQuantity", "Task", "Form") && root.listItem.showSelector && (root.listItem.isCompound || root.listItem.trailingComponent === null)
        }
        InputSlot {
            shown: root.is("InlineEdit") && root.listItem.showInput1 && root.listItem.trailingComponent === null
        }
        ActionSlot {
            shown: root.is("Action", "ActionGroup", "InlineEdit") && root.listItem.showPrimaryAction && root.listItem.trailingComponent === null
        }
        ActionSlot {
            actionName: "secondary"
            shown: root.is("ActionGroup") && root.listItem.showSecondaryAction && root.listItem.trailingComponent === null
        }
        MoreSlot {
            shown: root.is("ActionGroup", "Resource") && root.listItem.showMoreMenu && (root.listItem.isCompound || root.listItem.trailingComponent === null)
        }
    }

    CaptionText {
        objectName: "listItem_taskDescription"
        visible: root.is("Task") && root.listItem.showDescription
        width: parent.width
        height: Theme.textCaptionLineHeight * 2
        lines: 2
        text: root.listItem.description
    }

    RowLayout {
        objectName: "listItem_metadata"
        visible: root.is("DetailActions", "Resource", "Task") && root.listItem.showMetadata
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        CaptionText {
            objectName: "listItem_date"
            text: root.listItem.dateText
        }
        CaptionText {
            objectName: "listItem_metadata1"
            text: root.listItem.metadata1
        }
        CaptionText {
            objectName: "listItem_metadata2"
            text: root.listItem.metadata2
        }
    }

    RowLayout {
        objectName: "listItem_quantityRow"
        visible: root.is("DetailQuantity")
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        CaptionText {
            text: root.listItem.quantityLabel
        }
        QuantitySlot {
            shown: root.is("DetailQuantity")
        }
        SelectorSlot {
            unit: true
            shown: root.is("DetailQuantity") && root.listItem.showUnitSelector
        }
    }

    RowLayout {
        objectName: "listItem_modeRow"
        visible: root.is("DetailSettings")
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        CaptionText {
            text: root.listItem.modeLabel
        }
        SelectorSlot {
            shown: root.is("DetailSettings") && root.listItem.showSelector
        }
    }

    RowLayout {
        objectName: "listItem_scopeRow"
        visible: root.is("DetailSettings")
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        CaptionText {
            text: root.listItem.scopeLabel
        }
        SegmentsSlot {
            shown: root.is("DetailSettings") && root.listItem.showSegments
        }
    }

    RowLayout {
        objectName: "listItem_field1Row"
        visible: root.is("Form")
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        CaptionText {
            text: root.listItem.fieldLabel1
        }
        InputSlot {
            shown: root.is("Form") && root.listItem.showInput1
        }
    }

    RowLayout {
        objectName: "listItem_field2Row"
        visible: root.is("Form")
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        CaptionText {
            text: root.listItem.fieldLabel2
        }
        InputSlot {
            field: 2
            shown: root.is("Form") && root.listItem.showInput2
        }
    }

    RowLayout {
        objectName: "listItem_actions"
        visible: root.listItem.isCompound
        width: parent.width
        height: implicitHeight
        spacing: root.listItem.contentSpacing
        SegmentsSlot {
            shown: root.is("Media") && root.listItem.showSegments
        }
        ToggleSlot {
            shown: root.is("DetailQuantity", "Form") && root.listItem.showToggle
        }
        CaptionText {
            objectName: "listItem_status"
            text: root.is("DetailQuantity") ? root.listItem.optionLabel : root.is("Task") ? root.listItem.quantityLabel : root.listItem.statusText
        }
        QuantitySlot {
            shown: root.is("Task")
        }
        ActionSlot {
            shown: root.is("DetailActions") && root.listItem.showPrimaryAction
        }
        ActionSlot {
            actionName: "secondary"
            shown: root.listItem.isCompound && !root.is("DetailQuantity") && root.listItem.showSecondaryAction
        }
        ActionSlot {
            shown: root.listItem.isCompound && !root.is("DetailActions") && root.listItem.showPrimaryAction
        }
        MoreSlot {
            shown: root.is("DetailActions") && root.listItem.showMoreMenu
        }
    }

    Slot {
        shown: root.listItem.footerComponent !== null
        config: ({
                component: root.listItem.footerComponent
            })
        width: parent.width
    }
}

// Internal layout for ListItem's standard and compound variants.

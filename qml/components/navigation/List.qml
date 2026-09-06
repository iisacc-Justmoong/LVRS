pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property var model: null
    property var items: ["Label", "Label", "Label", "Label", "Label", "Label"]
    property int modelColumn: 0
    property string labelRole: "label"
    property string textRole: "text"
    property string titleRole: "title"
    property string iconRole: "iconName"
    property string enabledRole: "enabled"
    property string selectedRole: "selected"
    property string typeRole: "type"
    property string descriptionRole: "description"
    property int defaultItemType: ListItem.Mini
    property int selectedIndex: -1
    property bool interactive: true
    property bool scrollable: true

    // Optional top toolbar support for existing callers.
    property bool toolbarVisible: false
    property string toolbarIcon1: ""
    property string toolbarIcon2: ""
    property string toolbarIcon3: ""

    property bool footerVisible: true
    property var footerButton1: ({
            type: "icon",
            iconName: "addFile"
        })
    property var footerButton2: ({
            type: "icon",
            iconName: "generaldelete"
        })
    property var footerButton3: ({
            type: "menu",
            iconName: "settings"
        })

    property int listWidth: preferredListWidth()
    property int minimumListHeight: Theme.scaleMetric(140)
    property int itemHeight: Theme.iconSm + (Theme.gap2 * 2)
    property int itemSpacing: Theme.gapNone
    property int itemLabelLeftPadding: Theme.gap4
    property string defaultItemIconName: "nodesfolder"
    property bool expandToContent: false
    property color backgroundColor: Theme.panelBackground03
    property color selectedRowColor: Theme.primary
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5
    property Component itemDelegate: defaultItemDelegate

    signal itemTriggered(int index, var item)
    signal itemEdited(int index, var item, string field, var value)
    signal itemActionTriggered(int index, var item, string action, var payload)
    signal toolbarIconTriggered(int index, string source)
    signal footerButtonTriggered(int index, var config)

    readonly property bool usingModel: model !== undefined && model !== null
    readonly property var sourceModel: usingModel ? model : items
    readonly property int entryCount: listModelSource.count
    readonly property var itemDelegateItems: buildItemDelegateItems()

    Component {
        id: defaultItemDelegate

        ListItem {
            id: rowButton
            objectName: "list_defaultDelegate_" + index
            property var modelData: ({})
            property int index: modelData.index === undefined ? -1 : modelData.index
            readonly property var entry: modelData.entry
            readonly property bool rowSelected: modelData.selected === true
            readonly property bool rowEnabled: modelData.enabled === true

            width: parent ? parent.width : control.listWidth
            type: modelData.type === undefined ? control.defaultItemType : modelData.type
            label: rowButton.modelData.label || ""
            iconName: rowButton.modelData.iconName || control.defaultItemIconName
            selected: rowSelected
            enabled: rowEnabled
            rowHorizontalPadding: control.itemLabelLeftPadding
            rowVerticalPadding: Theme.gap2
            miniItemWidth: control.listWidth
            minItemWidth: control.listWidth
            listBackgroundColor: "transparent"
            selectedBackgroundColor: control.selectedRowColor
            separatorColor: control.separatorColor
            separatorOpacity: control.separatorOpacity
            separatorVisible: false

            function rowValue(name, fallback) {
                return configValue(modelData.properties, name, fallback);
            }

            description: rowButton.modelData.description || ""
            detail: rowValue("detail", "asasdsadasasdsadasasd Maxinum lines: 2 lines")
            value: rowValue("value", "Value")
            dateText: rowValue("dateText", "YYYY-MM-dd")
            metadata1: rowValue("metadata1", "Metadata")
            metadata2: rowValue("metadata2", "Status")
            statusText: rowValue("statusText", "Draft")
            folderLabel1: rowValue("folderLabel1", "Only")
            folderLabel2: rowValue("folderLabel2", "1 Line")
            tagLabel1: rowValue("tagLabel1", "Only")
            tagLabel2: rowValue("tagLabel2", "1 Line")
            quantityLabel: rowValue("quantityLabel", "Quantity label")
            optionLabel: rowValue("optionLabel", "Apply to all")
            modeLabel: rowValue("modeLabel", "Mode label")
            scopeLabel: rowValue("scopeLabel", "Scope label")
            durationText: rowValue("durationText", "03:42")
            mediaDetail: rowValue("mediaDetail", "WAV · 48 kHz")
            fieldLabel1: rowValue("fieldLabel1", "Field 1 label")
            fieldLabel2: rowValue("fieldLabel2", "Field 2 label")
            previewSource: rowValue("previewSource", "")
            previewIconName: rowValue("previewIconName", "nodesfolder")
            iconSource: rowValue("iconSource", "")
            trailingIconName: rowValue("trailingIconName", "generalchevronRight")
            bookmarkIconName: rowValue("bookmarkIconName", "bookmarksbookmark")
            folderIconName: rowValue("folderIconName", "folder@14x14")
            tagIconName: rowValue("tagIconName", "vcscurrentBranch")
            primaryAction: rowValue("primaryAction", ({}))
            secondaryAction: rowValue("secondaryAction", ({}))
            moreMenu: rowValue("moreMenu", ({}))
            selector: rowValue("selector", ({}))
            unitSelector: rowValue("unitSelector", ({}))
            stepper: rowValue("stepper", ({}))
            input1: rowValue("input1", ({}))
            input2: rowValue("input2", ({}))
            segments: rowValue("segments", ["Original", "Edited"])
            minimumQuantity: rowValue("minimumQuantity", 0)
            maximumQuantity: rowValue("maximumQuantity", Number.POSITIVE_INFINITY)
            stepSize: rowValue("stepSize", 1)
            toggleEnabled: rowValue("toggleEnabled", true)
            selectionEnabled: rowValue("selectionEnabled", true)
            inputable: rowValue("inputable", false)
            showLeadingIcon: rowValue("showLeadingIcon", true)
            showDescription: rowValue("showDescription", true)
            showValue: rowValue("showValue", true)
            showTrailingIcon: rowValue("showTrailingIcon", true)
            showBookmark: rowValue("showBookmark", true)
            showDate: rowValue("showDate", true)
            showFolders: rowValue("showFolders", true)
            showTags: rowValue("showTags", true)
            showMetadata: rowValue("showMetadata", true)
            showPrimaryAction: rowValue("showPrimaryAction", true)
            showSecondaryAction: rowValue("showSecondaryAction", true)
            showMoreMenu: rowValue("showMoreMenu", true)
            showStepper: rowValue("showStepper", true)
            showSelector: rowValue("showSelector", true)
            showUnitSelector: rowValue("showUnitSelector", true)
            showToggle: rowValue("showToggle", true)
            showSelection: rowValue("showSelection", true)
            showSegments: rowValue("showSegments", true)
            showInput1: rowValue("showInput1", true)
            showInput2: rowValue("showInput2", true)
            showPreview: rowValue("showPreview", true)
            leadingComponent: rowValue("leadingComponent", null)
            trailingComponent: rowValue("trailingComponent", null)
            previewComponent: rowValue("previewComponent", null)
            footerComponent: rowValue("footerComponent", null)

            function syncState() {
                const rowType = modelData.type;
                checked = rowValue("checked", rowType === ListItem.Toggle || rowType === ListItem.DetailQuantity || rowType === ListItem.DetailSettings || rowType === ListItem.Form);
                quantity = rowValue("quantity", 1);
                selectorIndex = rowValue("selectorIndex", 0);
                unitIndex = rowValue("unitIndex", 0);
                segmentIndex = rowValue("segmentIndex", 0);
                inputText1 = rowValue("inputText1", "Value");
                inputText2 = rowValue("inputText2", "Value");
            }

            Component.onCompleted: syncState()
            onModelDataChanged: syncState()

            onClicked: control.triggerItem(index)
            onEdited: function (field, value) {
                control.itemEdited(index, entry, field, value);
            }
            onActionTriggered: function (action, payload) {
                control.itemActionTriggered(index, entry, action, payload);
            }
            onInputEdited: function (text) {
                control.itemEdited(index, entry, "label", text);
            }
        }
    }

    ModelSource {
        id: listModelSource
        source: control.sourceModel
        column: control.modelColumn
    }

    function invalidateModel() {
        listModelSource.invalidate();
    }

    function entryAt(index) {
        listModelSource.revision;
        return listModelSource.at(index);
    }

    function roleValue(entry, roleName, fallbackValue) {
        return listModelSource.roleValue(entry, roleName, fallbackValue);
    }

    function itemLabel(entry) {
        return listModelSource.textValue(entry, [labelRole, textRole, titleRole, "display", "edit"], "");
    }

    function itemEnabled(entry) {
        return listModelSource.boolValue(entry, enabledRole, true);
    }

    function itemIconName(entry) {
        const roles = [iconRole, "icon", "sourceIcon"];
        for (let index = 0; index < roles.length; ++index) {
            const value = roleValue(entry, roles[index], null);
            if (value !== null && value !== undefined && String(value).length > 0)
                return String(value);
        }
        return defaultItemIconName;
    }

    function itemSelected(entry, index) {
        const value = roleValue(entry, selectedRole, null);
        if (value !== null && value !== undefined)
            return !!value;
        return index === selectedIndex;
    }

    function itemType(entry) {
        const raw = roleValue(entry, typeRole, roleValue(entry, "size", defaultItemType));
        if (typeof raw === "number")
            return raw >= ListItem.Mini && raw <= ListItem.Form ? Math.floor(raw) : defaultItemType;
        const names = ["Mini", "Detail", "Navigation", "Toggle", "Checkable", "Action", "ActionGroup", "Stepper", "Select", "InlineEdit", "DetailActions", "DetailQuantity", "DetailSettings", "Resource", "Media", "Task", "Form"];
        const index = names.indexOf(String(raw));
        return index >= 0 ? index : defaultItemType;
    }

    function preferredListWidth() {
        listModelSource.revision;
        let width = 170;
        for (let i = 0; i < entryCount; ++i) {
            const type = itemType(entryAt(i));
            width = Math.max(width, type >= ListItem.Action ? 400 : type >= ListItem.Navigation ? 280 : type === ListItem.Detail ? 194 : 170);
        }
        return Theme.scaleMetric(width);
    }

    function triggerItem(index) {
        const entry = entryAt(index);
        itemTriggered(index, entry);
    }

    function buildItemDelegateItems() {
        listModelSource.revision;
        const count = entryCount;
        const result = [];
        for (let i = 0; i < count; i++) {
            const entry = entryAt(i);
            result.push({
                "index": i,
                "entry": entry,
                "label": itemLabel(entry),
                "iconName": itemIconName(entry),
                "enabled": interactive && itemEnabled(entry),
                "selected": itemSelected(entry, i),
                "type": itemType(entry),
                "description": roleValue(entry, descriptionRole, "Supporting text"),
                "properties": entry && typeof entry === "object" ? entry : ({}),
                "trigger": function () {
                    control.triggerItem(i);
                }
            });
        }
        return result;
    }

    function createDelegateItem(parentItem, component, descriptor) {
        if (!component)
            return null;
        return component.createObject(parentItem, {
            "modelData": descriptor
        });
    }

    readonly property int contentHeight: {
        const toolbarHeight = control.toolbarVisible ? toolbar.implicitHeight : 0;
        const footerHeight = control.footerVisible ? footer.implicitHeight : 0;
        return toolbarHeight + listItemsColumn.height + footerHeight;
    }

    implicitWidth: control.listWidth
    implicitHeight: control.expandToContent ? Math.max(control.minimumListHeight, contentHeight) : control.minimumListHeight

    onSourceModelChanged: invalidateModel()

    Rectangle {
        objectName: "list_background"
        anchors.fill: parent
        color: control.backgroundColor
    }

    ColumnLayout {
        id: rootColumn
        anchors.fill: parent
        spacing: Theme.gapNone

        ListToolbar {
            id: toolbar
            visible: control.toolbarVisible
            Layout.fillWidth: true
            icon1: control.toolbarIcon1
            icon2: control.toolbarIcon2
            icon3: control.toolbarIcon3
            interactive: control.interactive
            onIconClicked: (index, source) => control.toolbarIconTriggered(index, source)
        }

        Flickable {
            id: listItemsViewport
            objectName: "list_itemsViewport"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: listItemsColumn.height
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            interactive: control.interactive && control.scrollable && contentHeight > height

            Column {
                id: listItemsColumn
                objectName: "list_itemsColumn"
                // Measure rows even before a zero-height expanding viewport is laid out.
                width: control.width
                spacing: control.itemSpacing

                Repeater {
                    model: control.entryCount

                    delegate: Item {
                        id: delegateRoot
                        objectName: "list_delegateRoot_" + index
                        required property int index
                        readonly property var descriptor: index >= 0 && index < control.itemDelegateItems.length ? control.itemDelegateItems[index] : ({})
                        property Item delegateItem: null

                        width: listItemsColumn.width
                        height: delegateItem ? Math.max(control.itemHeight, delegateItem.implicitHeight) : control.itemHeight
                        implicitHeight: height

                        function rebuildDelegate() {
                            if (delegateItem) {
                                delegateItem.destroy();
                                delegateItem = null;
                            }
                            delegateItem = control.createDelegateItem(delegateRoot, control.itemDelegate, descriptor);
                            if (!delegateItem)
                                return;
                            delegateItem.width = Qt.binding(function () {
                                return delegateRoot.width;
                            });
                            delegateItem.height = Qt.binding(function () {
                                return delegateRoot.height;
                            });
                        }

                        Component.onCompleted: rebuildDelegate()
                        onDescriptorChanged: {
                            if (delegateItem)
                                delegateItem.modelData = descriptor;
                        }

                        Connections {
                            target: control
                            function onItemDelegateChanged() {
                                delegateRoot.rebuildDelegate();
                            }
                        }
                    }
                }
            }
        }

        ListFooter {
            id: footer
            objectName: "list_footer"
            visible: control.footerVisible
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            interactive: control.interactive
            button1: control.footerButton1
            button2: control.footerButton2
            button3: control.footerButton3
            onButtonClicked: (index, config) => control.footerButtonTriggered(index, config)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.List { model: [{ label: "Item 1" }, { label: "Item 2" }] }

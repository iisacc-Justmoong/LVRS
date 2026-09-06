import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    enum ItemSize {
        Mini,
        Detail,
        Navigation,
        Toggle,
        Checkable,
        Action,
        ActionGroup,
        Stepper,
        Select,
        InlineEdit,
        DetailActions,
        DetailQuantity,
        DetailSettings,
        Resource,
        Media,
        Task,
        Form
    }

    property int type: ListItem.Mini
    property alias size: control.type

    property string label: "Label"
    // Legacy field retained as the detail-variant title.
    property string detail: "asasdsadasasdsadasasd Maxinum lines: 2 lines"
    property string iconName: "nodesfolder"
    property url iconSource: ""
    property bool selected: false
    property bool showChevron: false
    property bool inputable: false
    property string inputResult: control.label

    property string dateText: "YYYY-MM-dd"
    property string folderLabel1: "Only"
    property string folderLabel2: "1 Line"
    property string tagLabel1: "Only"
    property string tagLabel2: "1 Line"
    property string bookmarkIconName: "bookmarksbookmark"
    property string folderIconName: "folder@14x14"
    property string tagIconName: "vcscurrentBranch"

    property string description: "Supporting text"
    property string value: "Value"
    property string metadata1: "Metadata"
    property string metadata2: "Status"
    property string statusText: "Draft"
    property string quantityLabel: "Quantity label"
    property string optionLabel: "Apply to all"
    property string modeLabel: "Mode label"
    property string scopeLabel: "Scope label"
    property string durationText: "03:42"
    property string mediaDetail: "WAV · 48 kHz"
    property string fieldLabel1: "Field 1 label"
    property string fieldLabel2: "Field 2 label"
    property string inputText1: "Value"
    property string inputText2: "Value"
    property string previewIconName: "nodesfolder"
    property url previewSource: ""
    property string trailingIconName: "generalchevronRight"

    property real quantity: 1
    property real minimumQuantity: 0
    property real maximumQuantity: Number.POSITIVE_INFINITY
    property real stepSize: 1
    property int selectorIndex: 0
    property int unitIndex: 0
    property int segmentIndex: 0
    property var primaryAction: ({})
    property var secondaryAction: ({})
    property var moreMenu: ({})
    property var selector: ({})
    property var unitSelector: ({})
    property var stepper: ({})
    property var input1: ({})
    property var input2: ({})
    property var segments: ["Original", "Edited"]
    property bool toggleEnabled: true
    property bool selectionEnabled: true

    property bool showLeadingIcon: true
    property bool showDescription: true
    property bool showValue: true
    property bool showTrailingIcon: true
    property bool showBookmark: true
    property bool showDate: true
    property bool showFolders: true
    property bool showTags: true
    property bool showMetadata: true
    property bool showPrimaryAction: true
    property bool showSecondaryAction: true
    property bool showMoreMenu: true
    property bool showStepper: true
    property bool showSelector: true
    property bool showUnitSelector: true
    property bool showToggle: true
    property bool showSelection: true
    property bool showSegments: true
    property bool showInput1: true
    property bool showInput2: true
    property bool showPreview: true

    // Components may declare `property var listItem` to receive the owning row.
    property Component leadingComponent: null
    property Component trailingComponent: null
    property Component previewComponent: null
    property Component footerComponent: null

    property int navigationItemWidth: Theme.scaleMetric(280)
    property int compositeItemWidth: Theme.scaleMetric(400)
    property int standardItemHeight: Theme.scaleMetric(44)
    property int contentSpacing: Theme.gap8
    property int textSpacing: Theme.gap4
    property int sectionSpacing: Theme.gap12
    property int actionWidth: Theme.scaleMetric(72)
    property int inputWidth: Theme.scaleMetric(140)
    property int previewSize: Theme.scaleMetric(48)
    readonly property var variantNames: ["Mini", "Detail", "Navigation", "Toggle", "Checkable", "Action", "ActionGroup", "Stepper", "Select", "InlineEdit", "DetailActions", "DetailQuantity", "DetailSettings", "Resource", "Media", "Task", "Form"]
    readonly property string variantName: variantNames[type] || "Mini"
    readonly property bool isModern: type >= ListItem.Navigation && type <= ListItem.Form
    readonly property bool isCompound: type >= ListItem.DetailActions && type <= ListItem.Form

    checked: type === ListItem.Toggle || type === ListItem.DetailQuantity || type === ListItem.DetailSettings || type === ListItem.Form

    signal edited(string field, var value)
    signal actionTriggered(string action, var payload)

    function typeValue(value) {
        if (typeof value === "number")
            return value >= 0 && value < variantNames.length ? Math.floor(value) : ListItem.Mini;
        const index = variantNames.indexOf(String(value));
        return index < 0 ? ListItem.Mini : index;
    }

    function configValue(config, name, fallback) {
        return config && config[name] !== undefined && config[name] !== null ? config[name] : fallback;
    }

    function optionAt(options, index) {
        if (!options || index < 0)
            return null;
        return typeof options.get === "function" ? options.get(index) : options[index];
    }

    function optionText(option, fallback) {
        if (option === undefined || option === null)
            return fallback;
        if (typeof option !== "object")
            return String(option);
        return String(configValue(option, "text", configValue(option, "label", fallback)));
    }

    function editValue(field, value) {
        if (!effectiveEnabled)
            return false;
        if (field === "quantity") {
            const number = Number(value);
            if (!Number.isFinite(number))
                return false;
            value = Math.max(minimumQuantity, Math.min(Math.max(minimumQuantity, maximumQuantity), number));
        }
        if (["checked", "quantity", "selectorIndex", "unitIndex", "segmentIndex", "inputText1", "inputText2"].indexOf(field) < 0 || control[field] === value)
            return false;
        control[field] = value;
        edited(field, value);
        return true;
    }

    function stepQuantity(direction) {
        if (!showStepper || !configValue(stepper, "enabled", true))
            return false;
        return editValue("quantity", quantity + direction * Math.max(0, stepSize));
    }

    function actionText(action) {
        if (action === "secondary") {
            if (type === ListItem.DetailSettings)
                return "Reset";
            if (type === ListItem.Resource)
                return "Reveal";
            if (type === ListItem.Media)
                return "Stop";
            if (type === ListItem.Task)
                return "Assign";
            if (type === ListItem.Form)
                return "Cancel";
            return "Edit";
        }
        if (type === ListItem.InlineEdit || type === ListItem.Form)
            return "Save";
        if (type === ListItem.DetailQuantity || type === ListItem.DetailSettings)
            return "Apply";
        if (type === ListItem.Media)
            return "Play";
        if (type === ListItem.Task)
            return "Done";
        return "Open";
    }

    function triggerAction(action, payload) {
        const config = action === "primary" ? primaryAction : action === "secondary" ? secondaryAction : moreMenu;
        if (!effectiveEnabled || !configValue(config, "enabled", true) || configValue(config, "tone", AbstractButton.Default) === AbstractButton.Disabled)
            return false;
        const event = createMethodEvent(action);
        event.action = action;
        event.payload = payload;
        event.values = {
            inputText1: inputText1,
            inputText2: inputText2,
            checked: checked,
            quantity: quantity,
            selectorIndex: selectorIndex,
            unitIndex: unitIndex,
            segmentIndex: segmentIndex
        };
        const method = configValue(config, "method", configValue(config, "onTriggered", null));
        if (method)
            invokeMethod(method, event);
        const methods = configValue(config, "methods", []);
        for (let i = 0; i < methods.length; ++i)
            invokeMethod(methods[i], event);
        actionTriggered(action, payload === undefined ? event.values : payload);
        return true;
    }

    property int iconSize: Theme.iconSm
    property int rowHorizontalPadding: Theme.gap4
    property int rowVerticalPadding: Theme.gap2
    property int miniItemSpacing: Theme.scaleMetric(1)
    property int miniItemWidth: Theme.scaleMetric(170)
    property int detailItemWidth: Theme.scaleMetric(194)
    property int detailHorizontalPadding: Theme.scaleMetric(12)
    property int detailVerticalPadding: Theme.gap8
    property int detailSectionSpacing: Theme.gap8
    property int detailTopSpacing: Theme.scaleMetric(10)
    property int metadataItemSpacing: Theme.gap8
    property int metadataRowSpacing: Theme.gap2
    property int separatorHeight: Theme.scaleMetric(1)
    property int separatorTopSpacing: Theme.scaleMetric(1)
    property bool separatorVisible: false
    property int minItemWidth: control.size === ListItem.Detail ? control.detailItemWidth : control.isModern ? (control.type <= ListItem.Checkable ? control.navigationItemWidth : control.compositeItemWidth) : control.miniItemWidth
    property color listBackgroundColor: "transparent"
    property color selectedBackgroundColor: Theme.accentOverlay
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5

    readonly property url resolvedIconSource: control.iconSource.toString().length > 0 ? control.iconSource : Theme.iconPath(control.iconName)
    readonly property real iconSupersampleScale: RenderQuality.enabled ? RenderQuality.effectiveSupersampleScaleValue : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property int detailContentWidth: Math.max(0, control.detailItemWidth - (control.detailHorizontalPadding * 2))
    readonly property int detailTopHeight: Math.max(control.iconSize, Theme.textDescriptionLineHeight * 2)
    readonly property int detailMiddleHeight: Theme.textDescriptionLineHeight
    readonly property int detailBottomHeight: (control.iconSize * 2) + control.metadataRowSpacing
    readonly property int detailContentHeight: control.detailTopHeight + control.detailSectionSpacing + control.detailMiddleHeight + control.detailSectionSpacing + control.detailBottomHeight

    signal inputEdited(string text)
    signal inputSubmitted(string text)

    function normalizedText(value) {
        if (value === undefined || value === null)
            return "";
        return String(value);
    }

    function applyInputResult(value) {
        const normalized = normalizedText(value);
        if (control.label !== normalized)
            control.label = normalized;
        if (control.inputResult !== normalized)
            control.inputResult = normalized;
        return normalized;
    }

    function resolvedAsset(name) {
        const normalized = name === undefined || name === null ? "" : String(name).trim();
        return normalized.length > 0 ? Theme.iconPath(normalized) : "";
    }

    tone: AbstractButton.Borderless
    horizontalPadding: control.size === ListItem.Detail || control.isModern ? control.detailHorizontalPadding : control.rowHorizontalPadding
    verticalPadding: control.isCompound ? control.sectionSpacing : control.size === ListItem.Detail || control.isModern ? control.detailVerticalPadding : control.rowVerticalPadding
    spacing: Theme.gapNone
    cornerRadius: control.isModern ? Theme.scaleMetric(6) : Theme.gapNone
    clip: control.isModern
    Accessible.name: control.label
    Accessible.description: control.isModern ? control.description : control.detail

    implicitHeight: Math.max(control.isModern && !control.isCompound ? control.standardItemHeight : 0, contentRoot.implicitHeight + topPadding + bottomPadding)
    implicitWidth: Math.max(control.minItemWidth, contentRoot.implicitWidth + leftPadding + rightPadding)

    backgroundColor: control.selected ? control.selectedBackgroundColor : control.listBackgroundColor
    backgroundColorHover: control.selected ? control.selectedBackgroundColor : control.listBackgroundColor
    backgroundColorPressed: control.selected ? control.selectedBackgroundColor : Theme.accentBlueMuted
    backgroundColorDisabled: control.listBackgroundColor
    textColor: Theme.bodyColor
    textColorDisabled: Theme.disabledColor

    contentItem: Item {
        id: contentRoot
        objectName: "listItem_content"

        readonly property int miniBaseHeight: control.iconSize
        readonly property int miniSeparatorHeight: control.separatorVisible ? control.separatorTopSpacing + control.separatorHeight : 0

        implicitWidth: control.isModern ? control.minItemWidth - control.leftPadding - control.rightPadding : control.size === ListItem.Detail ? control.detailContentWidth : miniIcon.width + control.miniItemSpacing + miniLabel.implicitWidth
        implicitHeight: control.isModern ? modernLoader.implicitHeight : control.size === ListItem.Detail ? control.detailContentHeight : miniBaseHeight + miniSeparatorHeight

        Loader {
            id: modernLoader
            active: control.isModern
            visible: active
            width: parent.width
            height: implicitHeight
            y: Math.max(0, (parent.height - height) / 2)
            sourceComponent: Component {
                ListItemComposite {
                    listItem: control
                }
            }
        }

        Item {
            id: miniContent
            objectName: "listItem_miniContent"
            visible: control.size === ListItem.Mini
            x: 0
            y: 0
            width: parent.width
            height: contentRoot.miniBaseHeight

            Image {
                id: miniIcon
                objectName: "listItem_miniIcon"
                x: 0
                y: 0
                width: control.iconSize
                height: control.iconSize
                source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
                sourceSize.width: control.iconSourceSize
                sourceSize.height: control.iconSourceSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: RenderQuality.mipmapEnabled
            }

            Label {
                id: miniLabel
                objectName: "listItem_miniLabel"
                x: control.iconSize + control.miniItemSpacing
                y: (parent.height - height) * 0.5
                width: Math.min(Math.ceil(implicitWidth), Math.max(0, parent.width - x))
                height: Theme.textBodyLineHeight
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
                x: miniLabel.x
                y: 0
                width: Math.max(0, parent.width - x)
                height: parent.height
                active: control.size === ListItem.Mini && control.inputable
                visible: active
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
                        fieldMinHeight: Theme.textBodyLineHeight
                        centeredTextHeight: Theme.textBodyLineHeight
                        insetHorizontal: 0
                        insetVertical: 0
                        sideSpacing: 0
                        cornerRadius: 0
                        textColor: Theme.bodyColor
                        textColorDisabled: Theme.disabledColor
                        placeholderColor: Theme.disabledColor
                        placeholderColorDisabled: Theme.disabledColor

                        onTextEdited: {
                            const value = control.applyInputResult(text);
                            control.inputEdited(value);
                        }
                        onAccepted: control.inputSubmitted(control.applyInputResult(text))
                    }
                }

                onLoaded: {
                    if (item) {
                        item.objectName = control.objectName.length > 0 ? control.objectName + "_inputField" : "";
                        item.text = control.inputResult;
                    }
                }
            }
        }

        Rectangle {
            objectName: "listItem_separator"
            visible: control.size === ListItem.Mini && control.separatorVisible
            x: 0
            y: contentRoot.miniBaseHeight + control.separatorTopSpacing
            width: parent.width
            height: control.separatorHeight
            color: control.separatorColor
            opacity: control.separatorOpacity
        }

        Item {
            id: detailContent
            objectName: "listItem_detailContent"
            visible: control.size === ListItem.Detail
            x: 0
            y: 0
            width: parent.width
            height: control.detailContentHeight

            Item {
                id: detailTop
                objectName: "listItem_detailTop"
                x: 0
                y: 0
                width: parent.width
                height: control.detailTopHeight

                Label {
                    id: detailTitle
                    objectName: "listItem_detailTitle"
                    x: 0
                    y: 0
                    width: Math.max(0, parent.width - control.detailTopSpacing - control.iconSize)
                    height: parent.height
                    style: description
                    text: control.detail
                    color: control.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                }

                Image {
                    id: detailBookmark
                    objectName: "listItem_detailBookmark"
                    x: parent.width - width
                    y: 0
                    width: control.iconSize
                    height: control.iconSize
                    source: RenderQuality.resolveTextureSource(control.resolvedAsset(control.bookmarkIconName))
                    sourceSize.width: control.iconSourceSize
                    sourceSize.height: control.iconSourceSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: RenderQuality.mipmapEnabled
                }
            }

            Item {
                id: detailMiddle
                objectName: "listItem_detailMiddle"
                x: 0
                y: detailTop.y + detailTop.height + control.detailSectionSpacing
                width: parent.width
                height: control.detailMiddleHeight

                Label {
                    id: detailDate
                    objectName: "listItem_detailDate"
                    x: 0
                    y: 0
                    width: Math.ceil(implicitWidth)
                    height: parent.height
                    style: description
                    text: control.dateText
                    color: control.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item {
                id: detailBottom
                objectName: "listItem_detailBottom"
                x: 0
                y: detailMiddle.y + detailMiddle.height + control.detailSectionSpacing
                width: parent.width
                height: control.detailBottomHeight

                Item {
                    id: foldersRow
                    objectName: "listItem_detailFolders"
                    x: 0
                    y: 0
                    width: parent.width
                    height: control.iconSize

                    Item {
                        id: folderGroup1
                        x: 0
                        y: 0
                        width: folderIcon1.width + control.metadataItemSpacing + folderText1.width
                        height: control.iconSize

                        Image {
                            id: folderIcon1
                            objectName: "listItem_folderIcon1"
                            x: 0
                            y: 0
                            width: control.iconSize
                            height: control.iconSize
                            source: RenderQuality.resolveTextureSource(control.resolvedAsset(control.folderIconName))
                            sourceSize.width: control.iconSourceSize
                            sourceSize.height: control.iconSourceSize
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: RenderQuality.mipmapEnabled
                        }

                        Label {
                            id: folderText1
                            objectName: "listItem_folderLabel1"
                            x: control.iconSize + control.metadataItemSpacing
                            y: (parent.height - height) * 0.5
                            width: Math.ceil(implicitWidth)
                            height: Theme.textCaptionLineHeight
                            style: caption
                            text: control.folderLabel1
                            color: control.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
                        }
                    }

                    Item {
                        id: folderGroup2
                        x: folderGroup1.x + folderGroup1.width + control.metadataItemSpacing
                        y: 0
                        width: folderIcon2.width + control.metadataItemSpacing + folderText2.width
                        height: control.iconSize

                        Image {
                            id: folderIcon2
                            objectName: "listItem_folderIcon2"
                            x: 0
                            y: 0
                            width: control.iconSize
                            height: control.iconSize
                            source: RenderQuality.resolveTextureSource(control.resolvedAsset(control.folderIconName))
                            sourceSize.width: control.iconSourceSize
                            sourceSize.height: control.iconSourceSize
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: RenderQuality.mipmapEnabled
                        }

                        Label {
                            id: folderText2
                            x: control.iconSize + control.metadataItemSpacing
                            y: (parent.height - height) * 0.5
                            width: Math.ceil(implicitWidth)
                            height: Theme.textCaptionLineHeight
                            style: caption
                            text: control.folderLabel2
                            color: control.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
                        }
                    }
                }

                Item {
                    id: tagsRow
                    objectName: "listItem_detailTags"
                    x: 0
                    y: foldersRow.y + foldersRow.height + control.metadataRowSpacing
                    width: parent.width
                    height: control.iconSize

                    Item {
                        id: tagGroup1
                        x: 0
                        y: 0
                        width: tagIcon1.width + control.metadataItemSpacing + tagText1.width
                        height: control.iconSize

                        Image {
                            id: tagIcon1
                            objectName: "listItem_tagIcon1"
                            x: 0
                            y: 0
                            width: control.iconSize
                            height: control.iconSize
                            source: RenderQuality.resolveTextureSource(control.resolvedAsset(control.tagIconName))
                            sourceSize.width: control.iconSourceSize
                            sourceSize.height: control.iconSourceSize
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: RenderQuality.mipmapEnabled
                        }

                        Label {
                            id: tagText1
                            x: control.iconSize + control.metadataItemSpacing
                            y: (parent.height - height) * 0.5
                            width: Math.ceil(implicitWidth)
                            height: Theme.textCaptionLineHeight
                            style: caption
                            text: control.tagLabel1
                            color: control.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
                        }
                    }

                    Item {
                        id: tagGroup2
                        x: tagGroup1.x + tagGroup1.width + control.metadataItemSpacing
                        y: 0
                        width: tagIcon2.width + control.metadataItemSpacing + tagText2.width
                        height: control.iconSize

                        Image {
                            id: tagIcon2
                            objectName: "listItem_tagIcon2"
                            x: 0
                            y: 0
                            width: control.iconSize
                            height: control.iconSize
                            source: RenderQuality.resolveTextureSource(control.resolvedAsset(control.tagIconName))
                            sourceSize.width: control.iconSourceSize
                            sourceSize.height: control.iconSourceSize
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: RenderQuality.mipmapEnabled
                        }

                        Label {
                            id: tagText2
                            x: control.iconSize + control.metadataItemSpacing
                            y: (parent.height - height) * 0.5
                            width: Math.ceil(implicitWidth)
                            height: Theme.textCaptionLineHeight
                            style: caption
                            text: control.tagLabel2
                            color: control.effectiveEnabled ? Theme.captionColor : Theme.disabledColor
                        }
                    }
                }
            }
        }
    }

    onLabelChanged: {
        const normalized = control.normalizedText(control.label);
        if (control.inputResult !== normalized)
            control.inputResult = normalized;
        if (overlayInputLoader.status === Loader.Ready && overlayInputLoader.item && !overlayInputLoader.item.activeFocus && overlayInputLoader.item.text !== normalized) {
            overlayInputLoader.item.text = normalized;
        }
    }
    onInputableChanged: {
        if (control.inputable && overlayInputLoader.status === Loader.Ready && overlayInputLoader.item && !overlayInputLoader.item.activeFocus) {
            overlayInputLoader.item.text = control.inputResult;
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ListItem { label: "Label" }
// LV.ListItem { size: LV.ListItem.Detail; detail: "Two-line note title" }

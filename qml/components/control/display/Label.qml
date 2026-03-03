import QtQuick
import LVRS 1.0

Item {
    id: control

    // Style constants for API usage: LV.Label { style: body }
    readonly property int title: 0
    readonly property int title2: 1
    readonly property int header: 2
    readonly property int header2: 3
    readonly property int body: 4
    readonly property int description: 5
    readonly property int caption: 6
    readonly property int disabled: 7

    property int style: description

    readonly property color resolvedStyleColor: {
        if (style === title || style === title2 || style === header || style === header2)
            return Theme.titleHeaderColor
        if (style === body)
            return Theme.bodyColor
        if (style === caption)
            return Theme.captionColor
        if (style === disabled)
            return Theme.disabledColor
        return Theme.descriptionColor
    }
    readonly property int stylePixelSize: {
        if (style === title)
            return Theme.textTitle
        if (style === title2)
            return Theme.textTitle2
        if (style === header)
            return Theme.textHeader
        if (style === header2)
            return Theme.textHeader2
        if (style === body)
            return Theme.textBody
        if (style === caption)
            return Theme.textCaption
        if (style === disabled)
            return Theme.textDisabled
        return Theme.textDescription
    }
    readonly property int styleWeight: {
        if (style === title)
            return Theme.textTitleWeight
        if (style === title2)
            return Theme.textTitle2Weight
        if (style === header)
            return Theme.textHeaderWeight
        if (style === header2)
            return Theme.textHeader2Weight
        if (style === body)
            return Theme.textBodyWeight
        if (style === caption)
            return Theme.textCaptionWeight
        if (style === disabled)
            return Theme.textDisabledWeight
        return Theme.textDescriptionWeight
    }
    readonly property string styleName: {
        if (style === title)
            return Theme.textTitleStyleName
        if (style === title2)
            return Theme.textTitle2StyleName
        if (style === header)
            return Theme.textHeaderStyleName
        if (style === header2)
            return Theme.textHeader2StyleName
        if (style === body)
            return Theme.textBodyStyleName
        if (style === caption)
            return Theme.textCaptionStyleName
        if (style === disabled)
            return Theme.textDisabledStyleName
        return Theme.textDescriptionStyleName
    }
    readonly property int styleLineHeight: {
        if (style === title)
            return Theme.textTitleLineHeight
        if (style === title2)
            return Theme.textTitle2LineHeight
        if (style === header)
            return Theme.textHeaderLineHeight
        if (style === header2)
            return Theme.textHeader2LineHeight
        if (style === body)
            return Theme.textBodyLineHeight
        if (style === caption)
            return Theme.textCaptionLineHeight
        if (style === disabled)
            return Theme.textDisabledLineHeight
        return Theme.textDescriptionLineHeight
    }
    readonly property real styleLetterSpacing: {
        if (style === title)
            return Theme.textTitleLetterSpacing
        if (style === title2)
            return Theme.textTitle2LetterSpacing
        if (style === header)
            return Theme.textHeaderLetterSpacing
        if (style === header2)
            return Theme.textHeader2LetterSpacing
        if (style === body)
            return Theme.textBodyLetterSpacing
        if (style === caption)
            return Theme.textCaptionLetterSpacing
        if (style === disabled)
            return Theme.textDisabledLetterSpacing
        return Theme.textDescriptionLetterSpacing
    }

    property alias text: textNode.text
    property alias color: textNode.color
    property alias font: textNode.font
    property alias elide: textNode.elide
    property alias wrapMode: textNode.wrapMode
    property alias horizontalAlignment: textNode.horizontalAlignment
    property alias verticalAlignment: textNode.verticalAlignment
    property alias lineHeight: textNode.lineHeight
    property alias lineHeightMode: textNode.lineHeightMode
    property alias maximumLineCount: textNode.maximumLineCount
    property alias minimumPixelSize: textNode.minimumPixelSize
    property alias minimumPointSize: textNode.minimumPointSize
    property alias fontSizeMode: textNode.fontSizeMode
    property alias renderType: textNode.renderType
    property alias styleColor: textNode.styleColor
    property alias textFormat: textNode.textFormat
    property alias linkColor: textNode.linkColor

    implicitWidth: textNode.implicitWidth
    implicitHeight: textNode.implicitHeight

    Text {
        id: textNode
        width: control.width > 0 ? control.width : implicitWidth
        height: control.height > 0 ? control.height : implicitHeight
        color: control.resolvedStyleColor
        font.family: Theme.fontBody
        font.pixelSize: control.stylePixelSize
        font.weight: control.styleWeight
        font.styleName: control.styleName
        font.letterSpacing: control.styleLetterSpacing
        lineHeight: control.styleLineHeight
        lineHeightMode: Text.FixedHeight
        elide: Text.ElideRight
        renderType: RenderQuality.nativeTextRendering ? Text.NativeRendering : Text.QtRendering
    }

    onStyleChanged: {
        if (!Theme.isThemeTextStyleCompliant(stylePixelSize, styleWeight, styleName))
            Debug.warn("Label", "style-noncompliant", {
                "pixelSize": stylePixelSize,
                "weight": styleWeight,
                "styleName": styleName
            })
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Label { text: "Label"; style: body }

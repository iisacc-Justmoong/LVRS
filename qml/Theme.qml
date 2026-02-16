pragma Singleton
import QtQuick
import LVRS 1.0

QtObject {
    readonly property bool dark: true

    readonly property string fontBody: FontPolicy.resolveFamily(FontPolicy.preferredFamily)
    readonly property string fontDisplay: FontPolicy.resolveFamily(FontPolicy.preferredFamily)
    readonly property string iconSetBasePath: "qrc:/qt/qml/LVRS/resources/iconset/"

    function iconPath(iconName) {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        const trimmedName = rawName.trim()
        if (trimmedName.length === 0)
            return ""
        if (trimmedName.indexOf(":/") !== -1)
            return trimmedName
        const lowerName = trimmedName.toLowerCase()
        if (lowerName.length >= 4 && lowerName.lastIndexOf(".svg") === lowerName.length - 4)
            return iconSetBasePath + trimmedName
        return iconSetBasePath + trimmedName + ".svg"
    }

    //Window

    readonly property color window: "#141414"

    // Figma backgrounds
    // 147:29 rgba(37,38,39,0.8), 147:30 rgba(37,38,39,0.7), 147:31 rgba(37,38,39,0.6), 147:32 rgba(37,38,39,0.5)
    // 147:33 rgba(37,38,39,0.4), 147:34 rgba(37,38,39,0.3), 147:36 rgba(37,38,39,0.2), 147:37 rgba(37,38,39,0.2)
    readonly property color panelBackground01: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.8)
    readonly property color panelBackground02: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.7)
    readonly property color panelBackground03: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.6)
    readonly property color panelBackground04: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.5)
    readonly property color panelBackground05: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.4)
    readonly property color panelBackground06: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.3)
    readonly property color panelBackground07: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.2)
    readonly property color panelBackground08: Qt.rgba(37 / 255, 38 / 255, 39 / 255, 0.2)

    readonly property color windowAlt: panelBackground08
    readonly property color subSurface: panelBackground01
    readonly property color surfaceSolid: panelBackground02
    readonly property color surfaceAlt: panelBackground03
    readonly property color surfaceGhost: panelBackground07

    //Text Color (Figma tokens: TitleHeader / Body / Description / Caption / Disabled)

    readonly property color textTokenBase: "#FFFFFF"
    readonly property real textTokenTitleHeaderOpacity: 0.9
    readonly property real textTokenBodyOpacity: 0.8
    readonly property real textTokenDescriptionOpacity: 0.6
    readonly property real textTokenCaptionOpacity: 0.5
    readonly property real textTokenDisabledOpacity: 0.3

    readonly property color textTokenTitleHeader: "#E5FFFFFF"
    readonly property color textTokenBody: "#CCFFFFFF"
    readonly property color textTokenDescription: "#99FFFFFF"
    readonly property color textTokenCaption: "#80FFFFFF"
    readonly property color textTokenDisabled: "#4DFFFFFF"

    readonly property color titleHeaderColor: textTokenTitleHeader
    readonly property color bodyColor: textTokenBody
    readonly property color descriptionColor: textTokenDescription
    readonly property color captionColor: textTokenCaption
    readonly property color disabledColor: textTokenDisabled

    readonly property color textPrimary: titleHeaderColor
    readonly property color textSecondary: bodyColor
    readonly property color textTertiary: descriptionColor
    readonly property color textSeptenary: captionColor
    readonly property color textOctonary: disabledColor

    //Primary

    readonly property color primary: "#0a84ff"
    readonly property color accent: primary
    readonly property color success: "#32d74b"
    readonly property color warning: "#ffd60a"
    readonly property color danger: "#ff453a"
    readonly property color surface: "#38383c"
    readonly property color darkGrey10: "#b4b8bf"
    readonly property color accentTint: "#1F0A84FF"
    readonly property color dangerTint: "#1FFF453A"
    readonly property color primaryOverlay: "#400A84FF"
    readonly property color accentOverlay: primaryOverlay
    readonly property color dangerOverlay: "#59FF453A"
    readonly property color overlayBackdrop: "#59000000"
    readonly property color shadowStrong: "#40000000"
    readonly property color strokeSoft: "#D0D7E2"
    // Accent palette (all fill/stroke colors found in resources/iconset)
    readonly property color accentTransparent: "transparent" // none
    readonly property color accentWhite: "#FFFFFF"
    readonly property color accentGrayLight: "#CED0D6"
    readonly property color accentBlue: "#548AF7"
    readonly property color accentRed: "#DB5C5C"
    readonly property color accentSlate: "#43454A"
    readonly property color accentGreen: "#57965C"
    readonly property color accentBlueMuted: "#25324D"
    readonly property color accentOrangeMuted: "#C77D55"
    readonly property color accentGreenMuted: "#253627"
    readonly property color accentYellow: "#F2C55C"
    readonly property color accentRedBrownDark: "#402929"
    readonly property color accentGray: "#868A91"
    readonly property color accentYellowMuted: "#D6AE58"
    readonly property color accentBrownMuted: "#45322B"
    readonly property color accentPurple: "#A571E6"
    readonly property color accentBrownDarker: "#3D3223"
    readonly property color accentCharcoal: "#1E1F22"
    readonly property color accentGrayPale: "#B4B8BF"
    readonly property color accentBlueBright: "#3574F0"
    readonly property color accentPurpleDarker: "#2F2936"
    readonly property color accentGrayBright: "#F0F1F2"
    readonly property color accentRose: "#E55765"
    readonly property color accentRoseDarker: "#5E3838"
    readonly property color accentGrayMuted: "#5A5D63"
    readonly property color accentGreenBright: "#55A76A"
    readonly property color accentRedMuted: "#BD5757"
    readonly property color accentRedDark: "#9C4E4E"
    readonly property color accentRedDarker: "#7A4343"
    readonly property color accentSlateMuted: "#6F737A"
    readonly property color accentSlateDarker: "#6C707E"
    readonly property color accentGreenDarker: "#375239"

    // Extracted accent palette snapshot (resources/iconset/*.svg)
    // Generated from 2182 icons, 386 unique colors.
    readonly property var accentIconPaletteTokens: [
        { name: "accentTransparent", hex: "transparent", color: "transparent" },
        { name: "accentGrayLight", hex: "#CED0D6", color: "#CED0D6" },
        { name: "accentBlue", hex: "#548AF7", color: "#548AF7" },
        { name: "accentSlate", hex: "#43454A", color: "#43454A" },
        { name: "accentGreen", hex: "#57965C", color: "#57965C" },
        { name: "accentRed", hex: "#DB5C5C", color: "#DB5C5C" },
        { name: "accentWhite", hex: "#FFFFFF", color: "#FFFFFF" },
        { name: "accentOrangeMuted", hex: "#C77D55", color: "#C77D55" },
        { name: "accentBlueMuted", hex: "#25324D", color: "#25324D" },
        { name: "accentGreenMuted", hex: "#253627", color: "#253627" },
        { name: "accentBrownMuted", hex: "#45322B", color: "#45322B" },
        { name: "accentLightYellowVivid", hex: "#F4EC4F", color: "#F4EC4F" },
        { name: "accentRedBrownDark", hex: "#402929", color: "#402929" },
        { name: "accentPurple", hex: "#A571E6", color: "#A571E6" },
        { name: "accentYellowMuted", hex: "#D6AE58", color: "#D6AE58" },
        { name: "accentPurpleDarker", hex: "#2F2936", color: "#2F2936" },
        { name: "accentYellow", hex: "#F2C55C", color: "#F2C55C" },
        { name: "accentDeepestBlack", hex: "#000000", color: "#000000" },
        { name: "accentLighterIndigo", hex: "#B589EC", color: "#B589EC" },
        { name: "accentBrownDarker", hex: "#3D3223", color: "#3D3223" },
        { name: "accentLighterOrange", hex: "#CCA18A", color: "#CCA18A" },
        { name: "accentLightOrange", hex: "#E08855", color: "#E08855" },
        { name: "accentLightSky", hex: "#5DA8D3", color: "#5DA8D3" },
        { name: "accentSlateMuted", hex: "#6F737A", color: "#6F737A" },
        { name: "accentCharcoal", hex: "#1E1F22", color: "#1E1F22" },
        { name: "accentGray", hex: "#868A91", color: "#868A91" },
        { name: "accentDimGreen", hex: "#43A047", color: "#43A047" },
        { name: "accentLightSilver", hex: "#9DA0A8", color: "#9DA0A8" },
        { name: "accentDarkerCharcoal", hex: "#2B2D30", color: "#2B2D30" },
        { name: "accentBlueBright", hex: "#3574F0", color: "#3574F0" },
        { name: "accentLightCyanVivid", hex: "#1EDCFF", color: "#1EDCFF" },
        { name: "accentLightGreen", hex: "#76D275", color: "#76D275" },
        { name: "accentSlateDarker", hex: "#6C707E", color: "#6C707E" },
        { name: "accentPaleBlueVivid", hex: "#999DF7", color: "#999DF7" },
        { name: "accentLightRose", hex: "#DD507B", color: "#DD507B" },
        { name: "accentDimTeal", hex: "#24A394", color: "#24A394" },
        { name: "accentLightCyanVividBirch", hex: "#29B6F6", color: "#29B6F6" },
        { name: "accentBaseSkyMuted", hex: "#5D87A1", color: "#5D87A1" },
        { name: "accentDimOrange", hex: "#9C5121", color: "#9C5121" },
        { name: "accentGrayPale", hex: "#B4B8BF", color: "#B4B8BF" },
        { name: "accentLightOrangeVivid", hex: "#FB8835", color: "#FB8835" },
        { name: "accentDeepSkyVivid", hex: "#01579B", color: "#01579B" },
        { name: "accentDarkerIndigo", hex: "#2C2255", color: "#2C2255" },
        { name: "accentBaseSky", hex: "#3592C4", color: "#3592C4" },
        { name: "accentDarkerRoseMuted", hex: "#392A31", color: "#392A31" },
        { name: "accentLightCyan", hex: "#40B6E0", color: "#40B6E0" },
        { name: "accentLightYellowVividBirch", hex: "#FDEE21", color: "#FDEE21" },
        { name: "accentBaseRedVivid", hex: "#FF0D2A", color: "#FF0D2A" },
        { name: "accentLightAmberVivid", hex: "#FFA72A", color: "#FFA72A" },
        { name: "accentDarkerSky", hex: "#0F2648", color: "#0F2648" },
        { name: "accentBaseSkyBirch", hex: "#366ACF", color: "#366ACF" },
        { name: "accentDeepGraphite", hex: "#515151", color: "#515151" },
        { name: "accentGreenBright", hex: "#55A76A", color: "#55A76A" },
        { name: "accentDeepAmberMuted", hex: "#5E4D33", color: "#5E4D33" },
        { name: "accentDeepRed", hex: "#951B39", color: "#951B39" },
        { name: "accentDimRed", hex: "#A42122", color: "#A42122" },
        { name: "accentBaseRed", hex: "#D82F27", color: "#D82F27" },
        { name: "accentLightRedVivid", hex: "#EF5049", color: "#EF5049" },
        { name: "accentDarkerGreenVivid", hex: "#00701A", color: "#00701A" },
        { name: "accentDeepGreen", hex: "#187E28", color: "#187E28" },
        { name: "accentDeepestCharcoal", hex: "#1F2023", color: "#1F2023" },
        { name: "accentDimSky", hex: "#2D72B8", color: "#2D72B8" },
        { name: "accentLighterSkyVivid", hex: "#5890FF", color: "#5890FF" },
        { name: "accentGrayMuted", hex: "#5A5D63", color: "#5A5D63" },
        { name: "accentBaseGreen", hex: "#78D431", color: "#78D431" },
        { name: "accentBaseGray", hex: "#858C8C", color: "#858C8C" },
        { name: "accentDimOrangeBirch", hex: "#9D5025", color: "#9D5025" },
        { name: "accentLighterGreenVivid", hex: "#9EEE69", color: "#9EEE69" },
        { name: "accentBaseAmber", hex: "#BA9752", color: "#BA9752" },
        { name: "accentBaseRedBirch", hex: "#C62E53", color: "#C62E53" },
        { name: "accentBrightPearl", hex: "#E4E6E6", color: "#E4E6E6" },
        { name: "accentRose", hex: "#E55765", color: "#E55765" },
        { name: "accentBaseOrangeVivid", hex: "#E97826", color: "#E97826" },
        { name: "accentBrightBlueMuted", hex: "#EBECF0", color: "#EBECF0" },
        { name: "accentGrayBright", hex: "#F0F1F2", color: "#F0F1F2" },
        { name: "accentBrightBlueVivid", hex: "#F5F5FF", color: "#F5F5FF" },
        { name: "accentBaseOrangeVividBirch", hex: "#FF7800", color: "#FF7800" },
        { name: "accentBaseAmberVivid", hex: "#FFCC00", color: "#FFCC00" },
        { name: "accentDarkerCharcoalBirch", hex: "#27282E", color: "#27282E" },
        { name: "accentGreenDarker", hex: "#375239", color: "#375239" },
        { name: "accentBaseSkyCedar", hex: "#389FD6", color: "#389FD6" },
        { name: "accentLightSkyVivid", hex: "#588CF3", color: "#588CF3" },
        { name: "accentRoseDarker", hex: "#5E3838", color: "#5E3838" },
        { name: "accentBaseGreenBirch", hex: "#5FAD65", color: "#5FAD65" },
        { name: "accentLightSkyBirch", hex: "#6FAFD4", color: "#6FAFD4" },
        { name: "accentLighterSky", hex: "#75AADB", color: "#75AADB" },
        { name: "accentDeepRedBirch", hex: "#961B39", color: "#961B39" },
        { name: "accentDimMagenta", hex: "#9A3D70", color: "#9A3D70" },
        { name: "accentDimMagentaBirch", hex: "#9E2064", color: "#9E2064" },
        { name: "accentBaseRedCedar", hex: "#C92037", color: "#C92037" },
        { name: "accentBaseRedDawn", hex: "#CD2335", color: "#CD2335" },
        { name: "accentBrightSkyMuted", hex: "#DFE1E6", color: "#DFE1E6" },
        { name: "accentBaseOrangeVividCedar", hex: "#E66D17", color: "#E66D17" },
        { name: "accentLightOrangeVividBirch", hex: "#F07B3E", color: "#F07B3E" },
        { name: "accentLightOrangeVividCedar", hex: "#F58435", color: "#F58435" },
        { name: "accentLightRedVividBirch", hex: "#F8416D", color: "#F8416D" },
        { name: "accentPaleAmberVivid", hex: "#F9D88C", color: "#F9D88C" },
        { name: "accentLightOrangeVividDawn", hex: "#FC6D26", color: "#FC6D26" },
        { name: "accentLightMagentaVivid", hex: "#FF31CA", color: "#FF31CA" },
        { name: "accentDeepestSky", hex: "#0D1A2B", color: "#0D1A2B" },
        { name: "accentDeepestLimeMuted", hex: "#10110E", color: "#10110E" },
        { name: "accentDimGreenVivid", hex: "#10AA50", color: "#10AA50" },
        { name: "accentDeepGreenVivid", hex: "#12924F", color: "#12924F" },
        { name: "accentDimSkyBirch", hex: "#1B5A9A", color: "#1B5A9A" },
        { name: "accentBaseTeal", hex: "#21D789", color: "#21D789" },
        { name: "accentDimSkyCedar", hex: "#35538F", color: "#35538F" },
        { name: "accentLightSkyCedar", hex: "#417BDC", color: "#417BDC" },
        { name: "accentLightSkyVividBirch", hex: "#4285F4", color: "#4285F4" },
        { name: "accentLightSkyVividCedar", hex: "#4485F9", color: "#4485F9" },
        { name: "accentDimIndigo", hex: "#473788", color: "#473788" },
        { name: "accentLightSkyDawn", hex: "#4F93D1", color: "#4F93D1" },
        { name: "accentBaseCyan", hex: "#59A5BA", color: "#59A5BA" },
        { name: "accentDimGreenVividBirch", hex: "#5CE400", color: "#5CE400" },
        { name: "accentDimOrangeCedar", hex: "#90623E", color: "#90623E" },
        { name: "accentPaleBlueVividBirch", hex: "#999DF6", color: "#999DF6" },
        { name: "accentLightPurpleMuted", hex: "#AE72B2", color: "#AE72B2" },
        { name: "accentLighterTealMuted", hex: "#B8C4C2", color: "#B8C4C2" },
        { name: "accentBrightCyanVivid", hex: "#BBE6FB", color: "#BBE6FB" },
        { name: "accentLightRed", hex: "#DB5860", color: "#DB5860" },
        { name: "accentLightRedVividCedar", hex: "#EC3D2E", color: "#EC3D2E" },
        { name: "accentBaseRedVividBirch", hex: "#ED1C24", color: "#ED1C24" },
        { name: "accentLightRedVividDawn", hex: "#ED402F", color: "#ED402F" },
        { name: "accentLighterRedVivid", hex: "#F57B7C", color: "#F57B7C" },
        { name: "accentBrightSkyMutedBirch", hex: "#F7F8FA", color: "#F7F8FA" },
        { name: "accentLightRedVividEmber", hex: "#F8416C", color: "#F8416C" },
        { name: "accentLightOrangeVividEmber", hex: "#FE9133", color: "#FE9133" },
        { name: "accentLightOrangeVividFlint", hex: "#FF9235", color: "#FF9235" },
        { name: "accentDeepestGreenVivid", hex: "#004300", color: "#004300" },
        { name: "accentBaseSkyVivid", hex: "#007AFF", color: "#007AFF" },
        { name: "accentDimCyanVivid", hex: "#0097CB", color: "#0097CB" },
        { name: "accentDimCyanVividBirch", hex: "#00ADD7", color: "#00ADD7" },
        { name: "accentBaseCyanVivid", hex: "#00AEFF", color: "#00AEFF" },
        { name: "accentBaseSkyVividBirch", hex: "#0B8FFF", color: "#0B8FFF" },
        { name: "accentBaseSkyVividCedar", hex: "#0D94F6", color: "#0D94F6" },
        { name: "accentBaseCyanVividBirch", hex: "#11A3DE", color: "#11A3DE" },
        { name: "accentDimCyanVividCedar", hex: "#1287B1", color: "#1287B1" },
        { name: "accentDimCyanVividDawn", hex: "#18BBB7", color: "#18BBB7" },
        { name: "accentDimSkyDawn", hex: "#2376AB", color: "#2376AB" },
        { name: "accentDimCyan", hex: "#248996", color: "#248996" },
        { name: "accentDeepBlue", hex: "#282662", color: "#282662" },
        { name: "accentDimSkyEmber", hex: "#306998", color: "#306998" },
        { name: "accentDimSkyFlint", hex: "#375FAD", color: "#375FAD" },
        { name: "accentLightSkyVividDawn", hex: "#4387FB", color: "#4387FB" },
        { name: "accentBaseSkyDawn", hex: "#498DB7", color: "#498DB7" },
        { name: "accentDimGreenBirch", hex: "#499C54", color: "#499C54" },
        { name: "accentLightTealVivid", hex: "#4BFFDF", color: "#4BFFDF" },
        { name: "accentLightSkyEmber", hex: "#509CD1", color: "#509CD1" },
        { name: "accentDimCyanMuted", hex: "#59666C", color: "#59666C" },
        { name: "accentLightSkyFlint", hex: "#5A9FD4", color: "#5A9FD4" },
        { name: "accentLightCyanBirch", hex: "#60D0E7", color: "#60D0E7" },
        { name: "accentDeepOrangeMuted", hex: "#614438", color: "#614438" },
        { name: "accentDimPurple", hex: "#662E8D", color: "#662E8D" },
        { name: "accentDeepLimeVivid", hex: "#669915", color: "#669915" },
        { name: "accentDimLime", hex: "#729B1B", color: "#729B1B" },
        { name: "accentDeepRedCedar", hex: "#791514", color: "#791514" },
        { name: "accentLighterSkyVividBirch", hex: "#7AB7FE", color: "#7AB7FE" },
        { name: "accentLighterIndigoVivid", hex: "#7F52FF", color: "#7F52FF" },
        { name: "accentLighterCyanVivid", hex: "#81D8F7", color: "#81D8F7" },
        { name: "accentBaseLime", hex: "#8AC831", color: "#8AC831" },
        { name: "accentDeepRedVivid", hex: "#941F15", color: "#941F15" },
        { name: "accentLightIndigo", hex: "#9454DB", color: "#9454DB" },
        { name: "accentLightSilverBirch", hex: "#9A9996", color: "#9A9996" },
        { name: "accentDimMagentaCedar", hex: "#9F2064", color: "#9F2064" },
        { name: "accentLighterIndigoVividBirch", hex: "#A177F4", color: "#A177F4" },
        { name: "accentBaseOrange", hex: "#A36B4E", color: "#A36B4E" },
        { name: "accentBaseLimeBirch", hex: "#A9C23A", color: "#A9C23A" },
        { name: "accentDimRedBirch", hex: "#AD2524", color: "#AD2524" },
        { name: "accentLightAmber", hex: "#BCAE79", color: "#BCAE79" },
        { name: "accentBaseRedEmber", hex: "#C54D5B", color: "#C54D5B" },
        { name: "accentDimPurpleVivid", hex: "#C700D4", color: "#C700D4" },
        { name: "accentBasePurpleVivid", hex: "#C711E1", color: "#C711E1" },
        { name: "accentBaseRedFlint", hex: "#CD2032", color: "#CD2032" },
        { name: "accentBrightCyanVividBirch", hex: "#D3EFFC", color: "#D3EFFC" },
        { name: "accentDimMagentaVivid", hex: "#DD00A1", color: "#DD00A1" },
        { name: "accentBaseRoseVivid", hex: "#DD1265", color: "#DD1265" },
        { name: "accentBaseRedVividCedar", hex: "#E24329", color: "#E24329" },
        { name: "accentLightRedBirch", hex: "#E44857", color: "#E44857" },
        { name: "accentBrightPearlBirch", hex: "#E5E5E5", color: "#E5E5E5" },
        { name: "accentBaseYellowVivid", hex: "#E7C200", color: "#E7C200" },
        { name: "accentBaseAmberVividBirch", hex: "#EDA200", color: "#EDA200" },
        { name: "accentLightRedVividFlint", hex: "#EE4C2C", color: "#EE4C2C" },
        { name: "accentLightRedVividGrove", hex: "#F23838", color: "#F23838" },
        { name: "accentBaseRedVividDawn", hex: "#F40A0B", color: "#F40A0B" },
        { name: "accentBaseAmberVividCedar", hex: "#F4C20E", color: "#F4C20E" },
        { name: "accentLighterRedVividBirch", hex: "#F57C7C", color: "#F57C7C" },
        { name: "accentBaseYellowVividBirch", hex: "#F5E710", color: "#F5E710" },
        { name: "accentBaseAmberVividDawn", hex: "#F9AF17", color: "#F9AF17" },
        { name: "accentBrightRedVivid", hex: "#F9D8D2", color: "#F9D8D2" },
        { name: "accentLighterRedVividCedar", hex: "#FB5F81", color: "#FB5F81" },
        { name: "accentLightYellowVividCedar", hex: "#FBDC3E", color: "#FBDC3E" },
        { name: "accentLightAmberVividBirch", hex: "#FCA326", color: "#FCA326" },
        { name: "accentBaseAmberVividEmber", hex: "#FDB60D", color: "#FDB60D" },
        { name: "accentLighterYellowVivid", hex: "#FDEF50", color: "#FDEF50" },
        { name: "accentLighterOrangeVivid", hex: "#FE9E52", color: "#FE9E52" },
        { name: "accentLightOrangeVividGrove", hex: "#FEA42C", color: "#FEA42C" },
        { name: "accentBrightRoseVivid", hex: "#FEBBD0", color: "#FEBBD0" },
        { name: "accentBrightYellowVivid", hex: "#FFFFFE", color: "#FFFFFE" },
        { name: "accentDeepCyanVivid", hex: "#005A86", color: "#005A86" },
        { name: "accentDeepCyanVividBirch", hex: "#006995", color: "#006995" },
        { name: "accentDimSkyVivid", hex: "#0070C5", color: "#0070C5" },
        { name: "accentDimCyanVividEmber", hex: "#0086B6", color: "#0086B6" },
        { name: "accentBaseSkyVividDawn", hex: "#009DFF", color: "#009DFF" },
        { name: "accentDeepGreenVividBirch", hex: "#00A817", color: "#00A817" },
        { name: "accentBaseCyanVividCedar", hex: "#00B2E9", color: "#00B2E9" },
        { name: "accentDeepBlueVivid", hex: "#01009A", color: "#01009A" },
        { name: "accentDimSkyVividBirch", hex: "#0173D9", color: "#0173D9" },
        { name: "accentDeepTealVivid", hex: "#019486", color: "#019486" },
        { name: "accentBaseCyanVividDawn", hex: "#01AFE9", color: "#01AFE9" },
        { name: "accentDimGreenVividCedar", hex: "#01B202", color: "#01B202" },
        { name: "accentBaseCyanVividEmber", hex: "#01BDEF", color: "#01BDEF" },
        { name: "accentDeepestCyanMuted", hex: "#020303", color: "#020303" },
        { name: "accentDeepestIndigo", hex: "#030107", color: "#030107" },
        { name: "accentBaseSkyVividEmber", hex: "#0498FF", color: "#0498FF" },
        { name: "accentBaseCyanVividFlint", hex: "#06A7EB", color: "#06A7EB" },
        { name: "accentBaseCyanVividGrove", hex: "#07A6F0", color: "#07A6F0" },
        { name: "accentBaseSkyVividFlint", hex: "#087CFA", color: "#087CFA" },
        { name: "accentDimSkyVividCedar", hex: "#0F80CC", color: "#0F80CC" },
        { name: "accentBaseSkyVividGrove", hex: "#0F99EE", color: "#0F99EE" },
        { name: "accentDeepTealVividBirch", hex: "#0F9D58", color: "#0F9D58" },
        { name: "accentBaseGreenVivid", hex: "#0FE90F", color: "#0FE90F" },
        { name: "accentDarkerTeal", hex: "#176655", color: "#176655" },
        { name: "accentDeepSky", hex: "#1A476F", color: "#1A476F" },
        { name: "accentBaseSkyVividHarbor", hex: "#1B85F2", color: "#1B85F2" },
        { name: "accentLightSkyVividEmber", hex: "#1F58FF", color: "#1F58FF" },
        { name: "accentDimSkyGrove", hex: "#1F5B98", color: "#1F5B98" },
        { name: "accentDeepGreenBirch", hex: "#208A3C", color: "#208A3C" },
        { name: "accentBaseTealBirch", hex: "#27D788", color: "#27D788" },
        { name: "accentDimSkyHarbor", hex: "#2870BA", color: "#2870BA" },
        { name: "accentBaseBlue", hex: "#2A35BD", color: "#2A35BD" },
        { name: "accentDimSkyIris", hex: "#2C458F", color: "#2C458F" },
        { name: "accentLightSkyVividFlint", hex: "#2C6BF7", color: "#2C6BF7" },
        { name: "accentDeepSkyBirch", hex: "#2E436E", color: "#2E436E" },
        { name: "accentDeepSkyMuted", hex: "#34495E", color: "#34495E" },
        { name: "accentDimGreenCedar", hex: "#34A853", color: "#34A853" },
        { name: "accentLightBlueVivid", hex: "#355CFF", color: "#355CFF" },
        { name: "accentBaseBlueBirch", hex: "#3945CD", color: "#3945CD" },
        { name: "accentLightGreenVivid", hex: "#3BEA62", color: "#3BEA62" },
        { name: "accentBaseTealCedar", hex: "#3CBEB1", color: "#3CBEB1" },
        { name: "accentLightBlueVividBirch", hex: "#3F4CFE", color: "#3F4CFE" },
        { name: "accentBaseTealDawn", hex: "#41B883", color: "#41B883" },
        { name: "accentLightCyanVividCedar", hex: "#41D1FF", color: "#41D1FF" },
        { name: "accentLightBlueVividCedar", hex: "#4249FF", color: "#4249FF" },
        { name: "accentDeepGreenCedar", hex: "#49691F", color: "#49691F" },
        { name: "accentLightCyanVividDawn", hex: "#4AF7EE", color: "#4AF7EE" },
        { name: "accentLightSkyGrove", hex: "#4D77CF", color: "#4D77CF" },
        { name: "accentLighterSkyVividCedar", hex: "#4D9FFF", color: "#4D9FFF" },
        { name: "accentLightCyanCedar", hex: "#4DABCF", color: "#4DABCF" },
        { name: "accentLighterBlueVivid", hex: "#5258FF", color: "#5258FF" },
        { name: "accentLightSkyHarbor", hex: "#5294CF", color: "#5294CF" },
        { name: "accentLightSkyIris", hex: "#529FDB", color: "#529FDB" },
        { name: "accentBaseBlueCedar", hex: "#534BBA", color: "#534BBA" },
        { name: "accentLightBlue", hex: "#564DE2", color: "#564DE2" },
        { name: "accentLightGreenBirch", hex: "#59D87B", color: "#59D87B" },
        { name: "accentBaseGreenCedar", hex: "#5BC447", color: "#5BC447" },
        { name: "accentLighterSkyVividDawn", hex: "#5EA6F2", color: "#5EA6F2" },
        { name: "accentBaseGreenDawn", hex: "#60B258", color: "#60B258" },
        { name: "accentLightSkyJuniper", hex: "#6494C0", color: "#6494C0" },
        { name: "accentLightBlueVividDawn", hex: "#654FF0", color: "#654FF0" },
        { name: "accentLightSkyKite", hex: "#66A9DC", color: "#66A9DC" },
        { name: "accentDimIndigoVivid", hex: "#680AB4", color: "#680AB4" },
        { name: "accentLighterSkyVividEmber", hex: "#68AEFF", color: "#68AEFF" },
        { name: "accentLighterSkyVividFlint", hex: "#6A99FB", color: "#6A99FB" },
        { name: "accentLighterBlueVividBirch", hex: "#6C63FF", color: "#6C63FF" },
        { name: "accentLighterBlueVividCedar", hex: "#746EF7", color: "#746EF7" },
        { name: "accentLighterBlue", hex: "#7777E9", color: "#7777E9" },
        { name: "accentBaseIndigo", hex: "#7929D2", color: "#7929D2" },
        { name: "accentRedDarker", hex: "#7A4343", color: "#7A4343" },
        { name: "accentBaseIndigoBirch", hex: "#7B42BC", color: "#7B42BC" },
        { name: "accentBaseIndigoCedar", hex: "#8150BE", color: "#8150BE" },
        { name: "accentDimLimeVivid", hex: "#81BC0A", color: "#81BC0A" },
        { name: "accentDimOrangeDawn", hex: "#825845", color: "#825845" },
        { name: "accentLightGreenCedar", hex: "#85D970", color: "#85D970" },
        { name: "accentLighterCyan", hex: "#8ACCCE", color: "#8ACCCE" },
        { name: "accentDimPurpleVividBirch", hex: "#8B01D0", color: "#8B01D0" },
        { name: "accentBaseGreenEmber", hex: "#92D13D", color: "#92D13D" },
        { name: "accentLightIndigoBirch", hex: "#955AE0", color: "#955AE0" },
        { name: "accentBaseLimeCedar", hex: "#95C63D", color: "#95C63D" },
        { name: "accentBasePurpleVividBirch", hex: "#9913D4", color: "#9913D4" },
        { name: "accentRedDark", hex: "#9C4E4E", color: "#9C4E4E" },
        { name: "accentPaleSky", hex: "#9CC4E8", color: "#9CC4E8" },
        { name: "accentBasePurpleVividCedar", hex: "#A000F0", color: "#A000F0" },
        { name: "accentBasePurpleMuted", hex: "#A058A3", color: "#A058A3" },
        { name: "accentBasePurpleMutedBirch", hex: "#A05AA5", color: "#A05AA5" },
        { name: "accentLightLime", hex: "#ABDA67", color: "#ABDA67" },
        { name: "accentLighterIndigoBirch", hex: "#AF7FE4", color: "#AF7FE4" },
        { name: "accentPaleCyanVivid", hex: "#B1E4FA", color: "#B1E4FA" },
        { name: "accentDimLimeVividBirch", hex: "#B3D107", color: "#B3D107" },
        { name: "accentDimRedVivid", hex: "#B71422", color: "#B71422" },
        { name: "accentDimYellowVivid", hex: "#B7C003", color: "#B7C003" },
        { name: "accentBaseRedGrove", hex: "#B83535", color: "#B83535" },
        { name: "accentBaseRedHarbor", hex: "#B92D4E", color: "#B92D4E" },
        { name: "accentLightPurpleVivid", hex: "#BD34FE", color: "#BD34FE" },
        { name: "accentRedMuted", hex: "#BD5757", color: "#BD5757" },
        { name: "accentDimRedCedar", hex: "#C12127", color: "#C12127" },
        { name: "accentDimRedVividBirch", hex: "#C3002F", color: "#C3002F" },
        { name: "accentDimRedVividCedar", hex: "#C63D14", color: "#C63D14" },
        { name: "accentLightLimeBirch", hex: "#CADB5F", color: "#CADB5F" },
        { name: "accentDimOrangeVivid", hex: "#CC5F00", color: "#CC5F00" },
        { name: "accentLighterAmberMuted", hex: "#CCBEA7", color: "#CCBEA7" },
        { name: "accentDimPurpleVividCedar", hex: "#CD13C0", color: "#CD13C0" },
        { name: "accentLightMagenta", hex: "#CD669A", color: "#CD669A" },
        { name: "accentBaseMagentaVivid", hex: "#CF19B9", color: "#CF19B9" },
        { name: "accentBaseMagenta", hex: "#DA3A98", color: "#DA3A98" },
        { name: "accentBaseRedIris", hex: "#DB3B4B", color: "#DB3B4B" },
        { name: "accentBaseRedJuniper", hex: "#DB4437", color: "#DB4437" },
        { name: "accentDimRedVividDawn", hex: "#DD0031", color: "#DD0031" },
        { name: "accentLightRedCedar", hex: "#DE6F68", color: "#DE6F68" },
        { name: "accentLightYellow", hex: "#E1DB59", color: "#E1DB59" },
        { name: "accentBaseOrangeVividDawn", hex: "#E25A1C", color: "#E25A1C" },
        { name: "accentBaseRedVividEmber", hex: "#E30C34", color: "#E30C34" },
        { name: "accentLightRoseBirch", hex: "#E3557C", color: "#E3557C" },
        { name: "accentLightRedDawn", hex: "#E55C76", color: "#E55C76" },
        { name: "accentPaleRed", hex: "#E5B1B4", color: "#E5B1B4" },
        { name: "accentBaseMagentaVividBirch", hex: "#E70488", color: "#E70488" },
        { name: "accentLighterPurpleVivid", hex: "#E762F5", color: "#E762F5" },
        { name: "accentLighterPurpleVividBirch", hex: "#E859FF", color: "#E859FF" },
        { name: "accentLightRoseVivid", hex: "#E9478C", color: "#E9478C" },
        { name: "accentBaseRedVividFlint", hex: "#EA222E", color: "#EA222E" },
        { name: "accentLightRedVividHarbor", hex: "#EA4335", color: "#EA4335" },
        { name: "accentBaseRedVividGrove", hex: "#ED2226", color: "#ED2226" },
        { name: "accentLightRedVividIris", hex: "#ED5735", color: "#ED5735" },
        { name: "accentBasePurpleVividDawn", hex: "#EF00FF", color: "#EF00FF" },
        { name: "accentLightRedVividJuniper", hex: "#EF775A", color: "#EF775A" },
        { name: "accentBasePurpleVividEmber", hex: "#F00CEE", color: "#F00CEE" },
        { name: "accentLightPurpleVividBirch", hex: "#F229FF", color: "#F229FF" },
        { name: "accentBaseRedVividHarbor", hex: "#F25326", color: "#F25326" },
        { name: "accentBaseOrangeVividEmber", hex: "#F26522", color: "#F26522" },
        { name: "accentLightOrangeVividHarbor", hex: "#F2814F", color: "#F2814F" },
        { name: "accentLightYellowVividDawn", hex: "#F2DC55", color: "#F2DC55" },
        { name: "accentBrightWhite", hex: "#F2F2F2", color: "#F2F2F2" },
        { name: "accentBrightWhiteBirch", hex: "#F2F2F3", color: "#F2F2F3" },
        { name: "accentLightMagentaVividBirch", hex: "#F330C1", color: "#F330C1" },
        { name: "accentBaseAmberVividFlint", hex: "#F4BD19", color: "#F4BD19" },
        { name: "accentLighterRedVividDawn", hex: "#F57C7B", color: "#F57C7B" },
        { name: "accentLightRoseVividBirch", hex: "#F65098", color: "#F65098" },
        { name: "accentLightMagentaVividCedar", hex: "#F651BA", color: "#F651BA" },
        { name: "accentLightOrangeVividIris", hex: "#F68C44", color: "#F68C44" },
        { name: "accentLightOrangeVividJuniper", hex: "#F69923", color: "#F69923" },
        { name: "accentBrightOrange", hex: "#F6DECE", color: "#F6DECE" },
        { name: "accentBaseOrangeVividFlint", hex: "#F7941E", color: "#F7941E" },
        { name: "accentLightOrangeVividKite", hex: "#F79A23", color: "#F79A23" },
        { name: "accentBaseRedVividIris", hex: "#F80000", color: "#F80000" },
        { name: "accentLighterRedVividEmber", hex: "#F96B76", color: "#F96B76" },
        { name: "accentLighterRedVividFlint", hex: "#F9767C", color: "#F9767C" },
        { name: "accentPaleAmberVividBirch", hex: "#F9DD9D", color: "#F9DD9D" },
        { name: "accentLightRedVividKite", hex: "#FA4347", color: "#FA4347" },
        { name: "accentLighterRedVividGrove", hex: "#FB815A", color: "#FB815A" },
        { name: "accentLightOrangeVividLaurel", hex: "#FB9B35", color: "#FB9B35" },
        { name: "accentLightOrangeVividMeadow", hex: "#FB9C34", color: "#FB9C34" },
        { name: "accentBaseAmberVividGrove", hex: "#FBBC05", color: "#FBBC05" },
        { name: "accentBrightAmberVivid", hex: "#FBF0DF", color: "#FBF0DF" },
        { name: "accentLightOrangeVividNimbus", hex: "#FC904F", color: "#FC904F" },
        { name: "accentLightOrangeVividOak", hex: "#FC9144", color: "#FC9144" },
        { name: "accentLightAmberVividCedar", hex: "#FCC72B", color: "#FCC72B" },
        { name: "accentLighterAmberVivid", hex: "#FCDC53", color: "#FCDC53" },
        { name: "accentLighterRedVividHarbor", hex: "#FD5B5A", color: "#FD5B5A" },
        { name: "accentBaseAmberVividHarbor", hex: "#FE9C00", color: "#FE9C00" },
        { name: "accentLightOrangeVividPrairie", hex: "#FE9D35", color: "#FE9D35" },
        { name: "accentLighterOrangeVividBirch", hex: "#FE9F52", color: "#FE9F52" },
        { name: "accentLightOrangeVividQuartz", hex: "#FEA034", color: "#FEA034" },
        { name: "accentLighterOrangeVividCedar", hex: "#FEAB6C", color: "#FEAB6C" },
        { name: "accentLighterOrangeVividDawn", hex: "#FEB57E", color: "#FEB57E" },
        { name: "accentBaseRedVividJuniper", hex: "#FF0000", color: "#FF0000" },
        { name: "accentLightRoseVividCedar", hex: "#FF3F95", color: "#FF3F95" },
        { name: "accentLighterRedVividIris", hex: "#FF6164", color: "#FF6164" },
        { name: "accentBaseOrangeVividGrove", hex: "#FF7200", color: "#FF7200" },
        { name: "accentLighterRedVividJuniper", hex: "#FF775A", color: "#FF775A" },
        { name: "accentLightOrangeVividRidge", hex: "#FF9937", color: "#FF9937" },
        { name: "accentLightOrangeVividSolace", hex: "#FF9A34", color: "#FF9A34" },
        { name: "accentBaseAmberVividIris", hex: "#FF9E15", color: "#FF9E15" },
        { name: "accentBaseAmberVividJuniper", hex: "#FFA100", color: "#FFA100" },
        { name: "accentBaseAmberVividKite", hex: "#FFA800", color: "#FFA800" },
        { name: "accentLightAmberVividDawn", hex: "#FFAA1F", color: "#FFAA1F" },
        { name: "accentBaseAmberVividLaurel", hex: "#FFB504", color: "#FFB504" },
        { name: "accentBaseAmberVividMeadow", hex: "#FFB700", color: "#FFB700" },
        { name: "accentBaseAmberVividNimbus", hex: "#FFBA0F", color: "#FFBA0F" },
        { name: "accentBaseAmberVividOak", hex: "#FFC107", color: "#FFC107" },
        { name: "accentPaleOrangeVivid", hex: "#FFC39C", color: "#FFC39C" },
        { name: "accentBaseAmberVividPrairie", hex: "#FFCA00", color: "#FFCA00" },
        { name: "accentLighterAmberVividBirch", hex: "#FFDC52", color: "#FFDC52" },
        { name: "accentLightAmberVividEmber", hex: "#FFDD35", color: "#FFDD35" },
        { name: "accentLightYellowVividEmber", hex: "#FFE83D", color: "#FFE83D" },
        { name: "accentLightYellowVividFlint", hex: "#FFE83E", color: "#FFE83E" },
        { name: "accentPaleAmberVividCedar", hex: "#FFEA83", color: "#FFEA83" },
        { name: "accentBaseYellowVividCedar", hex: "#FFF915", color: "#FFF915" },
        { name: "accentBrightAmberVividBirch", hex: "#FFFEFC", color: "#FFFEFC" },
    ]

    readonly property int accentIconPaletteTokenCount: accentIconPaletteTokens.length

    //ContextMenu

    readonly property color contextMenuSurface: surface
    readonly property color contextMenuDivider: surface
    readonly property color contextMenuItemSelectedBackground: primary
    readonly property color contextMenuItemInactiveBackground: surface

    //Radius

    readonly property real radiusHairline: 0.5
    readonly property int radiusXs: 2
    readonly property int radiusSm: 4
    readonly property int radiusBase: 6
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12
    readonly property int radiusXl: 16

    //Spacing

    readonly property int gapNone: 0
    readonly property int gap2: 2
    readonly property int gap3: 3
    readonly property int gap4: 4
    readonly property int gap5: 5
    readonly property int gap6: 6
    readonly property int gap7: 7
    readonly property int gap8: 8
    readonly property int gap10: 10
    readonly property int gap12: 12
    readonly property int gap14: 14
    readonly property int gap16: 16
    readonly property int gap18: 18
    readonly property int gap20: 20
    readonly property int gap24: 24

    //Metrics

    readonly property real strokeHairline: 0.5
    readonly property real strokeThin: 1.0
    readonly property real strokeRegular: 1.5

    readonly property int controlHeightSm: 22
    readonly property int controlHeightMd: 36
    readonly property int inputMinWidth: 180
    readonly property int inputWidthMd: 206
    readonly property int buttonMinWidth: 100
    readonly property int dialogMinWidth: 280
    readonly property int dialogMaxWidth: 360
    readonly property int iconSm: 16
    readonly property int controlIndicatorSize: 18
    readonly property int toggleTrackWidth: 38
    readonly property int toggleTransitionDuration: 140
    readonly property int headerMinHeight: 56
    readonly property int headerExtraHeight: 32
    readonly property int scaffoldBlobPrimarySize: 520
    readonly property int scaffoldBlobPrimaryRadius: 260
    readonly property int scaffoldBlobPrimaryRightMargin: -140
    readonly property int scaffoldBlobPrimaryTopMargin: -200
    readonly property int scaffoldBlobSecondaryWidth: 640
    readonly property int scaffoldBlobSecondaryHeight: 380
    readonly property int scaffoldBlobSecondaryRadius: 220
    readonly property int scaffoldBlobSecondaryLeftMargin: -200
    readonly property int scaffoldBlobSecondaryBottomMargin: -180
    readonly property real scaffoldBlobSecondaryOpacity: 0.3

    readonly property int radiusControl: 5

    //TextSize

    readonly property int textTitle: 26
    readonly property int textTitleWeight: Font.Bold
    readonly property string textTitleStyleName: "Bold"
    readonly property int textTitleLineHeight: 26
    readonly property real textTitleLetterSpacing: 0
    readonly property int textTitle2: 22
    readonly property int textTitle2Weight: Font.Bold
    readonly property string textTitle2StyleName: "Bold"
    readonly property int textTitle2LineHeight: 22
    readonly property real textTitle2LetterSpacing: 0
    readonly property int textHeader: 17
    readonly property int textHeaderWeight: Font.DemiBold
    readonly property string textHeaderStyleName: "SemiBold"
    readonly property int textHeaderLineHeight: 17
    readonly property real textHeaderLetterSpacing: 0
    readonly property int textHeader2: 15
    readonly property int textHeader2Weight: Font.DemiBold
    readonly property string textHeader2StyleName: "SemiBold"
    readonly property int textHeader2LineHeight: 15
    readonly property real textHeader2LetterSpacing: 0
    readonly property int textBody: 12
    readonly property int textBodyWeight: Font.Medium
    readonly property string textBodyStyleName: "Medium"
    readonly property int textBodyLineHeight: 12
    readonly property real textBodyLetterSpacing: 0
    readonly property int textDescription: 12
    readonly property int textDescriptionWeight: Font.DemiBold
    readonly property string textDescriptionStyleName: "SemiBold"
    readonly property int textDescriptionLineHeight: 12
    readonly property real textDescriptionLetterSpacing: 0
    readonly property int textCaption: 11
    readonly property int textCaptionWeight: Font.Normal
    readonly property string textCaptionStyleName: "Regular"
    readonly property int textCaptionLineHeight: 11
    readonly property real textCaptionLetterSpacing: 0

    readonly property int textDisabled: textCaption
    readonly property int textDisabledWeight: textCaptionWeight
    readonly property string textDisabledStyleName: textCaptionStyleName
    readonly property int textDisabledLineHeight: textCaptionLineHeight
    readonly property real textDisabledLetterSpacing: textCaptionLetterSpacing

    readonly property int textOverline: textCaption
    readonly property int textOverlineWeight: textCaptionWeight
    readonly property string textOverlineStyleName: textCaptionStyleName
    readonly property int textDisplay: textTitle2
    readonly property int textDisplayWeight: textTitle2Weight
    readonly property string textDisplayStyleName: textTitle2StyleName
    readonly property int textDisplaySm: textHeader
    readonly property int textDisplaySmWeight: textHeaderWeight
    readonly property string textDisplaySmStyleName: textHeaderStyleName
    readonly property int textBodyLg: textHeader2
    readonly property int textBodyLgWeight: textHeader2Weight
    readonly property string textBodyLgStyleName: textHeader2StyleName

    function weightForTextSize(pixelSize) {
        return FontPolicy.weightForTextSize(pixelSize, textBodyWeight)
    }

    function styleNameForTextSize(pixelSize) {
        return FontPolicy.styleNameForTextSize(pixelSize, textBodyStyleName)
    }

    function isThemeTextStyleCompliant(pixelSize, weight, styleName) {
        return FontPolicy.isThemeTextStyleCompliant(pixelSize, weight, styleName)
    }
}


// API usage (external):
// import LVRS 1.0 as LV
// Rectangle { color: LV.Theme.window }

pragma Singleton
import QtQuick
import LVRS 1.0

QtObject {
    readonly property bool dark: true
    property string targetOverride: ""
    readonly property string normalizedTargetOverride: {
        const rawTarget = targetOverride === undefined || targetOverride === null
            ? ""
            : String(targetOverride).trim()
        if (rawTarget.length === 0)
            return ""
        const normalized = Platform.normalizeTarget(rawTarget)
        if (normalized.length > 0)
            return normalized
        return rawTarget.toLowerCase()
    }
    readonly property string effectiveTarget: normalizedTargetOverride.length > 0
        ? normalizedTargetOverride
        : Platform.canonicalOs
    readonly property var effectiveRuntimeProfile: Platform.runtimeProfile(effectiveTarget)
    readonly property bool mobileTarget: effectiveRuntimeProfile.mobile === true
    readonly property real metricScaleFactor: mobileTarget ? 2.0 : 1.0
    readonly property real typographyScaleFactor: metricScaleFactor

    readonly property string fontBody: FontPolicy.resolveFamily(FontPolicy.preferredFamily)
    readonly property string fontDisplay: FontPolicy.resolveFamily(FontPolicy.preferredFamily)
    readonly property string iconSetBasePath: "qrc:/qt/qml/LVRS/resources/iconset/"
    readonly property var iconNameAliases: ({
        "add": "generaladd",
        "projectstructure": "generalprojectStructure",
        "viewmoresymbolicdefault": "generalmoreHorizontal",
        "viewmoresymbolicborderless": "generalmoreHorizontal",
        "pandownsymbolicdefault": "generalchevronDown",
        "pandownsymbolicaccent": "generalchevronDownAccent",
        "pandownsymbolicborderless": "generalchevronDownBorderless",
        "pandownsymbolicdisabled": "generalchevronDownDisabled"
    })

    function normalizeIconLookupName(iconName) {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        let normalizedName = rawName.trim()
        if (normalizedName.length === 0)
            return ""
        if (normalizedName.toLowerCase().endsWith(".svg"))
            normalizedName = normalizedName.slice(0, -4)
        if (normalizedName.indexOf("/") !== -1) {
            const segments = normalizedName.split("/").filter(segment => segment.length > 0)
            normalizedName = segments.join("")
        }
        const aliasKey = normalizedName.toLowerCase()
        if (iconNameAliases[aliasKey] !== undefined)
            return iconNameAliases[aliasKey]
        return normalizedName
    }

    function iconPath(iconName) {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        const trimmedName = rawName.trim()
        if (trimmedName.length === 0)
            return ""
        if (trimmedName.indexOf(":/") !== -1)
            return trimmedName
        const resolvedName = normalizeIconLookupName(trimmedName)
        if (resolvedName.length === 0)
            return ""
        return iconSetBasePath + resolvedName + ".svg"
    }

    function scaleMetric(value) {
        const numericValue = Number(value)
        if (!isFinite(numericValue))
            return 0
        return Math.round(numericValue * metricScaleFactor)
    }

    function scaleRealMetric(value) {
        const numericValue = Number(value)
        if (!isFinite(numericValue))
            return 0
        return numericValue * metricScaleFactor
    }

    function scaleTextMetric(value) {
        const numericValue = Number(value)
        if (!isFinite(numericValue))
            return 0
        return Math.round(numericValue * typographyScaleFactor)
    }

    //Window

    readonly property color window: "#141414"

    // Panel background scale: saturation constrained to 1~3% (brighter steps are more saturated)
    readonly property color panelBackground01: "#1B1B1C"
    readonly property color panelBackground02: "#1D1D1D"
    readonly property color panelBackground03: "#1F1F20"
    readonly property color panelBackground04: "#212223"
    readonly property color panelBackground05: "#242525"
    readonly property color panelBackground06: "#262728"
    readonly property color panelBackground07: "#292A2B"
    readonly property color panelBackground08: "#2C2E2F"
    readonly property color panelBackground09: "#303232"
    readonly property color panelBackground10: "#343536"
    readonly property color panelBackground11: "#373A3B"
    readonly property color panelBackground12: "#3C3E3F"

    readonly property color windowAlt: panelBackground03
    readonly property color subSurface: panelBackground04
    readonly property color surfaceSolid: panelBackground05
    readonly property color surfaceAlt: panelBackground06
    readonly property color surfaceGhost: panelBackground02

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
    readonly property var accentPaletteTokens: [
        { name: "accentTransparent", color: "transparent" },
        { name: "accentGrayLight", color: "#CED0D6" },
        { name: "accentBlue", color: "#548AF7" },
        { name: "accentSlate", color: "#43454A" },
        { name: "accentGreen", color: "#57965C" },
        { name: "accentRed", color: "#DB5C5C" },
        { name: "accentWhite", color: "#FFFFFF" },
        { name: "accentOrangeMuted", color: "#C77D55" },
        { name: "accentBlueMuted", color: "#25324D" },
        { name: "accentGreenMuted", color: "#253627" },
        { name: "accentBrownMuted", color: "#45322B" },
        { name: "accentLightYellowVivid", color: "#F4EC4F" },
        { name: "accentRedBrownDark", color: "#402929" },
        { name: "accentPurple", color: "#A571E6" },
        { name: "accentYellowMuted", color: "#D6AE58" },
        { name: "accentPurpleDarker", color: "#2F2936" },
        { name: "accentYellow", color: "#F2C55C" },
        { name: "accentDeepestBlack", color: "#000000" },
        { name: "accentLighterIndigo", color: "#B589EC" },
        { name: "accentBrownDarker", color: "#3D3223" },
        { name: "accentLighterOrange", color: "#CCA18A" },
        { name: "accentLightOrange", color: "#E08855" },
        { name: "accentLightSky", color: "#5DA8D3" },
        { name: "accentSlateMuted", color: "#6F737A" },
        { name: "accentCharcoal", color: "#1E1F22" },
        { name: "accentGray", color: "#868A91" },
        { name: "accentDimGreen", color: "#43A047" },
        { name: "accentLightSilver", color: "#9DA0A8" },
        { name: "accentDarkerCharcoal", color: "#2B2D30" },
        { name: "accentBlueBright", color: "#3574F0" },
        { name: "accentLightCyanVivid", color: "#1EDCFF" },
        { name: "accentLightGreen", color: "#76D275" },
        { name: "accentSlateDarker", color: "#6C707E" },
        { name: "accentPaleBlueVivid", color: "#999DF7" },
        { name: "accentLightRose", color: "#DD507B" },
        { name: "accentDimTeal", color: "#24A394" },
        { name: "accentLightCyanVividBirch", color: "#29B6F6" },
        { name: "accentBaseSkyMuted", color: "#5D87A1" },
        { name: "accentDimOrange", color: "#9C5121" },
        { name: "accentGrayPale", color: "#B4B8BF" },
        { name: "accentLightOrangeVivid", color: "#FB8835" },
        { name: "accentDeepSkyVivid", color: "#01579B" },
        { name: "accentDarkerIndigo", color: "#2C2255" },
        { name: "accentBaseSky", color: "#3592C4" },
        { name: "accentDarkerRoseMuted", color: "#392A31" },
        { name: "accentLightCyan", color: "#40B6E0" },
        { name: "accentLightYellowVividBirch", color: "#FDEE21" },
        { name: "accentBaseRedVivid", color: "#FF0D2A" },
        { name: "accentLightAmberVivid", color: "#FFA72A" },
        { name: "accentDarkerSky", color: "#0F2648" },
        { name: "accentBaseSkyBirch", color: "#366ACF" },
        { name: "accentDeepGraphite", color: "#515151" },
        { name: "accentGreenBright", color: "#55A76A" },
        { name: "accentDeepAmberMuted", color: "#5E4D33" },
        { name: "accentDeepRed", color: "#951B39" },
        { name: "accentDimRed", color: "#A42122" },
        { name: "accentBaseRed", color: "#D82F27" },
        { name: "accentLightRedVivid", color: "#EF5049" },
        { name: "accentDarkerGreenVivid", color: "#00701A" },
        { name: "accentDeepGreen", color: "#187E28" },
        { name: "accentDeepestCharcoal", color: "#1F2023" },
        { name: "accentDimSky", color: "#2D72B8" },
        { name: "accentLighterSkyVivid", color: "#5890FF" },
        { name: "accentGrayMuted", color: "#5A5D63" },
        { name: "accentBaseGreen", color: "#78D431" },
        { name: "accentBaseGray", color: "#858C8C" },
        { name: "accentDimOrangeBirch", color: "#9D5025" },
        { name: "accentLighterGreenVivid", color: "#9EEE69" },
        { name: "accentBaseAmber", color: "#BA9752" },
        { name: "accentBaseRedBirch", color: "#C62E53" },
        { name: "accentBrightPearl", color: "#E4E6E6" },
        { name: "accentRose", color: "#E55765" },
        { name: "accentBaseOrangeVivid", color: "#E97826" },
        { name: "accentBrightBlueMuted", color: "#EBECF0" },
        { name: "accentGrayBright", color: "#F0F1F2" },
        { name: "accentBrightBlueVivid", color: "#F5F5FF" },
        { name: "accentBaseOrangeVividBirch", color: "#FF7800" },
        { name: "accentBaseAmberVivid", color: "#FFCC00" },
        { name: "accentDarkerCharcoalBirch", color: "#27282E" },
        { name: "accentGreenDarker", color: "#375239" },
        { name: "accentBaseSkyCedar", color: "#389FD6" },
        { name: "accentLightSkyVivid", color: "#588CF3" },
        { name: "accentRoseDarker", color: "#5E3838" },
        { name: "accentBaseGreenBirch", color: "#5FAD65" },
        { name: "accentLightSkyBirch", color: "#6FAFD4" },
        { name: "accentLighterSky", color: "#75AADB" },
        { name: "accentDeepRedBirch", color: "#961B39" },
        { name: "accentDimMagenta", color: "#9A3D70" },
        { name: "accentDimMagentaBirch", color: "#9E2064" },
        { name: "accentBaseRedCedar", color: "#C92037" },
        { name: "accentBaseRedDawn", color: "#CD2335" },
        { name: "accentBrightSkyMuted", color: "#DFE1E6" },
        { name: "accentBaseOrangeVividCedar", color: "#E66D17" },
        { name: "accentLightOrangeVividBirch", color: "#F07B3E" },
        { name: "accentLightOrangeVividCedar", color: "#F58435" },
        { name: "accentLightRedVividBirch", color: "#F8416D" },
        { name: "accentPaleAmberVivid", color: "#F9D88C" },
        { name: "accentLightOrangeVividDawn", color: "#FC6D26" },
        { name: "accentLightMagentaVivid", color: "#FF31CA" },
        { name: "accentDeepestSky", color: "#0D1A2B" },
        { name: "accentDeepestLimeMuted", color: "#10110E" },
        { name: "accentDimGreenVivid", color: "#10AA50" },
        { name: "accentDeepGreenVivid", color: "#12924F" },
        { name: "accentDimSkyBirch", color: "#1B5A9A" },
        { name: "accentBaseTeal", color: "#21D789" },
        { name: "accentDimSkyCedar", color: "#35538F" },
        { name: "accentLightSkyCedar", color: "#417BDC" },
        { name: "accentLightSkyVividBirch", color: "#4285F4" },
        { name: "accentLightSkyVividCedar", color: "#4485F9" },
        { name: "accentDimIndigo", color: "#473788" },
        { name: "accentLightSkyDawn", color: "#4F93D1" },
        { name: "accentBaseCyan", color: "#59A5BA" },
        { name: "accentDimGreenVividBirch", color: "#5CE400" },
        { name: "accentDimOrangeCedar", color: "#90623E" },
        { name: "accentPaleBlueVividBirch", color: "#999DF6" },
        { name: "accentLightPurpleMuted", color: "#AE72B2" },
        { name: "accentLighterTealMuted", color: "#B8C4C2" },
        { name: "accentBrightCyanVivid", color: "#BBE6FB" },
        { name: "accentLightRed", color: "#DB5860" },
        { name: "accentLightRedVividCedar", color: "#EC3D2E" },
        { name: "accentBaseRedVividBirch", color: "#ED1C24" },
        { name: "accentLightRedVividDawn", color: "#ED402F" },
        { name: "accentLighterRedVivid", color: "#F57B7C" },
        { name: "accentBrightSkyMutedBirch", color: "#F7F8FA" },
        { name: "accentLightRedVividEmber", color: "#F8416C" },
        { name: "accentLightOrangeVividEmber", color: "#FE9133" },
        { name: "accentLightOrangeVividFlint", color: "#FF9235" },
        { name: "accentDeepestGreenVivid", color: "#004300" },
        { name: "accentBaseSkyVivid", color: "#007AFF" },
        { name: "accentDimCyanVivid", color: "#0097CB" },
        { name: "accentDimCyanVividBirch", color: "#00ADD7" },
        { name: "accentBaseCyanVivid", color: "#00AEFF" },
        { name: "accentBaseSkyVividBirch", color: "#0B8FFF" },
        { name: "accentBaseSkyVividCedar", color: "#0D94F6" },
        { name: "accentBaseCyanVividBirch", color: "#11A3DE" },
        { name: "accentDimCyanVividCedar", color: "#1287B1" },
        { name: "accentDimCyanVividDawn", color: "#18BBB7" },
        { name: "accentDimSkyDawn", color: "#2376AB" },
        { name: "accentDimCyan", color: "#248996" },
        { name: "accentDeepBlue", color: "#282662" },
        { name: "accentDimSkyEmber", color: "#306998" },
        { name: "accentDimSkyFlint", color: "#375FAD" },
        { name: "accentLightSkyVividDawn", color: "#4387FB" },
        { name: "accentBaseSkyDawn", color: "#498DB7" },
        { name: "accentDimGreenBirch", color: "#499C54" },
        { name: "accentLightTealVivid", color: "#4BFFDF" },
        { name: "accentLightSkyEmber", color: "#509CD1" },
        { name: "accentDimCyanMuted", color: "#59666C" },
        { name: "accentLightSkyFlint", color: "#5A9FD4" },
        { name: "accentLightCyanBirch", color: "#60D0E7" },
        { name: "accentDeepOrangeMuted", color: "#614438" },
        { name: "accentDimPurple", color: "#662E8D" },
        { name: "accentDeepLimeVivid", color: "#669915" },
        { name: "accentDimLime", color: "#729B1B" },
        { name: "accentDeepRedCedar", color: "#791514" },
        { name: "accentLighterSkyVividBirch", color: "#7AB7FE" },
        { name: "accentLighterIndigoVivid", color: "#7F52FF" },
        { name: "accentLighterCyanVivid", color: "#81D8F7" },
        { name: "accentBaseLime", color: "#8AC831" },
        { name: "accentDeepRedVivid", color: "#941F15" },
        { name: "accentLightIndigo", color: "#9454DB" },
        { name: "accentLightSilverBirch", color: "#9A9996" },
        { name: "accentDimMagentaCedar", color: "#9F2064" },
        { name: "accentLighterIndigoVividBirch", color: "#A177F4" },
        { name: "accentBaseOrange", color: "#A36B4E" },
        { name: "accentBaseLimeBirch", color: "#A9C23A" },
        { name: "accentDimRedBirch", color: "#AD2524" },
        { name: "accentLightAmber", color: "#BCAE79" },
        { name: "accentBaseRedEmber", color: "#C54D5B" },
        { name: "accentDimPurpleVivid", color: "#C700D4" },
        { name: "accentBasePurpleVivid", color: "#C711E1" },
        { name: "accentBaseRedFlint", color: "#CD2032" },
        { name: "accentBrightCyanVividBirch", color: "#D3EFFC" },
        { name: "accentDimMagentaVivid", color: "#DD00A1" },
        { name: "accentBaseRoseVivid", color: "#DD1265" },
        { name: "accentBaseRedVividCedar", color: "#E24329" },
        { name: "accentLightRedBirch", color: "#E44857" },
        { name: "accentBrightPearlBirch", color: "#E5E5E5" },
        { name: "accentBaseYellowVivid", color: "#E7C200" },
        { name: "accentBaseAmberVividBirch", color: "#EDA200" },
        { name: "accentLightRedVividFlint", color: "#EE4C2C" },
        { name: "accentLightRedVividGrove", color: "#F23838" },
        { name: "accentBaseRedVividDawn", color: "#F40A0B" },
        { name: "accentBaseAmberVividCedar", color: "#F4C20E" },
        { name: "accentLighterRedVividBirch", color: "#F57C7C" },
        { name: "accentBaseYellowVividBirch", color: "#F5E710" },
        { name: "accentBaseAmberVividDawn", color: "#F9AF17" },
        { name: "accentBrightRedVivid", color: "#F9D8D2" },
        { name: "accentLighterRedVividCedar", color: "#FB5F81" },
        { name: "accentLightYellowVividCedar", color: "#FBDC3E" },
        { name: "accentLightAmberVividBirch", color: "#FCA326" },
        { name: "accentBaseAmberVividEmber", color: "#FDB60D" },
        { name: "accentLighterYellowVivid", color: "#FDEF50" },
        { name: "accentLighterOrangeVivid", color: "#FE9E52" },
        { name: "accentLightOrangeVividGrove", color: "#FEA42C" },
        { name: "accentBrightRoseVivid", color: "#FEBBD0" },
        { name: "accentBrightYellowVivid", color: "#FFFFFE" },
        { name: "accentDeepCyanVivid", color: "#005A86" },
        { name: "accentDeepCyanVividBirch", color: "#006995" },
        { name: "accentDimSkyVivid", color: "#0070C5" },
        { name: "accentDimCyanVividEmber", color: "#0086B6" },
        { name: "accentBaseSkyVividDawn", color: "#009DFF" },
        { name: "accentDeepGreenVividBirch", color: "#00A817" },
        { name: "accentBaseCyanVividCedar", color: "#00B2E9" },
        { name: "accentDeepBlueVivid", color: "#01009A" },
        { name: "accentDimSkyVividBirch", color: "#0173D9" },
        { name: "accentDeepTealVivid", color: "#019486" },
        { name: "accentBaseCyanVividDawn", color: "#01AFE9" },
        { name: "accentDimGreenVividCedar", color: "#01B202" },
        { name: "accentBaseCyanVividEmber", color: "#01BDEF" },
        { name: "accentDeepestCyanMuted", color: "#020303" },
        { name: "accentDeepestIndigo", color: "#030107" },
        { name: "accentBaseSkyVividEmber", color: "#0498FF" },
        { name: "accentBaseCyanVividFlint", color: "#06A7EB" },
        { name: "accentBaseCyanVividGrove", color: "#07A6F0" },
        { name: "accentBaseSkyVividFlint", color: "#087CFA" },
        { name: "accentDimSkyVividCedar", color: "#0F80CC" },
        { name: "accentBaseSkyVividGrove", color: "#0F99EE" },
        { name: "accentDeepTealVividBirch", color: "#0F9D58" },
        { name: "accentBaseGreenVivid", color: "#0FE90F" },
        { name: "accentDarkerTeal", color: "#176655" },
        { name: "accentDeepSky", color: "#1A476F" },
        { name: "accentBaseSkyVividHarbor", color: "#1B85F2" },
        { name: "accentLightSkyVividEmber", color: "#1F58FF" },
        { name: "accentDimSkyGrove", color: "#1F5B98" },
        { name: "accentDeepGreenBirch", color: "#208A3C" },
        { name: "accentBaseTealBirch", color: "#27D788" },
        { name: "accentDimSkyHarbor", color: "#2870BA" },
        { name: "accentBaseBlue", color: "#2A35BD" },
        { name: "accentDimSkyIris", color: "#2C458F" },
        { name: "accentLightSkyVividFlint", color: "#2C6BF7" },
        { name: "accentDeepSkyBirch", color: "#2E436E" },
        { name: "accentDeepSkyMuted", color: "#34495E" },
        { name: "accentDimGreenCedar", color: "#34A853" },
        { name: "accentLightBlueVivid", color: "#355CFF" },
        { name: "accentBaseBlueBirch", color: "#3945CD" },
        { name: "accentLightGreenVivid", color: "#3BEA62" },
        { name: "accentBaseTealCedar", color: "#3CBEB1" },
        { name: "accentLightBlueVividBirch", color: "#3F4CFE" },
        { name: "accentBaseTealDawn", color: "#41B883" },
        { name: "accentLightCyanVividCedar", color: "#41D1FF" },
        { name: "accentLightBlueVividCedar", color: "#4249FF" },
        { name: "accentDeepGreenCedar", color: "#49691F" },
        { name: "accentLightCyanVividDawn", color: "#4AF7EE" },
        { name: "accentLightSkyGrove", color: "#4D77CF" },
        { name: "accentLighterSkyVividCedar", color: "#4D9FFF" },
        { name: "accentLightCyanCedar", color: "#4DABCF" },
        { name: "accentLighterBlueVivid", color: "#5258FF" },
        { name: "accentLightSkyHarbor", color: "#5294CF" },
        { name: "accentLightSkyIris", color: "#529FDB" },
        { name: "accentBaseBlueCedar", color: "#534BBA" },
        { name: "accentLightBlue", color: "#564DE2" },
        { name: "accentLightGreenBirch", color: "#59D87B" },
        { name: "accentBaseGreenCedar", color: "#5BC447" },
        { name: "accentLighterSkyVividDawn", color: "#5EA6F2" },
        { name: "accentBaseGreenDawn", color: "#60B258" },
        { name: "accentLightSkyJuniper", color: "#6494C0" },
        { name: "accentLightBlueVividDawn", color: "#654FF0" },
        { name: "accentLightSkyKite", color: "#66A9DC" },
        { name: "accentDimIndigoVivid", color: "#680AB4" },
        { name: "accentLighterSkyVividEmber", color: "#68AEFF" },
        { name: "accentLighterSkyVividFlint", color: "#6A99FB" },
        { name: "accentLighterBlueVividBirch", color: "#6C63FF" },
        { name: "accentLighterBlueVividCedar", color: "#746EF7" },
        { name: "accentLighterBlue", color: "#7777E9" },
        { name: "accentBaseIndigo", color: "#7929D2" },
        { name: "accentRedDarker", color: "#7A4343" },
        { name: "accentBaseIndigoBirch", color: "#7B42BC" },
        { name: "accentBaseIndigoCedar", color: "#8150BE" },
        { name: "accentDimLimeVivid", color: "#81BC0A" },
        { name: "accentDimOrangeDawn", color: "#825845" },
        { name: "accentLightGreenCedar", color: "#85D970" },
        { name: "accentLighterCyan", color: "#8ACCCE" },
        { name: "accentDimPurpleVividBirch", color: "#8B01D0" },
        { name: "accentBaseGreenEmber", color: "#92D13D" },
        { name: "accentLightIndigoBirch", color: "#955AE0" },
        { name: "accentBaseLimeCedar", color: "#95C63D" },
        { name: "accentBasePurpleVividBirch", color: "#9913D4" },
        { name: "accentRedDark", color: "#9C4E4E" },
        { name: "accentPaleSky", color: "#9CC4E8" },
        { name: "accentBasePurpleVividCedar", color: "#A000F0" },
        { name: "accentBasePurpleMuted", color: "#A058A3" },
        { name: "accentBasePurpleMutedBirch", color: "#A05AA5" },
        { name: "accentLightLime", color: "#ABDA67" },
        { name: "accentLighterIndigoBirch", color: "#AF7FE4" },
        { name: "accentPaleCyanVivid", color: "#B1E4FA" },
        { name: "accentDimLimeVividBirch", color: "#B3D107" },
        { name: "accentDimRedVivid", color: "#B71422" },
        { name: "accentDimYellowVivid", color: "#B7C003" },
        { name: "accentBaseRedGrove", color: "#B83535" },
        { name: "accentBaseRedHarbor", color: "#B92D4E" },
        { name: "accentLightPurpleVivid", color: "#BD34FE" },
        { name: "accentRedMuted", color: "#BD5757" },
        { name: "accentDimRedCedar", color: "#C12127" },
        { name: "accentDimRedVividBirch", color: "#C3002F" },
        { name: "accentDimRedVividCedar", color: "#C63D14" },
        { name: "accentLightLimeBirch", color: "#CADB5F" },
        { name: "accentDimOrangeVivid", color: "#CC5F00" },
        { name: "accentLighterAmberMuted", color: "#CCBEA7" },
        { name: "accentDimPurpleVividCedar", color: "#CD13C0" },
        { name: "accentLightMagenta", color: "#CD669A" },
        { name: "accentBaseMagentaVivid", color: "#CF19B9" },
        { name: "accentBaseMagenta", color: "#DA3A98" },
        { name: "accentBaseRedIris", color: "#DB3B4B" },
        { name: "accentBaseRedJuniper", color: "#DB4437" },
        { name: "accentDimRedVividDawn", color: "#DD0031" },
        { name: "accentLightRedCedar", color: "#DE6F68" },
        { name: "accentLightYellow", color: "#E1DB59" },
        { name: "accentBaseOrangeVividDawn", color: "#E25A1C" },
        { name: "accentBaseRedVividEmber", color: "#E30C34" },
        { name: "accentLightRoseBirch", color: "#E3557C" },
        { name: "accentLightRedDawn", color: "#E55C76" },
        { name: "accentPaleRed", color: "#E5B1B4" },
        { name: "accentBaseMagentaVividBirch", color: "#E70488" },
        { name: "accentLighterPurpleVivid", color: "#E762F5" },
        { name: "accentLighterPurpleVividBirch", color: "#E859FF" },
        { name: "accentLightRoseVivid", color: "#E9478C" },
        { name: "accentBaseRedVividFlint", color: "#EA222E" },
        { name: "accentLightRedVividHarbor", color: "#EA4335" },
        { name: "accentBaseRedVividGrove", color: "#ED2226" },
        { name: "accentLightRedVividIris", color: "#ED5735" },
        { name: "accentBasePurpleVividDawn", color: "#EF00FF" },
        { name: "accentLightRedVividJuniper", color: "#EF775A" },
        { name: "accentBasePurpleVividEmber", color: "#F00CEE" },
        { name: "accentLightPurpleVividBirch", color: "#F229FF" },
        { name: "accentBaseRedVividHarbor", color: "#F25326" },
        { name: "accentBaseOrangeVividEmber", color: "#F26522" },
        { name: "accentLightOrangeVividHarbor", color: "#F2814F" },
        { name: "accentLightYellowVividDawn", color: "#F2DC55" },
        { name: "accentBrightWhite", color: "#F2F2F2" },
        { name: "accentBrightWhiteBirch", color: "#F2F2F3" },
        { name: "accentLightMagentaVividBirch", color: "#F330C1" },
        { name: "accentBaseAmberVividFlint", color: "#F4BD19" },
        { name: "accentLighterRedVividDawn", color: "#F57C7B" },
        { name: "accentLightRoseVividBirch", color: "#F65098" },
        { name: "accentLightMagentaVividCedar", color: "#F651BA" },
        { name: "accentLightOrangeVividIris", color: "#F68C44" },
        { name: "accentLightOrangeVividJuniper", color: "#F69923" },
        { name: "accentBrightOrange", color: "#F6DECE" },
        { name: "accentBaseOrangeVividFlint", color: "#F7941E" },
        { name: "accentLightOrangeVividKite", color: "#F79A23" },
        { name: "accentBaseRedVividIris", color: "#F80000" },
        { name: "accentLighterRedVividEmber", color: "#F96B76" },
        { name: "accentLighterRedVividFlint", color: "#F9767C" },
        { name: "accentPaleAmberVividBirch", color: "#F9DD9D" },
        { name: "accentLightRedVividKite", color: "#FA4347" },
        { name: "accentLighterRedVividGrove", color: "#FB815A" },
        { name: "accentLightOrangeVividLaurel", color: "#FB9B35" },
        { name: "accentLightOrangeVividMeadow", color: "#FB9C34" },
        { name: "accentBaseAmberVividGrove", color: "#FBBC05" },
        { name: "accentBrightAmberVivid", color: "#FBF0DF" },
        { name: "accentLightOrangeVividNimbus", color: "#FC904F" },
        { name: "accentLightOrangeVividOak", color: "#FC9144" },
        { name: "accentLightAmberVividCedar", color: "#FCC72B" },
        { name: "accentLighterAmberVivid", color: "#FCDC53" },
        { name: "accentLighterRedVividHarbor", color: "#FD5B5A" },
        { name: "accentBaseAmberVividHarbor", color: "#FE9C00" },
        { name: "accentLightOrangeVividPrairie", color: "#FE9D35" },
        { name: "accentLighterOrangeVividBirch", color: "#FE9F52" },
        { name: "accentLightOrangeVividQuartz", color: "#FEA034" },
        { name: "accentLighterOrangeVividCedar", color: "#FEAB6C" },
        { name: "accentLighterOrangeVividDawn", color: "#FEB57E" },
        { name: "accentBaseRedVividJuniper", color: "#FF0000" },
        { name: "accentLightRoseVividCedar", color: "#FF3F95" },
        { name: "accentLighterRedVividIris", color: "#FF6164" },
        { name: "accentBaseOrangeVividGrove", color: "#FF7200" },
        { name: "accentLighterRedVividJuniper", color: "#FF775A" },
        { name: "accentLightOrangeVividRidge", color: "#FF9937" },
        { name: "accentLightOrangeVividSolace", color: "#FF9A34" },
        { name: "accentBaseAmberVividIris", color: "#FF9E15" },
        { name: "accentBaseAmberVividJuniper", color: "#FFA100" },
        { name: "accentBaseAmberVividKite", color: "#FFA800" },
        { name: "accentLightAmberVividDawn", color: "#FFAA1F" },
        { name: "accentBaseAmberVividLaurel", color: "#FFB504" },
        { name: "accentBaseAmberVividMeadow", color: "#FFB700" },
        { name: "accentBaseAmberVividNimbus", color: "#FFBA0F" },
        { name: "accentBaseAmberVividOak", color: "#FFC107" },
        { name: "accentPaleOrangeVivid", color: "#FFC39C" },
        { name: "accentBaseAmberVividPrairie", color: "#FFCA00" },
        { name: "accentLighterAmberVividBirch", color: "#FFDC52" },
        { name: "accentLightAmberVividEmber", color: "#FFDD35" },
        { name: "accentLightYellowVividEmber", color: "#FFE83D" },
        { name: "accentLightYellowVividFlint", color: "#FFE83E" },
        { name: "accentPaleAmberVividCedar", color: "#FFEA83" },
        { name: "accentBaseYellowVividCedar", color: "#FFF915" },
        { name: "accentBrightAmberVividBirch", color: "#FFFEFC" },
    ]

    readonly property int accentPaletteTokenCount: accentPaletteTokens.length

    //ContextMenu

    readonly property color contextMenuSurface: panelBackground03
    readonly property color contextMenuDivider: panelBackground08
    readonly property color contextMenuItemSelectedBackground: primary
    readonly property color contextMenuItemInactiveBackground: panelBackground08

    //Radius

    readonly property real radiusHairline: scaleRealMetric(0.5)
    readonly property int radiusXs: scaleMetric(2)
    readonly property int radiusSm: scaleMetric(4)
    readonly property int radiusBase: scaleMetric(6)
    readonly property int radiusMd: scaleMetric(8)
    readonly property int radiusLg: scaleMetric(12)
    readonly property int radiusXl: scaleMetric(16)

    //Spacing

    readonly property int gapNone: 0
    readonly property int gap2: scaleMetric(2)
    readonly property int gap3: scaleMetric(3)
    readonly property int gap4: scaleMetric(4)
    readonly property int gap5: scaleMetric(5)
    readonly property int gap6: scaleMetric(6)
    readonly property int gap7: scaleMetric(7)
    readonly property int gap8: scaleMetric(8)
    readonly property int gap10: scaleMetric(10)
    readonly property int gap12: scaleMetric(12)
    readonly property int gap14: scaleMetric(14)
    readonly property int gap16: scaleMetric(16)
    readonly property int gap18: scaleMetric(18)
    readonly property int gap20: scaleMetric(20)
    readonly property int gap24: scaleMetric(24)

    //Metrics

    readonly property real strokeHairline: scaleRealMetric(0.5)
    readonly property real strokeThin: scaleRealMetric(1.0)
    readonly property real strokeRegular: scaleRealMetric(1.5)

    readonly property int controlHeightSm: scaleMetric(22)
    readonly property int controlHeightMd: scaleMetric(36)
    readonly property int inputMinWidth: scaleMetric(180)
    readonly property int inputWidthMd: scaleMetric(206)
    readonly property int buttonMinWidth: scaleMetric(100)
    readonly property int dialogMinWidth: scaleMetric(280)
    readonly property int dialogMaxWidth: scaleMetric(360)
    readonly property int iconSm: scaleMetric(16)
    readonly property int controlIndicatorSize: scaleMetric(18)
    readonly property int toggleTrackWidth: scaleMetric(38)
    readonly property int toggleTransitionDuration: scaleMetric(140)
    readonly property int headerMinHeight: scaleMetric(56)
    readonly property int headerExtraHeight: scaleMetric(32)
    readonly property int scaffoldBlobPrimarySize: scaleMetric(520)
    readonly property int scaffoldBlobPrimaryRadius: scaleMetric(260)
    readonly property int scaffoldBlobPrimaryRightMargin: scaleMetric(-140)
    readonly property int scaffoldBlobPrimaryTopMargin: scaleMetric(-200)
    readonly property int scaffoldBlobSecondaryWidth: scaleMetric(640)
    readonly property int scaffoldBlobSecondaryHeight: scaleMetric(380)
    readonly property int scaffoldBlobSecondaryRadius: scaleMetric(220)
    readonly property int scaffoldBlobSecondaryLeftMargin: scaleMetric(-200)
    readonly property int scaffoldBlobSecondaryBottomMargin: scaleMetric(-180)
    readonly property real scaffoldBlobSecondaryOpacity: 0.3

    readonly property int radiusControl: scaleMetric(5)

    //TextSize

    readonly property int textTitle: scaleTextMetric(26)
    readonly property int textTitleWeight: Font.Bold
    readonly property string textTitleStyleName: "Bold"
    readonly property int textTitleLineHeight: scaleTextMetric(26)
    readonly property real textTitleLetterSpacing: 0
    readonly property int textTitle2: scaleTextMetric(22)
    readonly property int textTitle2Weight: Font.Bold
    readonly property string textTitle2StyleName: "Bold"
    readonly property int textTitle2LineHeight: scaleTextMetric(22)
    readonly property real textTitle2LetterSpacing: 0
    readonly property int textHeader: scaleTextMetric(17)
    readonly property int textHeaderWeight: Font.DemiBold
    readonly property string textHeaderStyleName: "SemiBold"
    readonly property int textHeaderLineHeight: scaleTextMetric(17)
    readonly property real textHeaderLetterSpacing: 0
    readonly property int textHeader2: scaleTextMetric(15)
    readonly property int textHeader2Weight: Font.DemiBold
    readonly property string textHeader2StyleName: "SemiBold"
    readonly property int textHeader2LineHeight: scaleTextMetric(15)
    readonly property real textHeader2LetterSpacing: 0
    readonly property int textBody: scaleTextMetric(12)
    readonly property int textBodyWeight: Font.Medium
    readonly property string textBodyStyleName: "Medium"
    readonly property int textBodyLineHeight: scaleTextMetric(12)
    readonly property real textBodyLetterSpacing: 0
    readonly property int textDescription: scaleTextMetric(12)
    readonly property int textDescriptionWeight: Font.DemiBold
    readonly property string textDescriptionStyleName: "SemiBold"
    readonly property int textDescriptionLineHeight: scaleTextMetric(12)
    readonly property real textDescriptionLetterSpacing: 0
    readonly property int textCaption: scaleTextMetric(11)
    readonly property int textCaptionWeight: Font.Normal
    readonly property string textCaptionStyleName: "Regular"
    readonly property int textCaptionLineHeight: scaleTextMetric(11)
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

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
    // All targets share the authored logical sizes; mobile adds no token multiplier.
    readonly property real metricScaleFactor: 1.0
    readonly property real typographyScaleFactor: metricScaleFactor

    readonly property string fontBody: FontPolicy.resolveFamily(FontPolicy.preferredFamily)
    readonly property string fontDisplay: FontPolicy.resolveFamily(FontPolicy.preferredFamily)
    readonly property string iconSetBasePath: "qrc:/qt/qml/LVRS/resources/iconset/"
    readonly property var iconNameAliases: ({
        "add": "generaladd",
        "nodestest": "nodesTest",
        "wechat": "weChat",
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

    // BEGIN FIGMA BACKGROUND TOKENS
    // Generated from resources/figma-color-tokens.json.
    readonly property color canvas: Qt.rgba(0.06189476698637009, 0.06189476698637009, 0.06373938918113708, 1)
    readonly property color surfaceGhost: Qt.rgba(0.0696646124124527, 0.06979759782552719, 0.07046254724264145, 1)
    readonly property color windowAlt: Qt.rgba(0.1061861664056778, 0.1061861664056778, 0.1091860681772232, 1)
    readonly property color subSurface: Qt.rgba(0.12038286030292511, 0.12213718146085739, 0.12389150261878967, 1)
    readonly property color surfaceSolid: Qt.rgba(0.14418363571166992, 0.1475532352924347, 0.1475532352924347, 1)
    readonly property color surfaceAlt: Qt.rgba(0.15562701225280762, 0.15847958624362946, 0.1613321751356125, 1)
    readonly property color surfaceInteractive: Qt.rgba(0.17858824133872986, 0.17858824133872986, 0.17858824133872986, 1)
    readonly property color surfacePanel: Qt.rgba(0.17342455685138702, 0.17747892439365387, 0.1795060932636261, 1)
    readonly property color surfaceControl: Qt.rgba(0.18747973442077637, 0.192999929189682, 0.192999929189682, 1)
    readonly property color surfaceInput: Qt.rgba(0.19681522250175476, 0.1986967921257019, 0.20057833194732666, 1)
    readonly property color surfaceInputHover: Qt.rgba(0.2218484729528427, 0.23012775182724, 0.23288752138614655, 1)
    readonly property color surfaceRaised: Qt.rgba(0.24484941363334656, 0.25022611021995544, 0.2529144883155823, 1)
    readonly property color window: Qt.rgba(0.09921568632125854, 0.09921568632125854, 0.09921568632125854, 1)
    readonly property color alertGlassTint: Qt.rgba(0.1438627392053604, 0.15378431975841522, 0.16370590031147003, 0.7200000286102295)
    readonly property color materialDenseFill: Qt.rgba(0.09921568632125854, 0.09921568632125854, 0.09921568632125854, 0.75)
    readonly property color materialGlassFill: Qt.rgba(0.09921568632125854, 0.09921568632125854, 0.09921568632125854, 0.25)
    readonly property color materialBackdropBase: Qt.rgba(0.05456862971186638, 0.08929412066936493, 0.15874511003494263, 1)
    readonly property color alertCanvas: Qt.rgba(0.0694509819149971, 0.0744117721915245, 0.07937255501747131, 1)
    readonly property color alertSurface: Qt.rgba(0.1438627392053604, 0.15378431975841522, 0.16370590031147003, 1)
    // END FIGMA BACKGROUND TOKENS

    // Compatibility names remain available to existing consumers.
    readonly property color panelBackground01: canvas
    readonly property color panelBackground02: surfaceGhost
    readonly property color panelBackground03: windowAlt
    readonly property color panelBackground04: subSurface
    readonly property color panelBackground05: surfaceSolid
    readonly property color panelBackground06: surfaceAlt
    readonly property color panelBackground07: surfaceInteractive
    readonly property color panelBackground08: surfacePanel
    readonly property color panelBackground09: surfaceControl
    readonly property color panelBackground10: surfaceInput
    readonly property color panelBackground11: surfaceInputHover
    readonly property color panelBackground12: surfaceRaised

    // Text colors (Figma: Text/titleHeaderColor, Text/bodyColor, Text/descriptionColor,
    // Text/captionColor, Text/disabledColor).

    readonly property color textTokenBase: "#FFFFFF"
    readonly property real textTokenTitleHeaderOpacity: 0.9
    readonly property real textTokenBodyOpacity: 0.8
    readonly property real textTokenDescriptionOpacity: 0.6
    readonly property real textTokenCaptionOpacity: 0.5
    readonly property real textTokenDisabledOpacity: 0.3

    readonly property color textTokenTitleHeader: Qt.rgba(1, 1, 1, textTokenTitleHeaderOpacity)
    readonly property color textTokenBody: Qt.rgba(1, 1, 1, textTokenBodyOpacity)
    readonly property color textTokenDescription: Qt.rgba(1, 1, 1, textTokenDescriptionOpacity)
    readonly property color textTokenCaption: Qt.rgba(1, 1, 1, textTokenCaptionOpacity)
    readonly property color textTokenDisabled: Qt.rgba(1, 1, 1, textTokenDisabledOpacity)

    readonly property color titleHeaderColor: textTokenTitleHeader
    readonly property color bodyColor: textTokenBody
    readonly property color descriptionColor: textTokenDescription
    readonly property color captionColor: textTokenCaption
    readonly property color disabledColor: textTokenDisabled

    // Figma Tabs / Material 3 Blue dark navigation tokens.
    readonly property color mobileTabAndroidSurface: "#1e1f25"
    readonly property color mobileTabAndroidIndicator: "#404659"
    readonly property color mobileTabAndroidSelectedText: "#c0c6dc"
    readonly property color mobileTabAndroidText: "#c5c6d0"
    readonly property color mobileTabAndroidBadge: "#f2b8b5"

    // Figma MobileNavigation collection / TabBar/LVRS Mobile (1074:2064).
    readonly property int mobileNavigationHeight: 56
    readonly property int mobileNavigationTabWidth: 56
    readonly property int mobileNavigationTouchSize: 48
    readonly property int mobileNavigationIconSize: 24
    readonly property real mobileNavigationRadiusIOS: 28
    readonly property real mobileNavigationRadiusAndroid: 12
    readonly property int mobileNavigationStretchDuration: 140

    readonly property color textPrimary: titleHeaderColor
    readonly property color textSecondary: bodyColor
    readonly property color textTertiary: descriptionColor
    readonly property color textSeptenary: captionColor
    readonly property color textOctonary: disabledColor

    //Primary

    readonly property color defaultPrimary: accentBlue
    // Shared by all windows in this QML engine; configured by the app root.
    property color primaryColor: defaultPrimary
    readonly property color primary: primaryColor
    readonly property color accent: primary
    readonly property bool usingDefaultPrimary: Qt.colorEqual(primary, defaultPrimary)
    readonly property color primaryHover: usingDefaultPrimary ? accentRoleColor("Blue", "Hover") : Qt.darker(primary, 1.12)
    readonly property color primaryPressed: usingDefaultPrimary ? accentRoleColor("Blue", "Pressed") : Qt.darker(primary, 1.2)
    readonly property color accentMuted: usingDefaultPrimary ? accentBlueMuted
        : Qt.tint(surfaceInteractive, Qt.rgba(primary.r, primary.g, primary.b, primary.a * 0.25))
    readonly property color accentDetail: primaryHover
    readonly property color success: accentGreen
    readonly property color warning: accentYellow
    readonly property color danger: accentRed
    readonly property color surface: "#38383c"
    readonly property color darkGrey10: "#b4b8bf"
    readonly property color accentTint: Qt.rgba(primary.r, primary.g, primary.b, primary.a * (31 / 255))
    readonly property color dangerTint: Qt.rgba(danger.r, danger.g, danger.b, danger.a * (31 / 255))
    readonly property color primaryOverlay: Qt.rgba(primary.r, primary.g, primary.b, primary.a * (64 / 255))
    readonly property color accentOverlay: primaryOverlay
    readonly property color dangerOverlay: Qt.rgba(danger.r, danger.g, danger.b, danger.a * (89 / 255))
    readonly property color overlayBackdrop: "#59000000"
    readonly property color shadowStrong: "#40000000"
    // Standard Material/Window and Material/Panel, Figma 944:31 and 944:28.
    // Primary radial opacity is halved from the original 80% / 22% design.
    readonly property color materialTint: Qt.rgba(materialDenseFill.r, materialDenseFill.g, materialDenseFill.b, 1)
    readonly property color materialWindowFill: materialTint
    readonly property real materialWindowOpacity: 0.50
    // ApplicationWindow: an opaque 98% window / 2% app-accent mix with fine grain.
    readonly property color applicationWindowFill: window
    readonly property real applicationWindowOpacity: 1.0
    readonly property real applicationWindowAccentOpacity: 0.02
    readonly property real applicationWindowGrainStrength: 2 / 255
    readonly property real materialDenseOpacity: 0.75
    readonly property real materialGlassOpacity: 0.25
    readonly property real materialDenseBlur: scaleRealMetric(64)
    readonly property real materialGlassBlur: scaleRealMetric(16)
    // Menus reuse the window's backdrop with a lighter, more diffused coating.
    readonly property real contextMenuOpacity: 0.12
    readonly property real contextMenuBlur: materialDenseBlur
    readonly property real contextMenuAccentStrength: 0.08
    readonly property real materialIntenseOpacity: 0.40
    readonly property real materialFaintOpacity: 0.11
    readonly property real materialPanelRadius: scaleRealMetric(12)
    readonly property real materialWindowRadius: scaleRealMetric(16)
    readonly property color materialDenseEdge: Qt.rgba(1, 1, 1, 0.12)
    readonly property color materialGlassEdge: Qt.rgba(1, 1, 1, 0.22)
    // Alert / Material tokens from Figma 106:283. Typography stays on Title/Body.
    readonly property color alertGlassEdge: Qt.rgba(1, 1, 1, 0.2)
    readonly property color alertTitleColor: "#F4F5F7"
    readonly property color alertBodyColor: "#D6D9DF"
    readonly property color alertActionPrimary: primary
    readonly property color alertActionBorder: "#596168"
    readonly property color alertDivider: "#363B3F"
    readonly property color alertIconSurface: usingDefaultPrimary ? accentRoleColor("Blue", "Subtle")
        : Qt.tint(surfaceInteractive, Qt.rgba(primary.r, primary.g, primary.b, primary.a * 0.16))
    readonly property color alertIconBorder: usingDefaultPrimary ? accentRoleColor("Blue", "Outline")
        : Qt.tint(surfaceInteractive, Qt.rgba(primary.r, primary.g, primary.b, primary.a * 0.36))
    // Material/TextField semantic fills, Figma 114:179.
    readonly property color inputFieldGlassTint: Qt.rgba(surfaceInput.r, surfaceInput.g,
                                                        surfaceInput.b, 0.64)
    readonly property color inputFieldGlassTintDisabled: Qt.rgba(surfaceInput.r, surfaceInput.g,
                                                                surfaceInput.b, 0.36)
    readonly property color inputFieldGlassTintInline: Qt.rgba(surfaceInput.r, surfaceInput.g,
                                                              surfaceInput.b, 0.16)
    readonly property color inputFieldGlassReflection: Qt.rgba(1, 1, 1, 0.018)
    readonly property color strokeSoft: "#D0D7E2"

    // Compatibility names resolve to the authored ten-color accent system.
    readonly property color accentPaletteBlue: accentBlue
    readonly property color accentPalettePurple: accentPurple
    readonly property color accentPalettePink: accentRose
    readonly property color accentPaletteRed: accentRed
    readonly property color accentPaletteOrange: accentOrange
    readonly property color accentPaletteYellow: accentYellow
    readonly property color accentPaletteGreen: accentGreen
    readonly property color accentPaletteGray: "#8C8C8C"

    // Authored Figma swatches 1477:30..40, resolved through the role tokens.
    readonly property color accentOrange: accentRoleColor("Orange", "Default")
    readonly property color accentBrown: accentRoleColor("Brown", "Default")
    readonly property color accentCyan: accentRoleColor("Cyan", "Default")
    readonly property color accentTeal: accentRoleColor("Teal", "Default")

    // Canonical accents and retained artwork-specific color names.
    readonly property color accentTransparent: "transparent" // none
    readonly property color accentWhite: "#FFFFFF"
    readonly property color accentGrayLight: "#CED0D6"
    // BEGIN FIGMA ACCENT TOKENS
    // Generated from resources/figma-color-tokens.json; do not derive states from surfaces.
    readonly property var figmaAccentRoles: ({
        "blue": {
            "default": [0.1557759940624237, 0.5030806064605713, 0.8999999761581421, 1],
            "hover": [0.13908571004867554, 0.449179083108902, 0.8035714030265808, 1],
            "pressed": [0.1298133283853531, 0.4192337989807129, 0.75, 1],
            "selected": [0.1342381089925766, 0.2210642546415329, 0.3202941119670868, 1],
            "disabled": [0.1557759940624237, 0.5030806064605713, 0.8999999761581421, 0.30000001192092896],
            "subtle": [0.1316535770893097, 0.18722230195999146, 0.25072941184043884, 1],
            "outline": [0.13739700615406036, 0.2624266743659973, 0.40531763434410095, 1],
            "overlay": [0.1557759940624237, 0.5030806064605713, 0.8999999761581421, 0.250980406999588],
            "text": [0.3758823573589325, 0.6299999952316284, 0.8999999761581421, 1],
        },
        "red": {
            "default": [0.8999999761581421, 0.31362923979759216, 0.31362923979759216, 1],
            "hover": [0.8035714030265808, 0.2800261080265045, 0.2800261080265045, 1],
            "pressed": [0.75, 0.26135769486427307, 0.26135769486427307, 1],
            "selected": [0.3202941119670868, 0.1737014353275299, 0.1737014353275299, 1],
            "disabled": [0.8999999761581421, 0.31362923979759216, 0.31362923979759216, 0.30000001192092896],
            "subtle": [0.25072941184043884, 0.15691010653972626, 0.15691010653972626, 1],
            "outline": [0.40531763434410095, 0.19422417879104614, 0.19422417879104614, 1],
            "overlay": [0.8999999761581421, 0.31362923979759216, 0.31362923979759216, 0.250980406999588],
            "text": [0.8999999761581421, 0.4895404875278473, 0.4895404875278473, 1],
        },
        "green": {
            "default": [0.0815294086933136, 0.8152941465377808, 0.18000000715255737, 1],
            "hover": [0.0727941170334816, 0.7279411554336548, 0.1607142835855484, 1],
            "pressed": [0.06794117391109467, 0.679411768913269, 0.15000000596046448, 1],
            "selected": [0.11567647755146027, 0.29911765456199646, 0.1402941197156906, 1],
            "disabled": [0.0815294086933136, 0.8152941465377808, 0.18000000715255737, 0.30000001192092896],
            "subtle": [0.11977411806583405, 0.23717647790908813, 0.13552941381931305, 1],
            "outline": [0.11066824197769165, 0.37482354044914246, 0.14611764252185822, 1],
            "overlay": [0.0815294086933136, 0.8152941465377808, 0.18000000715255737, 0.250980406999588],
            "text": [0.32707059383392334, 0.840705931186676, 0.3959999978542328, 1],
        },
        "yellow": {
            "default": [0.8999999761581421, 0.7625914812088013, 0.21295729279518127, 1],
            "hover": [0.8035714030265808, 0.6808851957321167, 0.19014044106006622, 1],
            "pressed": [0.75, 0.6354928612709045, 0.17746439576148987, 1],
            "selected": [0.3202941119670868, 0.2859419882297516, 0.14853344857692719, 1],
            "disabled": [0.8999999761581421, 0.7625914812088013, 0.21295729279518127, 0.30000001192092896],
            "subtle": [0.25072941184043884, 0.2287440448999405, 0.14080259203910828, 1],
            "outline": [0.40531763434410095, 0.35585054755210876, 0.15798227488994598, 1],
            "overlay": [0.8999999761581421, 0.7625914812088013, 0.21295729279518127, 0.250980406999588],
            "text": [0.8999999761581421, 0.8038139939308167, 0.4190700948238373, 1],
        },
        "purple": {
            "default": [0.5172802805900574, 0.3259204030036926, 0.8999999761581421, 1],
            "hover": [0.461857408285141, 0.2910003662109375, 0.8035714030265808, 1],
            "pressed": [0.4310669004917145, 0.2716003358364105, 0.75, 1],
            "selected": [0.2246142029762268, 0.17677423357963562, 0.3202941119670868, 1],
            "disabled": [0.5172802805900574, 0.3259204030036926, 0.8999999761581421, 0.30000001192092896],
            "subtle": [0.18949425220489502, 0.1588766872882843, 0.25072941184043884, 1],
            "outline": [0.26753854751586914, 0.19864900410175323, 0.40531763434410095, 1],
            "overlay": [0.5172802805900574, 0.3259204030036926, 0.8999999761581421, 0.250980406999588],
            "text": [0.6320961713790894, 0.498144268989563, 0.8999999761581421, 1],
        },
        "rose": {
            "default": [0.8779597282409668, 0.30369314551353455, 0.4472598135471344, 1],
            "hover": [0.7838926315307617, 0.27115461230278015, 0.39933910965919495, 1],
            "pressed": [0.7316331267356873, 0.2530776262283325, 0.37271648645401, 1],
            "selected": [0.31478404998779297, 0.1712174117565155, 0.20710907876491547, 1],
            "disabled": [0.8779597282409668, 0.30369314551353455, 0.4472598135471344, 0.30000001192092896],
            "subtle": [0.24720297753810883, 0.15532033145427704, 0.17829099297523499, 1],
            "outline": [0.3973831534385681, 0.1906471848487854, 0.24233117699623108, 1],
            "overlay": [0.8779597282409668, 0.30369314551353455, 0.4472598135471344, 0.250980406999588],
            "text": [0.8845718502998352, 0.48258522152900696, 0.583081841468811, 1],
        },
        "orange": {
            "default": [0.8999999761581421, 0.4761454164981842, 0.2126682698726654, 1],
            "hover": [0.8035714030265808, 0.42512986063957214, 0.18988238275051117, 1],
            "pressed": [0.75, 0.3967878520488739, 0.1772235631942749, 1],
            "selected": [0.3202941119670868, 0.21433047950267792, 0.14846119284629822, 1],
            "disabled": [0.8999999761581421, 0.4761454164981842, 0.2126682698726654, 0.30000001192092896],
            "subtle": [0.25072941184043884, 0.1829126924276352, 0.14075633883476257, 1],
            "outline": [0.40531763434410095, 0.25273001194000244, 0.15787822008132935, 1],
            "overlay": [0.8999999761581421, 0.4761454164981842, 0.2126682698726654, 0.250980406999588],
            "text": [0.8999999761581421, 0.6033017635345459, 0.4188677966594696, 1],
        },
        "brown": {
            "default": [0.23544545471668243, 0.1355230212211609, 0.09870952367782593, 1],
            "hover": [0.21021915972232819, 0.121002696454525, 0.08813349902629852, 1],
            "pressed": [0.1962045580148697, 0.11293584853410721, 0.08225793391466141, 1],
            "selected": [0.15415547788143158, 0.12917488813400269, 0.11997150629758835, 1],
            "disabled": [0.23544545471668243, 0.1355230212211609, 0.09870952367782593, 0.30000001192092896],
            "subtle": [0.1444006860256195, 0.1284130960702896, 0.12252293527126312, 1],
            "outline": [0.16607801616191864, 0.13010594248771667, 0.11685307323932648, 1],
            "overlay": [0.23544545471668243, 0.1355230212211609, 0.09870952367782593, 0.250980406999588],
            "text": [0.5358718037605286, 0.4811214208602905, 0.4609502851963043, 1],
        },
        "cyan": {
            "default": [0.1947030872106552, 0.758940577507019, 0.8999999761581421, 1],
            "hover": [0.17384204268455505, 0.6776254773139954, 0.8035714030265808, 1],
            "pressed": [0.16225257515907288, 0.632450520992279, 0.75, 1],
            "selected": [0.14396989345550537, 0.28502926230430603, 0.3202941119670868, 1],
            "disabled": [0.1947030872106552, 0.758940577507019, 0.8999999761581421, 0.30000001192092896],
            "subtle": [0.13788191974163055, 0.22815990447998047, 0.25072941184043884, 1],
            "outline": [0.1514107584953308, 0.3545362651348114, 0.40531763434410095, 1],
            "overlay": [0.1947030872106552, 0.758940577507019, 0.8999999761581421, 0.250980406999588],
            "text": [0.4062921702861786, 0.8012584447860718, 0.8999999761581421, 1],
        },
        "teal": {
            "default": [0.07790151238441467, 0.7089037895202637, 0.7790151834487915, 1],
            "hover": [0.06955492496490479, 0.6329497694969177, 0.6955492496490479, 1],
            "pressed": [0.06491792947053909, 0.5907531380653381, 0.6491792798042297, 1],
            "selected": [0.11476950347423553, 0.2725200653076172, 0.29004791378974915, 1],
            "disabled": [0.07790151238441467, 0.7089037895202637, 0.7790151834487915, 0.30000001192092896],
            "subtle": [0.1191936582326889, 0.2201540172100067, 0.23137183487415314, 1],
            "outline": [0.10936219245195389, 0.33652299642562866, 0.3617630898952484, 1],
            "overlay": [0.07790151238441467, 0.7089037895202637, 0.7790151834487915, 0.250980406999588],
            "text": [0.32453107833862305, 0.7662326097488403, 0.8153105974197388, 1],
        },
    })
    readonly property var figmaIconAccentRoles: ({
        "blue": {
            "pressed": [0.11683199554681778, 0.3773104250431061, 0.675000011920929, 1],
            "selected": [0.12081430107355118, 0.19895783066749573, 0.2882646918296814, 1],
            "subtle": [0.11848821491003036, 0.16850008070468903, 0.22565646469593048, 1],
        },
        "red": {
            "pressed": [0.675000011920929, 0.23522193729877472, 0.23522193729877472, 1],
            "selected": [0.2882646918296814, 0.15633128583431244, 0.15633128583431244, 1],
            "disabled": [0.8100000023841858, 0.2822663187980652, 0.2822663187980652, 0.30000001192092896],
            "subtle": [0.22565646469593048, 0.1412190943956375, 0.1412190943956375, 1],
            "text": [0.8100000023841858, 0.4405864179134369, 0.4405864179134369, 1],
        },
        "green": {
            "pressed": [0.0611470565199852, 0.6114705801010132, 0.13500000536441803, 1],
            "selected": [0.10410883277654648, 0.26920586824417114, 0.12626470625400543, 1],
            "subtle": [0.10779670625925064, 0.2134588360786438, 0.12197647243738174, 1],
            "outline": [0.09960141777992249, 0.33734118938446045, 0.13150587677955627, 1],
        },
        "yellow": {
            "pressed": [0.675000011920929, 0.5719435811042786, 0.15971796214580536, 1],
            "selected": [0.2882646918296814, 0.2573477625846863, 0.13368010520935059, 1],
            "subtle": [0.22565646469593048, 0.2058696299791336, 0.1267223209142685, 1],
            "text": [0.8100000023841858, 0.7234326004981995, 0.3771630823612213, 1],
        },
        "purple": {
            "pressed": [0.3879602253437042, 0.24444030225276947, 0.675000011920929, 1],
            "selected": [0.2021527737379074, 0.15909680724143982, 0.2882646918296814, 1],
            "subtle": [0.17054483294487, 0.14298900961875916, 0.22565646469593048, 1],
            "text": [0.5688865780830383, 0.4483298361301422, 0.8100000023841858, 1],
        },
        "rose": {
            "pressed": [0.6584697961807251, 0.2277698665857315, 0.335444837808609, 1],
            "selected": [0.28330564498901367, 0.15409566462039948, 0.18639816343784332, 1],
            "subtle": [0.22248268127441406, 0.13978829979896545, 0.160461887717247, 1],
        },
        "orange": {
            "pressed": [0.675000011920929, 0.35710906982421875, 0.15950120985507965, 1],
            "selected": [0.2882646918296814, 0.19289742410182953, 0.13361506164073944, 1],
            "subtle": [0.22565646469593048, 0.16462141275405884, 0.12668070197105408, 1],
            "text": [0.8100000023841858, 0.5429716110229492, 0.3769810199737549, 1],
        },
        "brown": {
        },
        "cyan": {
            "pressed": [0.1460273116827011, 0.5692054629325867, 0.675000011920929, 1],
        },
        "teal": {
            "pressed": [0.058426134288311005, 0.5316778421401978, 0.5842613577842712, 1],
        },
    })
    function accentRoleColor(colorName, roleName) {
        const palette = figmaAccentRoles[String(colorName || "").toLowerCase()]
        const channels = palette === undefined ? undefined : palette[String(roleName || "Default").toLowerCase()]
        return channels === undefined ? undefined : Qt.rgba(channels[0], channels[1], channels[2], channels[3])
    }
    function iconAccentRoleColor(colorName, sourceRole) {
        const role = String(sourceRole || "Default").toLowerCase()
        if (role === "default") return accentRoleColor(colorName, "Hover")
        if (role === "hover") return accentRoleColor(colorName, "Pressed")
        const palette = figmaIconAccentRoles[String(colorName || "").toLowerCase()]
        const channels = palette === undefined ? undefined : palette[role]
        if (channels !== undefined) return Qt.rgba(channels[0], channels[1], channels[2], channels[3])
        const c = accentRoleColor(colorName, sourceRole)
        return c === undefined ? undefined : Qt.rgba(c.r * 0.9, c.g * 0.9, c.b * 0.9, c.a)
    }
    // END FIGMA ACCENT TOKENS
    readonly property color accentBlue: accentRoleColor("Blue", "Default")
    readonly property color accentRed: accentRoleColor("Red", "Default")
    readonly property color accentSlate: "#43454A"
    readonly property color accentGreen: accentRoleColor("Green", "Default")
    readonly property color accentBlueMuted: accentRoleColor("Blue", "Selected")
    readonly property color accentOrangeMuted: accentOrange
    readonly property color accentGreenMuted: "#253627"
    readonly property color accentYellow: accentRoleColor("Yellow", "Default")
    readonly property color accentRedBrownDark: "#402929"
    readonly property color accentGray: "#868A91"
    readonly property color accentYellowMuted: "#D6AE58"
    readonly property color accentBrownMuted: accentBrown
    readonly property color accentPurple: accentRoleColor("Purple", "Default")
    readonly property color accentBrownDarker: "#3D3223"
    readonly property color accentCharcoal: "#1E1F22"
    readonly property color accentGrayPale: "#B4B8BF"
    readonly property color accentBlueBright: "#3574F0"
    readonly property color accentPurpleDarker: "#2F2936"
    readonly property color accentGrayBright: "#F0F1F2"
    readonly property color accentRose: accentRoleColor("Rose", "Default")
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
    // Stable 386-entry name/color contract; authored accent paints are recolored.
    readonly property var accentPaletteTokens: [
        { name: "accentTransparent", color: "transparent" },
        { name: "accentGrayLight", color: "#CED0D6" },
        { name: "accentBlue", color: "#2880E5" },
        { name: "accentSlate", color: "#43454A" },
        { name: "accentGreen", color: "#15D02E" },
        { name: "accentRed", color: "#E55050" },
        { name: "accentWhite", color: "#FFFFFF" },
        { name: "accentOrangeMuted", color: "#E57936" },
        { name: "accentBlueMuted", color: "#223852" },
        { name: "accentGreenMuted", color: "#253627" },
        { name: "accentBrownMuted", color: "#3C2319" },
        { name: "accentLightYellowVivid", color: "#F4EC4F" },
        { name: "accentRedBrownDark", color: "#402929" },
        { name: "accentPurple", color: "#8453E5" },
        { name: "accentYellowMuted", color: "#D6AE58" },
        { name: "accentPurpleDarker", color: "#2F2936" },
        { name: "accentYellow", color: "#E5C236" },
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
        { name: "accentDimTeal", color: "#14B5C7" },
        { name: "accentLightCyanVividBirch", color: "#29B6F6" },
        { name: "accentBaseSkyMuted", color: "#5D87A1" },
        { name: "accentDimOrange", color: "#9C5121" },
        { name: "accentGrayPale", color: "#B4B8BF" },
        { name: "accentLightOrangeVivid", color: "#FB8835" },
        { name: "accentDeepSkyVivid", color: "#01579B" },
        { name: "accentDarkerIndigo", color: "#2C2255" },
        { name: "accentBaseSky", color: "#3592C4" },
        { name: "accentDarkerRoseMuted", color: "#392A31" },
        { name: "accentLightCyan", color: "#32C2E5" },
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
        { name: "accentRose", color: "#E04D72" },
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
        { name: "accentBaseSkyVivid", color: "#2880E5" },
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

    readonly property color contextMenuSurface: materialTint
    readonly property color menuDivider: surfacePanel
    readonly property color contextMenuDivider: disabledColor
    readonly property color contextMenuItemSelectedBackground: primary
    readonly property color contextMenuItemInactiveBackground: surfacePanel

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
    readonly property int iconSm: scaleMetric(18)
    readonly property int controlIndicatorSize: iconSm
    readonly property int toggleTrackWidth: scaleMetric(38)
    readonly property int toggleTransitionDuration: 320
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
    readonly property int textSubtitle: scaleTextMetric(22)
    readonly property int textSubtitleWeight: Font.Bold
    readonly property string textSubtitleStyleName: "Bold"
    readonly property int textSubtitleLineHeight: scaleTextMetric(22)
    readonly property real textSubtitleLetterSpacing: 0
    readonly property int textHeader: scaleTextMetric(17)
    readonly property int textHeaderWeight: Font.DemiBold
    readonly property string textHeaderStyleName: "SemiBold"
    readonly property int textHeaderLineHeight: scaleTextMetric(17)
    readonly property real textHeaderLetterSpacing: 0
    readonly property int textSubheader: scaleTextMetric(15)
    readonly property int textSubheaderWeight: Font.DemiBold
    readonly property string textSubheaderStyleName: "SemiBold"
    readonly property int textSubheaderLineHeight: scaleTextMetric(15)
    readonly property real textSubheaderLetterSpacing: 0
    readonly property int textBody: 13
    readonly property int textBodyWeight: Font.Medium
    readonly property string textBodyStyleName: "Medium"
    readonly property int textBodyLineHeight: 13
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

    // Previous typography role names remain read-only compatibility aliases.
    readonly property int textTitle2: textSubtitle
    readonly property int textTitle2Weight: textSubtitleWeight
    readonly property string textTitle2StyleName: textSubtitleStyleName
    readonly property int textTitle2LineHeight: textSubtitleLineHeight
    readonly property real textTitle2LetterSpacing: textSubtitleLetterSpacing
    readonly property int textHeader2: textSubheader
    readonly property int textHeader2Weight: textSubheaderWeight
    readonly property string textHeader2StyleName: textSubheaderStyleName
    readonly property int textHeader2LineHeight: textSubheaderLineHeight
    readonly property real textHeader2LetterSpacing: textSubheaderLetterSpacing

    readonly property int textDisabled: textCaption
    readonly property int textDisabledWeight: textCaptionWeight
    readonly property string textDisabledStyleName: textCaptionStyleName
    readonly property int textDisabledLineHeight: textCaptionLineHeight
    readonly property real textDisabledLetterSpacing: textCaptionLetterSpacing

    readonly property int textOverline: textCaption
    readonly property int textOverlineWeight: textCaptionWeight
    readonly property string textOverlineStyleName: textCaptionStyleName
    readonly property int textDisplay: textSubtitle
    readonly property int textDisplayWeight: textSubtitleWeight
    readonly property string textDisplayStyleName: textSubtitleStyleName
    readonly property int textDisplaySm: textHeader
    readonly property int textDisplaySmWeight: textHeaderWeight
    readonly property string textDisplaySmStyleName: textHeaderStyleName
    readonly property int textBodyLg: textSubheader
    readonly property int textBodyLgWeight: textSubheaderWeight
    readonly property string textBodyLgStyleName: textSubheaderStyleName

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

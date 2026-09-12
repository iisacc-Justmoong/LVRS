import QtQuick
import "CatalogMotion.js" as CatalogMotion

QtObject {
    id: registry

    function component(spec) {
        return {
            key: spec.key,
            label: spec.label,
            location: spec.location || "",
            summary: spec.summary || "",
            previewId: spec.previewId || "placeholder",
            usage: spec.usage || "",
            related: spec.related || [],
            docPath: spec.docPath || "",
            roleLabel: spec.roleLabel || "Component",
            notes: spec.notes || [],
            iconGlyph: spec.iconGlyph || "",
            kind: spec.kind || "component"
        }
    }

    function section(spec) {
        return {
            key: spec.key,
            label: spec.label,
            location: "",
            summary: spec.summary || "",
            previewId: spec.previewId || "section",
            usage: spec.usage || "",
            related: spec.related || [],
            docPath: "",
            roleLabel: spec.roleLabel || "Section",
            notes: spec.notes || [],
            iconGlyph: spec.iconGlyph || "",
            kind: spec.kind || "section",
            groups: spec.groups || [],
            items: spec.items || []
        }
    }

    readonly property var overview: section({
        key: "catalog-overview",
        label: "Catalog Overview",
        previewId: "overview",
        roleLabel: "Overview",
        iconGlyph: "O",
        summary: "Browse every shipped QML component and supporting type. Each page pairs a live playground with a specific interaction recipe, motion response, usage and source reference.",
        notes: [
            "Use the sidebar to browse by shell, layout, control, navigation, and surface domains.",
            "Each leaf page pairs a live preview with source path, usage snippet, and related types.",
            "Section pages summarize the types contained under that branch."
        ]
    })

    readonly property var sections: [
        section({
            key: "motion-and-support", label: "Motion & supporting types", iconGlyph: "M",
            summary: "Shared motion primitives and the supporting types used inside LVRS controls.",
            items: [
                component({"key": "motion", "label": "Motion", "location": "qml/Motion.qml", "docPath": "docs/motion.md", "previewId": "motion-lab", "roleLabel": "Motion primitive", "summary": "Shared timing, speed and reduced-motion policy for the complete LVRS component family.", "usage": "Component.onCompleted: {\n    LV.Motion.speed = 1\n    LV.Motion.reducedMotion = false\n}", "related": ["motion", "abstract-button"]}),
                component({"key": "theme", "label": "Theme", "location": "qml/Theme.qml", "docPath": "docs/motion.md", "previewId": "application-shell", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.Label { text: \"Shared accent\"; color: LV.Theme.primary }", "related": ["motion", "abstract-button"]}),
                component({"key": "window-chrome-interaction", "label": "WindowChromeInteraction", "location": "qml/WindowChromeInteraction.qml", "docPath": "docs/motion.md", "previewId": "window-shell", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.WindowChromeInteraction {\n    anchors.fill: parent\n    targetWindow: toolWindow\n    moveHandleHeight: 28\n}", "related": ["motion", "abstract-button"]}),
                component({"key": "button-method-registry", "label": "ButtonMethodRegistry", "location": "qml/components/control/buttons/ButtonMethodRegistry.qml", "docPath": "docs/motion.md", "previewId": "abstract-button", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.LabelButton {\n    text: \"Action\"\n    method: function(event) { console.log(event.trigger) }\n}\n// AbstractButton owns its method registry.", "related": ["motion", "abstract-button"]}),
                component({"key": "focus-ring", "label": "FocusRing", "location": "qml/components/control/util/FocusRing.qml", "docPath": "docs/motion.md", "previewId": "motion-lab", "roleLabel": "Motion primitive", "summary": "Noninteractive visual outline for keyboard and text-entry focus.", "usage": "LV.FocusRing { anchors.fill: parent; active: field.activeFocus }", "related": ["motion", "abstract-button"]}),
                component({"key": "interaction-motion", "label": "InteractionMotion", "location": "qml/components/control/util/InteractionMotion.qml", "docPath": "docs/motion.md", "previewId": "motion-lab", "roleLabel": "Motion primitive", "summary": "Bounded squash and rebound transform for pointer, keyboard and touch feedback.", "usage": "transform: LV.InteractionMotion {\n    target: tile\n    pressed: pointer.pressed\n    hovered: pointer.containsMouse\n}", "related": ["motion", "abstract-button"]}),
                component({"key": "spring-behavior", "label": "SpringBehavior", "location": "qml/components/control/util/SpringBehavior.qml", "docPath": "docs/motion.md", "previewId": "motion-lab", "roleLabel": "Motion primitive", "summary": "Reusable interruptible elastic Behavior for a numeric presentation property.", "usage": "rotation: expanded ? 8 : 0\nLV.SpringBehavior on rotation { duration: LV.Motion.releaseDuration }", "related": ["motion", "abstract-button"]}),
                component({"key": "state-color-behavior", "label": "StateColorBehavior", "location": "qml/components/control/util/StateColorBehavior.qml", "docPath": "docs/motion.md", "previewId": "motion-lab", "roleLabel": "Motion primitive", "summary": "Reusable short color transition for rendered state changes.", "usage": "color: selected ? LV.Theme.primary : LV.Theme.surfaceAlt\nLV.StateColorBehavior on color {}", "related": ["motion", "abstract-button"]}),
                component({"key": "list-item-composite", "label": "ListItemComposite", "location": "qml/components/navigation/ListItemComposite.qml", "docPath": "docs/motion.md", "previewId": "list-navigation", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.ListItem {\n    type: LV.ListItem.Task\n    label: \"Publish release\"\n}\n// ListItem owns the composite renderer.", "related": ["motion", "abstract-button"]}),
                component({"key": "list-item-selector", "label": "ListItemSelector", "location": "qml/components/navigation/ListItemSelector.qml", "docPath": "docs/motion.md", "previewId": "list-navigation", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.ListItem {\n    type: LV.ListItem.Select\n    selector: ({ items: [\"Draft\", \"Published\"] })\n}\n// The row owns selector state and its popup.", "related": ["motion", "abstract-button"]}),
                component({"key": "page-router-transition-driver", "label": "PageRouterTransitionDriver", "location": "qml/components/navigation/PageRouterTransitionDriver.qml", "docPath": "docs/motion.md", "previewId": "router-navigation", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.PageRouter {\n    id: router\n    interactiveTransitionSettleDuration: 360\n}\n// The router creates and owns its transition driver.", "related": ["motion", "abstract-button"]}),
                component({"key": "page-transition-controller", "label": "PageTransitionController", "location": "qml/components/navigation/PageTransitionController.qml", "docPath": "docs/motion.md", "previewId": "router-navigation", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.PageTransitionController {\n    router: pageRouter\n}\n// begin(), update(), finish() expose the router gesture contract.", "related": ["motion", "abstract-button"]}),
                component({"key": "card-image-content", "label": "CardImageContent", "location": "qml/components/surfaces/CardImageContent.qml", "docPath": "docs/motion.md", "previewId": "card-gallery", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.Card {\n    type: LV.Card.File\n    title: \"Image.png\"\n    previewSource: imageUrl\n}\n// Card owns its private image renderer.", "related": ["motion", "abstract-button"]}),
                component({"key": "card-information-content", "label": "CardInformationContent", "location": "qml/components/surfaces/CardInformationContent.qml", "docPath": "docs/motion.md", "previewId": "card-gallery", "roleLabel": "Supporting type", "summary": "Supporting LVRS type. Inspect its live consumer and interaction contract below.", "usage": "LV.Card {\n    type: LV.Card.Project\n    title: \"Studio\"\n    progress: 0.6\n}\n// Card owns its private information renderer.", "related": ["motion", "abstract-button"]})
            ]
        }),
        section({
            key: "application",
            label: "Application",
            iconGlyph: "A",
            summary: "Root window and shell types that define platform behavior, adaptive layout, and compatibility wrappers.",
            notes: [
                "Application types own the outer shell and should be the first stop for app-level composition.",
                "ApplicationWindow is the standard downstream bootstrap shell, while AppBootstrapWindow and AppShell remain compatibility wrappers."
            ],
            items: [
                component({
                    key: "app-bootstrap-window",
                    label: "AppBootstrapWindow",
                    location: "qml/AppBootstrapWindow.qml",
                    docPath: "docs/components/app/AppBootstrapWindow.md",
                    previewId: "application-shell",
                    roleLabel: "Compatibility root",
                    summary: "Compatibility wrapper around ApplicationWindow that only preconfigures a visible root window.",
                    usage: "LV.AppBootstrapWindow {\n    title: \"Workspace\"\n}",
                    related: ["application-window", "page-router", "navigator"]
                }),
                component({
                    key: "application-window",
                    label: "ApplicationWindow",
                    location: "qml/ApplicationWindow.qml",
                    docPath: "docs/components/app/ApplicationWindow.md",
                    previewId: "application-shell",
                    roleLabel: "Bootstrap root",
                    summary: "Adaptive LVRS root shell with an app primary color, window policy, bootstrap defaults, navigation scaffold, runtime wiring, and page-stack hosting.",
                    usage: "LV.ApplicationWindow {\n    visible: true\n    width: 1320\n    height: 860\n    primaryColor: \"#A571E6\"\n    title: \"Workspace\"\n    subtitle: \"Adaptive shell\"\n    pageRoutes: [{ path: \"/\", component: homePage }]\n}",
                    related: ["app-shell", "page-router", "navigator", "window-safe-area-observer"]
                }),
                component({
                    key: "app-shell",
                    label: "AppShell",
                    location: "qml/AppShell.qml",
                    docPath: "docs/components/app/AppShell.md",
                    previewId: "application-shell",
                    roleLabel: "Compatibility wrapper",
                    summary: "Thin compatibility wrapper over ApplicationWindow for older code paths that still import AppShell.",
                    usage: "LV.AppShell {\n    visible: true\n    width: 1200\n    height: 820\n    title: \"Legacy shell\"\n}",
                    related: ["application-window"]
                }),
                component({
                    key: "window",
                    label: "Window",
                    location: "qml/Window.qml",
                    previewId: "window-shell",
                    roleLabel: "Lightweight window",
                    summary: "Lower-level LVRS window wrapper for cases that need render-quality and platform policy without the application scaffold.",
                    usage: "LV.Window {\n    visible: true\n    width: 720\n    height: 480\n    title: \"Tool window\"\n}",
                    related: ["application-window", "window-safe-area-observer"]
                }),
                component({
                    key: "window-safe-area-observer",
                    label: "WindowSafeAreaObserver",
                    location: "backend/platform/windowsafeareaobserver.h",
                    docPath: "docs/components/app/WindowSafeAreaObserver.md",
                    previewId: "safe-area-observer",
                    roleLabel: "Safe-area observer",
                    summary: "QML bridge that reports the live system safe-area insets for a specific window without forcing layout into that viewport.",
                    usage: "LV.WindowSafeAreaObserver {\n    window: root\n}\n",
                    related: ["application-window", "window"]
                })
            ]
        }),
        section({
            key: "layout",
            label: "Layout",
            iconGlyph: "L",
            summary: "Structural types for headers, vertical and horizontal flow, stacking, and spacing.",
            items: [
                component({
                    key: "app-header",
                    label: "AppHeader",
                    location: "qml/components/layout/AppHeader.qml",
                    docPath: "docs/components/layout/AppHeader.md",
                    previewId: "app-header",
                    roleLabel: "Header surface",
                    summary: "Toolbar-style page header with title, subtitle, optional menu affordance, and action slot.",
                    usage: "LV.AppHeader {\n    title: \"Studio\"\n    subtitle: \"Visual catalog\"\n    menuVisible: true\n}",
                    related: ["label-button", "icon-button"]
                }),
                component({
                    key: "v-stack",
                    label: "VStack",
                    location: "qml/components/layout/VStack.qml",
                    docPath: "docs/components/layout/VStack.md",
                    previewId: "stack-layout",
                    roleLabel: "Vertical layout",
                    summary: "Convenience vertical stack for predictable spacing and alignment between child items.",
                    usage: "LV.VStack {\n    spacing: LV.Theme.gap8\n    LV.Label { text: \"One\" }\n    LV.Label { text: \"Two\" }\n}",
                    related: ["h-stack", "z-stack", "spacer"]
                }),
                component({
                    key: "h-stack",
                    label: "HStack",
                    location: "qml/components/layout/HStack.qml",
                    docPath: "docs/components/layout/HStack.md",
                    previewId: "stack-layout",
                    roleLabel: "Horizontal layout",
                    summary: "Convenience horizontal stack for tightly controlled row composition.",
                    usage: "LV.HStack {\n    spacing: LV.Theme.gap8\n    LV.Label { text: \"Left\" }\n    LV.Label { text: \"Right\" }\n}",
                    related: ["v-stack", "z-stack", "spacer"]
                }),
                component({
                    key: "z-stack",
                    label: "ZStack",
                    location: "qml/components/layout/ZStack.qml",
                    docPath: "docs/components/layout/ZStack.md",
                    previewId: "stack-layout",
                    roleLabel: "Layered layout",
                    summary: "Overlay stack for layered composition where children share the same footprint.",
                    usage: "LV.ZStack {\n    Rectangle { color: LV.Theme.surfaceAlt }\n    LV.Label { anchors.centerIn: parent; text: \"Overlay\" }\n}",
                    related: ["v-stack", "h-stack"]
                }),
                component({
                    key: "spacer",
                    label: "Spacer",
                    location: "qml/components/layout/Spacer.qml",
                    docPath: "docs/components/layout/Spacer.md",
                    previewId: "stack-layout",
                    roleLabel: "Flexible spacer",
                    summary: "Elastic filler item used to push siblings apart inside row or column layouts.",
                    usage: "LV.HStack {\n    LV.Label { text: \"Leading\" }\n    LV.Spacer { }\n    LV.Label { text: \"Trailing\" }\n}",
                    related: ["v-stack", "h-stack"]
                })
            ]
        }),
        section({
            key: "control",
            label: "Control",
            iconGlyph: "C",
            summary: "Interactive and content controls used inside shells, cards, lists, and overlays.",
            groups: [
                section({
                    key: "control-buttons",
                    label: "Buttons",
                    iconGlyph: "B",
                    summary: "Action buttons, menu triggers, segmented containers, and compact selectors.",
                    items: [
                        component({"key": "help-button", "label": "HelpButton", "location": "qml/components/control/buttons/HelpButton.qml", "docPath": "docs/components/control/HelpButton.md", "previewId": "figma-parity", "roleLabel": "Figma component", "summary": "Circular help trigger with the Figma question-mark text and 21px footprint.", "usage": "LV.HelpButton { onClicked: showHelp() }", "related": ["abstract-button", "motion"]}),
                        component({"key": "color-picker-button", "label": "ColorPickerButton", "location": "qml/components/control/buttons/ColorPickerButton.qml", "docPath": "docs/components/control/ColorPickerButton.md", "previewId": "figma-parity", "roleLabel": "Figma component", "summary": "Color trigger in three sizes, with the exported hue ring and current-color well.", "usage": "LV.ColorPickerButton { currentColor: \"#7A5AF8\"; buttonSize: LV.ColorPickerButton.Medium }", "related": ["abstract-button", "motion"]}),
                        component({
                            key: "abstract-button",
                            label: "AbstractButton",
                            location: "qml/components/control/buttons/AbstractButton.qml",
                            docPath: "docs/components/control/AbstractButton.md",
                            previewId: "abstract-button",
                            roleLabel: "Button base",
                            summary: "Shared button foundation for tone policy, disabled gating, layout metrics, and interaction states.",
                            usage: "LV.AbstractButton {\n    text: \"Action\"\n    tone: LV.AbstractButton.Primary\n}",
                            related: ["label-button", "icon-button", "alert-button"]
                        }),
                        component({
                            key: "push-button",
                            label: "PushButton",
                            location: "qml/components/control/buttons/PushButton.qml",
                            docPath: "docs/components/control/PushButton.md",
                            previewId: "button-family",
                            roleLabel: "Push action",
                            summary: "Separate push-button family with label or icon content and an 8px corner radius.",
                            usage: "LV.PushButton {\n    text: \"Save\"\n    method: function(eventData) { save() }\n}",
                            related: ["dropdown-button", "label-button", "icon-button"]
                        }),
                        component({
                            key: "dropdown-button",
                            label: "DropdownButton",
                            location: "qml/components/control/buttons/DropdownButton.qml",
                            docPath: "docs/components/control/DropdownButton.md",
                            previewId: "button-family",
                            roleLabel: "Dropdown action",
                            summary: "Separate dropdown-button family with asymmetric padding and a trailing chevron.",
                            usage: "LV.DropdownButton {\n    text: \"Open\"\n    method: function(eventData) { menu.open() }\n}",
                            related: ["push-button", "label-menu-button", "icon-menu-button"]
                        }),
                        component({
                            key: "label-button",
                            label: "LabelButton",
                            location: "qml/components/control/buttons/LabelButton.qml",
                            docPath: "docs/components/control/LabelButton.md",
                            previewId: "button-family",
                            roleLabel: "Text action",
                            summary: "Figma 22px text button with fixed Body 13 typography and accent default tone.",
                            usage: "LV.LabelButton {\n    text: \"Save\"\n}",
                            related: ["icon-button", "label-menu-button"]
                        }),
                        component({
                            key: "icon-button",
                            label: "IconButton",
                            location: "qml/components/control/buttons/IconButton.qml",
                            docPath: "docs/components/control/IconButton.md",
                            previewId: "button-family",
                            roleLabel: "Icon action",
                            summary: "Figma 22px icon button with a measured 18px frame, 2px inset, and accent default tone.",
                            usage: "LV.IconButton {\n    iconName: \"add\"\n}",
                            related: ["label-button", "icon-menu-button", "toolbar-button"]
                        }),
                        component({
                            key: "label-menu-button",
                            label: "LabelMenuButton",
                            location: "qml/components/control/buttons/LabelMenuButton.qml",
                            docPath: "docs/components/control/LabelMenuButton.md",
                            previewId: "button-family",
                            roleLabel: "Menu trigger",
                            summary: "DropdownButton label preset with 8px left padding, 2px right padding, and no label-chevron gap.",
                            usage: "LV.LabelMenuButton {\n    text: \"Options\"\n}",
                            related: ["icon-menu-button", "context-menu"]
                        }),
                        component({
                            key: "icon-menu-button",
                            label: "IconMenuButton",
                            location: "qml/components/control/buttons/IconMenuButton.qml",
                            docPath: "docs/components/control/IconMenuButton.md",
                            previewId: "button-family",
                            roleLabel: "Icon menu trigger",
                            summary: "Figma 22px icon menu trigger with two 18px frames and measured -2px overlap.",
                            usage: "LV.IconMenuButton {\n    iconName: \"projectStructure\"\n}",
                            related: ["label-menu-button", "context-menu"]
                        }),
                        component({
                            key: "label-segmented-control",
                            label: "LabelSegmentedControl",
                            location: "qml/components/control/buttons/LabelSegmentedControl.qml",
                            docPath: "docs/components/control/LabelSegmentedControl.md",
                            previewId: "segmented-control",
                            roleLabel: "Segment container",
                            summary: "Figma 206:3827 segmented shell with a 12px outer radius and 56x22 label buttons with 8px corners.",
                            usage: "LV.LabelSegmentedControl {\n    LV.LabelButton { text: \"Button\" }\n    LV.LabelButton { text: \"Button\" }\n}",
                            related: ["icon-segmented-control", "label-button"]
                        }),
                        component({
                            key: "icon-segmented-control",
                            label: "IconSegmentedControl",
                            location: "qml/components/control/buttons/IconSegmentedControl.qml",
                            docPath: "docs/components/control/IconSegmentedControl.md",
                            previewId: "segmented-control",
                            roleLabel: "Icon segment container",
                            summary: "Figma 206:3912 segmented shell with a 12px outer radius and 22x22 icon buttons with 8px corners.",
                            usage: "LV.IconSegmentedControl {\n    LV.IconButton { iconName: \"projectStructure\" }\n    LV.IconButton { iconName: \"projectStructure\" }\n}",
                            related: ["label-segmented-control", "icon-button"]
                        }),
                        component({
                            key: "combo-box",
                            label: "ComboBox",
                            location: "qml/components/control/buttons/ComboBox.qml",
                            docPath: "docs/components/control/ComboBox.md",
                            previewId: "selector-control",
                            roleLabel: "Compact selector",
                            summary: "Figma-sized selector trigger that pairs text with a Stepper indicator.",
                            usage: "LV.ComboBox {\n    text: \"Control\"\n    arrow: LV.Stepper.Down\n}",
                            related: ["stepper", "label-menu-button"]
                        }),
                        component({
                            key: "stepper",
                            label: "Stepper",
                            location: "qml/components/control/buttons/Stepper.qml",
                            docPath: "docs/components/control/Stepper.md",
                            previewId: "selector-control",
                            roleLabel: "Chevron indicator",
                            summary: "Compact chevron control with up, down, or up-down direction modes.",
                            usage: "LV.Stepper {\n    tone: LV.AbstractButton.Primary\n    arrow: LV.Stepper.UpDown\n}",
                            related: ["combo-box"]
                        })
                    ]
                }),
                section({
                    key: "control-selection",
                    label: "Selection",
                    iconGlyph: "S",
                    summary: "Binary and single-choice controls with deterministic LVRS rendering.",
                    items: [
                        component({
                            key: "check-box",
                            label: "CheckBox",
                            location: "qml/components/control/check/CheckBox.qml",
                            docPath: "docs/components/control/CheckBox.md",
                            previewId: "selection-control",
                            roleLabel: "Multi-select control",
                            summary: "Custom-painted checkbox with explicit checked, unchecked, enabled, and disabled visuals.",
                            usage: "LV.CheckBox {\n    text: \"Remember\"\n    checked: true\n}",
                            related: ["radio-button", "toggle-switch"]
                        }),
                        component({
                            key: "radio-button",
                            label: "RadioButton",
                            location: "qml/components/control/check/RadioButton.qml",
                            docPath: "docs/components/control/RadioButton.md",
                            previewId: "selection-control",
                            roleLabel: "Single-select control",
                            summary: "Deterministic radio control for single-choice groups with LVRS palette mapping.",
                            usage: "LV.RadioButton {\n    text: \"Primary\"\n    checked: true\n}",
                            related: ["check-box", "toggle-switch"]
                        }),
                        component({
                            key: "toggle-switch",
                            label: "ToggleSwitch",
                            location: "qml/components/control/check/ToggleSwitch.qml",
                            docPath: "docs/components/control/ToggleSwitch.md",
                            previewId: "toggle-switch",
                            roleLabel: "Binary switch",
                            summary: "A compact switch whose knob squashes on press, stretches in motion, and rebounds into place. Shared desktop/mobile geometry.",
                            usage: "LV.ToggleSwitch {\n    checked: true\n    transitionDuration: 320\n}",
                            related: ["check-box", "radio-button"]
                        })
                    ]
                }),
                section({
                    key: "control-display",
                    label: "Display",
                    iconGlyph: "D",
                    summary: "Text, progress, and table primitives for structured information display.",
                    items: [
                        component({
                            key: "label",
                            label: "Label",
                            location: "qml/components/control/display/Label.qml",
                            docPath: "docs/components/control/Label.md",
                            previewId: "label-display",
                            roleLabel: "Typography wrapper",
                            summary: "LVRS text wrapper that maps semantic style tokens to concrete typography metrics.",
                            usage: "LV.Label {\n    style: body\n    text: \"Status\"\n}",
                            related: ["app-header", "app-card"]
                        }),
                        component({
                            key: "progress-bar",
                            label: "ProgressBar",
                            location: "qml/components/control/display/ProgressBar.qml",
                            docPath: "docs/components/control/ProgressBar.md",
                            previewId: "progress-display",
                            roleLabel: "Progress indicator",
                            summary: "Simple deterministic progress bar with regular and large size contracts.",
                            usage: "LV.ProgressBar {\n    width: 180\n    currentValue: 64\n    endValue: 100\n}",
                            related: ["label"]
                        }),
                        component({
                            key: "table",
                            label: "Table",
                            location: "qml/components/control/display/Table.qml",
                            docPath: "docs/components/control/Table.md",
                            previewId: "table-display",
                            roleLabel: "Composite table",
                            summary: "Spreadsheet surface with typed columns, range selection, TSV exchange, sorting, resizing, and atomic undo.",
                            usage: "LV.Table {\n    columns: [{ label: \"Name\" }, { label: \"Count\", type: \"int\" }]\n    model: [[{ value: \"Renderer\" }, { value: 3 }]]\n    editable: true\n    sortingEnabled: true\n}",
                            related: ["table-header", "table-row", "table-cell-item"]
                        }),
                        component({
                            key: "table-header",
                            label: "TableHeader",
                            location: "qml/components/control/display/TableHeader.qml",
                            docPath: "docs/components/control/TableHeader.md",
                            previewId: "table-display",
                            roleLabel: "Table primitive",
                            summary: "Standalone header strip used when composing tables manually for parity testing.",
                            usage: "LV.TableHeader {\n    cellItems: [{ label: \"Name\" }, { label: \"Count\", type: \"int\" }]\n    interactive: true\n}",
                            related: ["table", "table-row"]
                        }),
                        component({
                            key: "table-row",
                            label: "TableRow",
                            location: "qml/components/control/display/TableRow.qml",
                            docPath: "docs/components/control/TableRow.md",
                            previewId: "table-display",
                            roleLabel: "Table primitive",
                            summary: "Row primitive that renders an array of cell values in LVRS table rhythm.",
                            usage: "LV.TableRow {\n    cellItems: [{ value: \"Renderer\" }, { value: \"Active\" }]\n}",
                            related: ["table", "table-cell-item"]
                        }),
                        component({
                            key: "table-cell-item",
                            label: "TableCellItem",
                            location: "qml/components/control/display/TableCellItem.qml",
                            docPath: "docs/components/control/TableCellItem.md",
                            previewId: "table-display",
                            roleLabel: "Table cell",
                            summary: "Editable table cell primitive used by TableRow and manual table composition.",
                            usage: "LV.TableCellItem {\n    text: \"Renderer\"\n    selected: true\n}",
                            related: ["table", "table-row"]
                        })
                    ]
                }),
                section({
                    key: "control-input",
                    label: "Input",
                    iconGlyph: "I",
                    summary: "Text and value input components with deterministic LVRS behavior.",
                    items: [
                        component({
                            key: "slider",
                            label: "Slider",
                            location: "qml/components/control/input/Slider.qml",
                            docPath: "docs/components/control/Slider.md",
                            previewId: "slider-gallery",
                            roleLabel: "Value input",
                            summary: "Seven Figma types, four sizes, native pointer/touch/keyboard input, centered fills, endpoint labels, and segmented stops.",
                            usage: "LV.Slider {\n    type: LV.Slider.CenterBiasedTicks\n    size: LV.Slider.Regular\n    from: -1\n    to: 1\n    value: 0\n    Accessible.name: \"Exposure\"\n    onMoved: model.exposure = value\n}",
                            related: ["progress-bar", "stepper", "input-field"]
                        }),
                        component({
                            key: "color-picker",
                            label: "ColorPicker",
                            location: "qml/components/control/input/ColorPicker.qml",
                            docPath: "docs/components/control/ColorPicker.md",
                            previewId: "color-picker-gallery",
                            roleLabel: "Color input view",
                            summary: "Six content-only color editors with HSV, RGB, CMYK, Hex, alpha and recent colors. Pass the view to a host through Component.",
                            usage: "Component {\n    id: colorView\n    LV.ColorPicker {\n        type: LV.ColorPicker.SaturationBrightness\n        currentColor: document.color\n        onColorEdited: color => document.color = color\n    }\n}\nLV.Modal {\n    contentComponent: colorView\n    showIcon: false\n    primaryText: \"\"\n}",
                            related: ["slider", "input-field", "modal"]
                        }),
                        component({
                            key: "abstract-input-bar",
                            label: "AbstractInputBar",
                            location: "qml/components/control/input/AbstractInputBar.qml",
                            previewId: "input-field",
                            roleLabel: "Input base",
                            summary: "FocusScope-based input foundation that exposes TextInput APIs, slots, and visual policy for one-line entry controls.",
                            usage: "LV.AbstractInputBar {\n    placeholderText: \"Type here\"\n    text: \"LVRS\"\n}",
                            related: ["input-field", "text-editor", "code-editor"]
                        }),
                        component({
                            key: "input-field",
                            label: "InputField",
                            location: "qml/components/control/input/InputField.qml",
                            docPath: "docs/components/control/InputField.md",
                            previewId: "input-field",
                            roleLabel: "Single-line input",
                            summary: "22px one-line input with centered Body 13 typography, Disabled-token placeholders, and Rounded, Cylinder, and Inline frames.",
                            usage: "LV.InputField {\n    style: cylinderStyle\n    search: true\n    placeholderText: \"Search\"\n}",
                            related: ["abstract-input-bar", "text-editor"]
                        }),
                        component({
                            key: "text-editor",
                            label: "TextEditor",
                            location: "qml/components/control/input/TextEditor.qml",
                            docPath: "docs/components/control/TextEditor.md",
                            previewId: "text-editor",
                            roleLabel: "Multi-line editor",
                            summary: "File-connected plain-text multi-line editor with automatic read-through/write-through synchronization, chunked lazy loading, and embedded guard helpers.",
                            usage: "LV.TextEditor {\n    filePath: \"/tmp/notes.txt\"\n    chunkSize: 65536\n    onSyncFinished: console.log(path)\n}",
                            related: ["code-editor", "input-method-guard", "wheel-scroll-guard"]
                        }),
                        component({
                            key: "code-editor",
                            label: "CodeEditor",
                            location: "qml/components/control/input/CodeEditor.qml",
                            docPath: "docs/components/control/CodeEditor.md",
                            previewId: "code-editor",
                            roleLabel: "Code editor",
                            summary: "Plain-text code editor with optional snippet header and no-wrap behavior.",
                            usage: "LV.CodeEditor {\n    editorHeight: 180\n    snippetTitle: \"main.qml\"\n    text: \"import QtQuick\"\n}",
                            related: ["text-editor", "input-method-guard", "wheel-scroll-guard"]
                        })
                    ]
                }),
                section({
                    key: "control-utilities",
                    label: "Utilities",
                    iconGlyph: "U",
                    summary: "Invisible helpers that bridge events, IME composition, and wheel routing into visual controls.",
                    items: [
                        component({
                            key: "event-listener",
                            label: "EventListener",
                            location: "qml/components/control/util/EventListener.qml",
                            docPath: "docs/components/control/EventListener.md",
                            previewId: "event-listener",
                            roleLabel: "Interaction bridge",
                            summary: "Invisible incident-driven listener that converts pointer, keyboard, wheel, and global input into callback payloads.",
                            usage: "LV.Label {\n    text: \"Click me\"\n    LV.EventListener { trigger: \"clicked\"; action: function(eventData) { console.log(eventData.x) } }\n}",
                            related: ["input-method-guard", "wheel-scroll-guard"]
                        }),
                        component({
                            key: "input-method-guard",
                            label: "InputMethodGuard",
                            location: "qml/components/control/util/InputMethodGuard.qml",
                            docPath: "docs/components/control/InputMethodGuard.md",
                            previewId: "guard-utility",
                            roleLabel: "IME guard",
                            summary: "Composition-state guard for committing IME text when focus or visibility transitions would otherwise leave residue.",
                            usage: "LV.InputMethodGuard {\n    target: editor\n    guardEnabled: enabled && !readOnly\n}",
                            related: ["text-editor", "code-editor", "wheel-scroll-guard"]
                        }),
                        component({
                            key: "wheel-scroll-guard",
                            label: "WheelScrollGuard",
                            location: "qml/components/control/util/WheelScrollGuard.qml",
                            docPath: "docs/components/control/WheelScrollGuard.md",
                            previewId: "guard-utility",
                            roleLabel: "Wheel router",
                            summary: "Routes wheel events into an inner Flickable so nested scroll regions behave predictably.",
                            usage: "LV.WheelScrollGuard {\n    targetFlickable: innerFlickable\n    consumeInside: true\n}",
                            related: ["text-editor", "code-editor", "input-method-guard"]
                        })
                    ]
                })
            ]
        }),
        section({
            key: "navigation",
            label: "Navigation",
            iconGlyph: "N",
            summary: "Routing, hierarchy browsing, list panels, and menu surfaces for navigation-heavy tooling.",
            groups: [
                section({
                    key: "navigation-routing",
                    label: "Routing",
                    iconGlyph: "R",
                    summary: "Global navigation singleton, route stack host, and declarative link trigger.",
                    items: [
                        component({
                            key: "navigator",
                            label: "Navigator",
                            location: "qml/components/navigation/Navigator.qml",
                            docPath: "docs/components/navigation/Navigator.md",
                            previewId: "router-navigation",
                            roleLabel: "Navigation singleton",
                            summary: "Global router delegate that forwards navigation calls to the active PageRouter.",
                            usage: "LV.LabelButton {\n    text: \"Open reports\"\n    onClicked: LV.Navigator.go(\"/reports\")\n}",
                            related: ["page-router", "link"]
                        }),
                        component({
                            key: "page-router",
                            label: "PageRouter",
                            location: "qml/components/navigation/PageRouter.qml",
                            docPath: "docs/components/navigation/PageRouter.md",
                            previewId: "router-navigation",
                            roleLabel: "Route stack host",
                            summary: "StackView-based route resolver that owns page history, params, and route-component mapping.",
                            usage: "LV.PageRouter {\n    routes: [{ path: \"/\", component: homePage }]\n    initialPath: \"/\"\n}",
                            related: ["navigator", "link", "application-window"]
                        }),
                        component({
                            key: "link",
                            label: "Link",
                            location: "qml/components/navigation/Link.qml",
                            docPath: "docs/components/navigation/Link.md",
                            previewId: "router-navigation",
                            roleLabel: "Declarative link",
                            summary: "AbstractButton-based navigation trigger for route paths or component targets.",
                            usage: "LV.Link {\n    router: router\n    href: \"/reports\"\n    text: \"Reports\"\n}",
                            related: ["page-router", "navigator"]
                        })
                    ]
                }),
                section({
                    key: "navigation-hierarchy",
                    label: "Hierarchy",
                    iconGlyph: "H",
                    summary: "Tree navigation surface plus the toolbar, list manager, row delegate, and toolbar button primitive that power it.",
                    items: [
                        component({
                            key: "toolbar-button",
                            label: "ToolbarButton",
                            location: "qml/components/navigation/ToolbarButton.qml",
                            previewId: "hierarchy-navigation",
                            roleLabel: "Toolbar primitive",
                            summary: "IconButton-derived toolbar primitive that cooperates with HierarchyToolbar selection state.",
                            usage: "LV.ToolbarButton {\n    buttonId: \"expand\"\n    iconGlyph: \"+\"\n}",
                            related: ["hierarchy-toolbar", "icon-button"]
                        }),
                        component({
                            key: "hierarchy-toolbar",
                            label: "HierarchyToolbar",
                            location: "qml/components/navigation/HierarchyToolbar.qml",
                            docPath: "docs/components/navigation/HierarchyToolbar.md",
                            previewId: "hierarchy-navigation",
                            roleLabel: "Toolbar strip",
                            summary: "Top toolbar for hierarchy panels with array-driven icon slots and active state handling.",
                            usage: "LV.HierarchyToolbar {\n    buttonItems: [{ id: \"layers\", iconName: \"projectStructure\" }]\n}",
                            related: ["toolbar-button", "hierarchy"]
                        }),
                        component({
                            key: "hierarchy",
                            label: "Hierarchy",
                            location: "qml/components/navigation/Hierarchy.qml",
                            docPath: "docs/components/navigation/Hierarchy.md",
                            previewId: "hierarchy-navigation",
                            roleLabel: "Tree panel",
                            summary: "Composite tree panel that renders a flat depth array and forwards item-driven drag/drop moves through the bound model.",
                            usage: "LV.Hierarchy {\n    editable: true\n    model: [\n        { key: \"root\", depth: 0, label: \"Root\", expanded: true },\n        { key: \"camera\", depth: 1, label: \"Camera\" }\n    ]\n    onListItemMoved: function(item, itemId, itemKey, fromIndex, toIndex, depth) {\n        console.log(itemKey, fromIndex, toIndex, depth)\n    }\n}",
                            related: ["hierarchy-list", "hierarchy-item", "hierarchy-toolbar"]
                        }),
                        component({
                            key: "hierarchy-list",
                            label: "HierarchyList",
                            location: "qml/components/navigation/HierarchyList.qml",
                            docPath: "docs/components/navigation/HierarchyList.md",
                            previewId: "hierarchy-navigation",
                            roleLabel: "Tree list manager",
                            summary: "Depth-aware view manager for flat hierarchy arrays; editable moves are initiated from generated HierarchyItem rows.",
                            usage: "LV.HierarchyList {\n    editable: true\n    model: [\n        { key: \"root\", depth: 0, label: \"Root\", expanded: true },\n        { key: \"camera\", depth: 1, label: \"Camera\" }\n    ]\n}",
                            related: ["hierarchy", "hierarchy-item"]
                        }),
                        component({
                            key: "hierarchy-item",
                            label: "HierarchyItem",
                            location: "qml/components/navigation/HierarchyItem.qml",
                            docPath: "docs/components/navigation/HierarchyItem.md",
                            previewId: "hierarchy-navigation",
                            roleLabel: "Tree row",
                            summary: "Tree row delegate that owns the drag/drop lifecycle API, exposes drop-preview metadata, and renders chevrons, icons, and selection state.",
                            usage: "LV.HierarchyItem {\n    label: \"Camera\"\n    itemKey: \"camera\"\n    showChevron: true\n    hasChildItems: true\n    onDragEnded: function(committed, fromIndex, toIndex, targetDepth) {\n        if (committed)\n            console.log(itemKey, fromIndex, toIndex, targetDepth)\n    }\n}",
                            related: ["hierarchy-list", "hierarchy"]
                        })
                    ]
                }),
                section({
                    key: "navigation-list",
                    label: "List",
                    iconGlyph: "L",
                    summary: "Flat list panel, row, footer, and toolbar components for inspector-like side surfaces.",
                    items: [
                        component({
                            key: "list",
                            label: "List",
                            location: "qml/components/navigation/List.qml",
                            previewId: "list-navigation",
                            roleLabel: "Flat list panel",
                            summary: "Mixed-height list with 17 row types, scrolling, model edit events and optional toolbar/footer slots.",
                            usage: "LV.List {\n    expandToContent: true\n    items: [\n        { type: \"Navigation\", label: \"Library\", value: \"24\" },\n        { type: \"Form\", label: \"Metadata\", inputText1: \"Project\" }\n    ]\n    onItemEdited: function(index, item, field, value) {\n        console.log(index, field, value)\n    }\n}",
                            related: ["list-item", "list-toolbar", "list-footer"]
                        }),
                        component({
                            key: "list-item",
                            label: "ListItem",
                            location: "qml/components/navigation/ListItem.qml",
                            previewId: "list-navigation",
                            roleLabel: "List row",
                            summary: "17 Figma variants composing editable buttons, menus, steppers, selectors, inputs, metadata and previews.",
                            usage: "LV.ListItem {\n    type: LV.ListItem.DetailQuantity\n    label: \"Export\"\n    quantity: 2\n    selector: ({ items: [\"PNG\", \"JPEG\"] })\n    unitSelector: ({ items: [\"Scale\", \"Pixels\"] })\n    primaryAction: ({ text: \"Export\", method: function(event) {\n        console.log(event.values.quantity)\n    } })\n}",
                            related: ["list", "input-field"]
                        }),
                        component({
                            key: "list-toolbar",
                            label: "ListToolbar",
                            location: "qml/components/navigation/ListToolbar.qml",
                            previewId: "list-navigation",
                            roleLabel: "List toolbar",
                            summary: "Three-slot icon toolbar for flat list surfaces.",
                            usage: "LV.ListToolbar {\n    icon1: \"projectStructure\"\n    icon2: \"add\"\n}",
                            related: ["list", "toolbar-button"]
                        }),
                        component({
                            key: "list-footer",
                            label: "ListFooter",
                            location: "qml/components/navigation/ListFooter.qml",
                            docPath: "docs/components/navigation/ListFooter.md",
                            previewId: "list-navigation",
                            roleLabel: "List footer",
                            summary: "Footer strip with up to three configurable icon or menu slots.",
                            usage: "LV.ListFooter {\n    button1: { type: \"icon\", iconName: \"projectStructure\" }\n}",
                            related: ["list", "icon-button", "icon-menu-button"]
                        })
                    ]
                }),
                section({
                    key: "navigation-menu",
                    label: "Menu",
                    iconGlyph: "M",
                    summary: "Context menu surface and row primitives for item menus and submenu affordances.",
                    items: [
                        component({"key": "context-menu-item", "label": "ContextMenuItem", "location": "qml/components/navigation/ContextMenuItem.qml", "docPath": "docs/components/navigation/ContextMenuItem.md", "previewId": "figma-parity", "roleLabel": "Figma component", "summary": "Compact 18px row with Inter Regular 12 and Pretendard SemiBold 12 shortcut.", "usage": "LV.ContextMenuItem { label: \"Label\"; key: \"Key\" }", "related": ["context-menu", "motion"]}),
                        component({"key": "context-menu-divider", "label": "ContextMenuDivider", "location": "qml/components/navigation/ContextMenuDivider.qml", "docPath": "docs/components/navigation/ContextMenuDivider.md", "previewId": "figma-parity", "roleLabel": "Figma component", "summary": "145 by 3px separator with a 4px inset and 30% white line.", "usage": "LV.ContextMenuDivider {}", "related": ["context-menu", "motion"]}),
                        component({"key": "menu", "label": "Menu", "location": "qml/components/navigation/Menu.qml", "docPath": "docs/components/navigation/Menu.md", "previewId": "figma-parity", "roleLabel": "Figma component", "summary": "Regular menu surface with 24px rows, 4px vertical padding and the 12% / 64px frosted window material.", "usage": "LV.Menu { items: [{ label: \"Open\", key: \"⌘O\" }] }", "related": ["context-menu", "motion"]}),
                        component({
                            key: "context-menu",
                            label: "ContextMenu",
                            location: "qml/components/navigation/ContextMenu.qml",
                            docPath: "docs/components/navigation/ContextMenu.md",
                            previewId: "menu-navigation",
                            roleLabel: "Menu surface",
                            summary: "Context-menu surface with placement solver, entry model contract, and overlay behavior.",
                            usage: "LV.ContextMenu {\n    items: [{ label: \"Inspect\", eventName: \"inspect\" }]\n}",
                            related: ["menu-item", "menu-divider", "label-menu-button"]
                        }),
                        component({
                            key: "menu-item",
                            label: "MenuItem",
                            location: "qml/components/navigation/MenuItem.qml",
                            docPath: "docs/components/navigation/MenuItem.md",
                            previewId: "menu-navigation",
                            roleLabel: "Menu row",
                            summary: "Context-menu row that renders label, shortcut, selection state, and optional chevron.",
                            usage: "LV.MenuItem {\n    label: \"Open Recent\"\n    key: \"Cmd+O\"\n    showChevron: true\n    hasChildItems: true\n}",
                            related: ["context-menu", "menu-divider"]
                        }),
                        component({
                            key: "menu-divider",
                            label: "MenuDivider",
                            location: "qml/components/navigation/MenuDivider.qml",
                            docPath: "docs/components/navigation/MenuDivider.md",
                            previewId: "menu-navigation",
                            roleLabel: "Separator",
                            summary: "Single-axis divider used between context-menu groups or item clusters.",
                            usage: "LV.MenuDivider {\n    axis: \"horizontal\"\n}",
                            related: ["context-menu", "menu-item"]
                        })
                    ]
                })
            ]
        }),
        section({
            key: "surfaces",
            label: "Surfaces",
            iconGlyph: "F",
            summary: "Cards and overlays that frame content, alerts, and modal dialogs.",
            items: [
                component({
                    key: "window-material", label: "WindowMaterial",
                    location: "qml/components/surfaces/WindowMaterial.qml",
                    docPath: "docs/components/surfaces/Materials.md", previewId: "material-gallery",
                    roleLabel: "Application background",
                    summary: "Uniform #0B0B0B fill at 50% without gradients. ApplicationWindow retains its native frosted backdrop.",
                    usage: "LV.ApplicationWindow { primaryColor: \"#A571E6\" }",
                    related: ["panel-material", "popover", "tooltip"]
                }),
                component({
                    key: "panel-material", label: "PanelMaterial",
                    location: "qml/components/surfaces/PanelMaterial.qml",
                    docPath: "docs/components/surfaces/Materials.md", previewId: "material-gallery",
                    roleLabel: "Transient surface",
                    summary: "Glass 25 combines a subtle app-accent radial gradient with a 25% neutral tint and 16px backdrop blur.",
                    usage: "LV.PanelMaterial { anchors.fill: parent; backdropSource: siblingContent }",
                    related: ["window-material", "popover", "tooltip"]
                }),
                component({
                    key: "material-surface", label: "MaterialSurface",
                    location: "qml/components/surfaces/MaterialSurface.qml",
                    docPath: "docs/components/surfaces/Materials.md", previewId: "material-gallery",
                    roleLabel: "Shared material renderer",
                    summary: "Configurable density, Primary color, safe backdrop capture and an optional custom silhouette.",
                    usage: "LV.MaterialSurface { density: LV.MaterialSurface.Glass25 }",
                    related: ["window-material", "panel-material"]
                }),
                component({
                    key: "popover", label: "Popover",
                    location: "qml/components/surfaces/Popover.qml",
                    docPath: "docs/components/surfaces/Popover.md", previewId: "material-gallery",
                    roleLabel: "Transient content popup",
                    summary: "Composable Popup with Glass 25, app-accent inheritance and outside-click / Escape dismissal.",
                    usage: "LV.Popover { width: 300; height: 180; contentItem: SettingsView {} }",
                    related: ["panel-material", "tooltip", "context-menu"]
                }),
                component({
                    key: "app-card",
                    label: "AppCard",
                    location: "qml/components/surfaces/AppCard.qml",
                    docPath: "docs/components/surfaces/AppCard.md",
                    previewId: "app-card-surface",
                    roleLabel: "Reusable card",
                    summary: "Reusable card surface with title, subtitle, separator, and flexible content slot.",
                    usage: "LV.AppCard {\n    title: \"System health\"\n    subtitle: \"Last 15 minutes\"\n    LV.Label { text: \"No incidents\" }\n}",
                    related: ["label", "alert"]
                }),
                component({
                    key: "card",
                    label: "Card",
                    location: "qml/components/surfaces/Card.qml",
                    docPath: "docs/components/surfaces/Card.md",
                    previewId: "card-gallery",
                    roleLabel: "File and resource cards",
                    summary: "Eight Figma designs: full-image File, FilePreview, Folder, Project, Device, Model, Member, and Link, with independent selection and actions.",
                    usage: "LV.Card {\n    type: LV.Card.File\n    size: LV.Card.Large\n    detail: LV.Card.Detailed\n    previewSource: imageUrl\n    title: \"Coastal house.png\"\n    metadata: \"PNG · 1536 × 1024\"\n    details: \"A quiet view of the sea.\"\n    showMenu: true\n    onMenuRequested: fileMenu.open()\n}",
                    related: ["app-card", "label", "progress-bar", "push-button"]
                }),
                component({
                    key: "alert",
                    label: "Alert",
                    location: "qml/components/surfaces/Alert.qml",
                    docPath: "docs/components/surfaces/Alert.md",
                    previewId: "alert-surface",
                    roleLabel: "Overlay alert",
                    summary: "Frosted glass alert with a configurable image, title, description, and up to three callable actions.",
                    usage: "LV.Alert {\n    id: alert\n    open: true\n    imageSource: \"qrc:/qt/qml/LVRS/resources/images/alert-file-text.svg\"\n    title: \"Save changes?\"\n    description: \"You have unsaved changes.\"\n    button1Text: \"Save\"\n    button1Method: function() { documentController.save(); alert.open = false }\n    button2Text: \"Discard\"\n    button2Method: function() { documentController.discard(); alert.open = false }\n    button3Text: \"Cancel\"\n    button3Method: function() { alert.open = false }\n}",
                    related: ["alert-button", "modal"]
                }),
                component({
                    key: "alert-button",
                    label: "AlertButton",
                    location: "qml/components/surfaces/AlertButton.qml",
                    previewId: "alert-surface",
                    roleLabel: "Alert action button",
                    summary: "Alert-specific button variant tuned to the dialog visual contract for default and primary actions.",
                    usage: "LV.AlertButton {\n    text: \"Confirm\"\n    tone: LV.AbstractButton.Primary\n}",
                    related: ["alert", "abstract-button"]
                }),
                component({
                    key: "modal",
                    label: "Modal",
                    location: "qml/components/surfaces/Modal.qml",
                    docPath: "docs/components/surfaces/Modal.md",
                    previewId: "modal-surface",
                    roleLabel: "Modal dialog",
                    summary: "Apple-style centered modal with icon, title, description, and up to three actions.",
                    usage: "LV.Modal {\n    open: true\n    title: \"Continue?\"\n    description: \"Confirm the action.\"\n    primaryText: \"Continue\"\n}",
                    related: ["alert", "alert-button", "sheet"]
                }),
                component({
                    key: "sheet",
                    label: "Sheet",
                    location: "qml/components/surfaces/Sheet.qml",
                    docPath: "docs/components/surfaces/Sheet.md",
                    previewId: "sheet-gallery",
                    roleLabel: "Adaptive content sheet",
                    summary: "Mobile bottom sheet and desktop modal with device-radius input, reusable Component or inline content, and fixed-header scrolling.",
                    usage: "LV.Sheet {\n    id: sheet\n    title: \"Export image\"\n    cornerRadius: deviceCornerRadius\n    contentComponent: exportView\n}\n// Open with sheet.open(), close with sheet.close().",
                    related: ["modal", "color-picker", "label-button", "tooltip"]
                }),
                component({
                    key: "tooltip",
                    label: "Tooltip",
                    location: "qml/components/surfaces/Tooltip.qml",
                    docPath: "docs/components/surfaces/Tooltip.md",
                    previewId: "tooltip-gallery",
                    roleLabel: "Anchored content bubble",
                    summary: "A speech bubble accepting any Component or inline view. Its tail tracks the origin while the body chooses available space inside the display.",
                    usage: "LV.Tooltip {\n    target: helpButton\n    contentComponent: helpView\n}\n// Manual origin: tooltip.openAt(canvas, Qt.point(x, y), previewView)",
                    related: ["sheet", "label", "label-button"]
                })
            ]
        })
    ]

    readonly property int componentCount: countComponents([overview].concat(sections))
    readonly property int totalEntryCount: countEntries([overview].concat(sections))
    readonly property var hierarchyModel: buildHierarchy([overview].concat(sections))

    function motionGuide(key) { return CatalogMotion.guide(key) }

    function allComponents() {
        const result = []
        function visit(records) {
            for (let i = 0; i < records.length; i++) {
                if (records[i].kind === "component") result.push(records[i])
                visit(childRecords(records[i]))
            }
        }
        visit(sections)
        return result
    }

    function filteredHierarchy(query) {
        const term = String(query || "").trim().toLowerCase()
        if (!term.length) return hierarchyModel
        return allComponents().filter(function(record) {
            return (record.label + " " + record.summary + " " + record.location).toLowerCase().indexOf(term) >= 0
        }).map(function(record) { return buildHierarchyNode(record, 0) })
    }

    function childRecords(record) {
        const children = []
        const groups = record && record.groups ? record.groups : []
        const items = record && record.items ? record.items : []
        for (let i = 0; i < groups.length; i++)
            children.push(groups[i])
        for (let i = 0; i < items.length; i++)
            children.push(items[i])
        return children
    }

    function buildHierarchy(records) {
        const nodes = []
        for (let i = 0; i < records.length; i++)
            appendHierarchyRows(records[i], 0, nodes)
        return nodes
    }

    function buildHierarchyNode(record, depth) {
        const children = childRecords(record)
        const icon = record.iconGlyph && String(record.iconGlyph).length > 0
            ? String(record.iconGlyph)
            : String(record.label || "?").charAt(0).toUpperCase()
        return {
            key: record.key,
            depth: depth,
            label: record.label,
            iconGlyph: icon,
            expanded: true,
            showChevron: children.length > 0
        }
    }

    function appendHierarchyRows(record, depth, sink) {
        const children = childRecords(record)
        sink.push(buildHierarchyNode(record, depth))
        for (let i = 0; i < children.length; i++)
            appendHierarchyRows(children[i], depth + 1, sink)
    }

    function countEntries(records) {
        let total = 0
        for (let i = 0; i < records.length; i++) {
            total += 1
            total += countEntries(childRecords(records[i]))
        }
        return total
    }

    function countComponents(records) {
        let total = 0
        for (let i = 0; i < records.length; i++) {
            const record = records[i]
            if (record.kind === "component")
                total += 1
            total += countComponents(childRecords(record))
        }
        return total
    }

    function entryByKey(key) {
        if (!key)
            return overview
        return findRecord([overview].concat(sections), key) || overview
    }

    function directChildren(key) {
        const record = entryByKey(key)
        return childRecords(record)
    }

    function breadcrumb(key) {
        return findBreadcrumb([overview].concat(sections), key, []) || [overview.label]
    }

    function findRecord(records, key) {
        for (let i = 0; i < records.length; i++) {
            const record = records[i]
            if (record.key === key)
                return record
            const nested = findRecord(childRecords(record), key)
            if (nested)
                return nested
        }
        return null
    }

    function findBreadcrumb(records, key, prefix) {
        for (let i = 0; i < records.length; i++) {
            const record = records[i]
            const nextPrefix = prefix.concat([record.label])
            if (record.key === key)
                return nextPrefix
            const nested = findBreadcrumb(childRecords(record), key, nextPrefix)
            if (nested)
                return nested
        }
        return null
    }
}

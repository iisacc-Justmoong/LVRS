import QtQuick

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
        summary: "LVRS public QML types are organized into a left-side hierarchy so the catalog can be used as a navigable visual reference instead of a tabbed demo.",
        notes: [
            "Use the sidebar to browse by shell, layout, control, navigation, and surface domains.",
            "Each leaf page pairs a live preview with source path, usage snippet, and related types.",
            "Section pages summarize the types contained under that branch."
        ]
    })

    readonly property var sections: [
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
                    summary: "Adaptive LVRS root shell that combines window policy, bootstrap defaults, navigation scaffold, runtime wiring, and page-stack hosting.",
                    usage: "LV.ApplicationWindow {\n    visible: true\n    width: 1320\n    height: 860\n    title: \"Workspace\"\n    subtitle: \"Adaptive shell\"\n    pageRoutes: [{ path: \"/\", component: homePage }]\n}",
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
                            key: "label-button",
                            label: "LabelButton",
                            location: "qml/components/control/buttons/LabelButton.qml",
                            docPath: "docs/components/control/LabelButton.md",
                            previewId: "button-family",
                            roleLabel: "Text action",
                            summary: "Compact fixed-height text button for dense action rows and toolbar controls.",
                            usage: "LV.LabelButton {\n    text: \"Save\"\n    tone: LV.AbstractButton.Borderless\n}",
                            related: ["icon-button", "label-menu-button"]
                        }),
                        component({
                            key: "icon-button",
                            label: "IconButton",
                            location: "qml/components/control/buttons/IconButton.qml",
                            docPath: "docs/components/control/IconButton.md",
                            previewId: "button-family",
                            roleLabel: "Icon action",
                            summary: "Icon-first button variant with token, source URL, or glyph fallback input.",
                            usage: "LV.IconButton {\n    iconName: \"add\"\n    tone: LV.AbstractButton.Borderless\n}",
                            related: ["label-button", "icon-menu-button", "toolbar-button"]
                        }),
                        component({
                            key: "label-menu-button",
                            label: "LabelMenuButton",
                            location: "qml/components/control/buttons/LabelMenuButton.qml",
                            docPath: "docs/components/control/LabelMenuButton.md",
                            previewId: "button-family",
                            roleLabel: "Menu trigger",
                            summary: "Text-first menu trigger with a trailing chevron indicator aligned to LVRS menu affordances.",
                            usage: "LV.LabelMenuButton {\n    text: \"Options\"\n    tone: LV.AbstractButton.Default\n}",
                            related: ["icon-menu-button", "context-menu"]
                        }),
                        component({
                            key: "icon-menu-button",
                            label: "IconMenuButton",
                            location: "qml/components/control/buttons/IconMenuButton.qml",
                            docPath: "docs/components/control/IconMenuButton.md",
                            previewId: "button-family",
                            roleLabel: "Icon menu trigger",
                            summary: "Icon-centric menu button with tone-aware chevron rendering and deterministic indicator mapping.",
                            usage: "LV.IconMenuButton {\n    iconName: \"projectStructure\"\n    tone: LV.AbstractButton.Borderless\n}",
                            related: ["label-menu-button", "context-menu"]
                        }),
                        component({
                            key: "label-segmented-control",
                            label: "LabelSegmentedControl",
                            location: "qml/components/control/buttons/LabelSegmentedControl.qml",
                            docPath: "docs/components/control/LabelSegmentedControl.md",
                            previewId: "segmented-control",
                            roleLabel: "Segment container",
                            summary: "Segmented shell for text buttons that normalizes sibling tone and shared framing.",
                            usage: "LV.LabelSegmentedControl {\n    LV.LabelButton { text: \"Week\" }\n    LV.LabelButton { text: \"Month\" }\n}",
                            related: ["icon-segmented-control", "label-button"]
                        }),
                        component({
                            key: "icon-segmented-control",
                            label: "IconSegmentedControl",
                            location: "qml/components/control/buttons/IconSegmentedControl.qml",
                            docPath: "docs/components/control/IconSegmentedControl.md",
                            previewId: "segmented-control",
                            roleLabel: "Icon segment container",
                            summary: "Segmented shell for icon buttons with shared background, border, and spacing contract.",
                            usage: "LV.IconSegmentedControl {\n    LV.IconButton { iconName: \"projectStructure\" }\n    LV.IconButton { iconName: \"add\" }\n}",
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
                            previewId: "selection-control",
                            roleLabel: "Binary switch",
                            summary: "Switch control with animated knob transitions and explicit hover, pressed, and disabled track colors.",
                            usage: "LV.ToggleSwitch {\n    text: \"Enabled\"\n    checked: true\n}",
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
                            summary: "Turnkey table composition that renders headers and rows from array data.",
                            usage: "LV.Table {\n    headerColumns: [\"Name\", \"State\"]\n    rows: [[\"Renderer\", \"Active\"]]\n}",
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
                            usage: "LV.TableHeader {\n    columns: [\"Column\", \"Column\"]\n}",
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
                            usage: "LV.TableRow {\n    cells: [\"Renderer\", \"Active\"]\n}",
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
                            usage: "LV.TableCellItem {\n    text: \"Renderer\"\n}",
                            related: ["table", "table-row"]
                        })
                    ]
                }),
                section({
                    key: "control-input",
                    label: "Input",
                    iconGlyph: "I",
                    summary: "Text entry foundations and editor components with deterministic LVRS behavior.",
                    items: [
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
                            summary: "One-line input control with optional search affordance and integrated clear action.",
                            usage: "LV.InputField {\n    search: true\n    placeholderText: \"Search\"\n}",
                            related: ["abstract-input-bar", "text-editor"]
                        }),
                        component({
                            key: "text-editor",
                            label: "TextEditor",
                            location: "qml/components/control/input/TextEditor.qml",
                            docPath: "docs/components/control/TextEditor.md",
                            previewId: "text-editor",
                            roleLabel: "Multi-line editor",
                            summary: "Scrollable multi-line text editor with explicit outer height and embedded guard helpers.",
                            usage: "LV.TextEditor {\n    editorHeight: 160\n    text: \"Notes go here\"\n}",
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
                            summary: "Flat list surface with optional toolbar and footer slots for inspector-style sidebars.",
                            usage: "LV.List {\n    items: [{ label: \"Overview\" }, { label: \"Reports\" }]\n}",
                            related: ["list-item", "list-toolbar", "list-footer"]
                        }),
                        component({
                            key: "list-item",
                            label: "ListItem",
                            location: "qml/components/navigation/ListItem.qml",
                            previewId: "list-navigation",
                            roleLabel: "List row",
                            summary: "Borderless row delegate for flat lists with optional inline input overlay.",
                            usage: "LV.ListItem {\n    label: \"Inspector row\"\n    inputable: true\n}",
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
                    key: "alert",
                    label: "Alert",
                    location: "qml/components/surfaces/Alert.qml",
                    docPath: "docs/components/surfaces/Alert.md",
                    previewId: "alert-surface",
                    roleLabel: "Overlay alert",
                    summary: "Overlay dialog surface with explicit one, two, or three action layouts.",
                    usage: "LV.Alert {\n    open: true\n    buttonCount: 2\n    title: \"Delete item?\"\n    primaryText: \"Delete\"\n}",
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
                    related: ["alert", "alert-button"]
                })
            ]
        })
    ]

    readonly property int componentCount: countComponents([overview].concat(sections))
    readonly property int totalEntryCount: countEntries([overview].concat(sections))
    readonly property var hierarchyModel: buildHierarchy([overview].concat(sections))

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

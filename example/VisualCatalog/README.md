# Visual Catalog Example

This example is the LVRS visual documentation browser driven by `Main.qml`.
The left sidebar is a `Hierarchy` index of the public QML surface, and the right panel shows the selected type as a live reference page with preview, source path, related types, and usage snippet.

## Run

From repository root:

```bash
./build.sh
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

The checked-in launcher resolves the build-tree executable from `build/example/VisualCatalog/bin/LVRSExampleVisualCatalog` during in-repository development and a sibling snapshot runtime after install.

## Structure

- `qml/CatalogRegistry.qml` holds the catalog tree, component metadata, and sidebar model.
- `qml/Main.qml` renders the application shell, hierarchy sidebar, and detail panels.
- The catalog shell uses `layoutSafeAreaBounds` explicitly, and the Application section now includes `WindowSafeAreaObserver` as the safe-area reference entry.

## Intended Use

- Browse the library by domain instead of memorizing file paths.
- Use section nodes for category overviews.
- Use component nodes for concrete visual documentation and usage examples.
- Use the hierarchy preview to validate flat depth-array reordering and parent reassignment by drag and drop.

## Sheet gallery

Surfaces → Sheet includes mobile Fit/Medium/Large, desktop export using the same content Component, and a direct inline frame. The 28/55/0 radius controls are illustrative logical-pixel inputs, not measured specifications of named devices. Folder selection owns its ListView and fixed Save action. Use `LVRSTests_sheet` for the content, radius, layout and modal input contracts.

## ToggleSwitch motion

Selection → ToggleSwitch has dedicated On/Off, slow (`640ms`), immediate (`0ms`), and disabled examples. Hold to squash the knob, tap or press Space to see its stretch and rebound, and click again during travel to check retargeting. Run `LVRSTests_toggle_switch` for the input and motion contracts.

## Tooltip gallery

Surfaces → Tooltip provides four display-corner launch points, a draggable origin, a reusable help view, a status view with interactive LVRS controls, and automatic hover/focus/hold text help. The frame bounds the bubbles and makes direction changes visible. Run `LVRSTests_tooltip` for placement, rendered tails, content lifecycle, scrolling, and input checks.

## App accent preview

The ApplicationWindow / shell preview includes LVRS blue, Purple, and Orange
choices. Each assigns the catalog root's `primaryColor`, updating the whole
catalog immediately. Choose LVRS blue to restore the default. The usage snippet
shows the same app-root input for consumers.

## Standard materials

WindowMaterial, PanelMaterial, MaterialSurface and Popover share the interactive Material gallery. Its accent controls update the app theme, and its actions open a ContextMenu, Tooltip and Popover above the same window. The catalog contains 84 type entries. The host ApplicationWindow and standalone WindowMaterial show a uniform #0B0B0B fill at 50% without gradients, retaining native macOS frosted blur. The PanelMaterial preview keeps its Glass 25 radial treatment. Changing the accent updates controls and panel accents while the Window fill remains near black.

## Component Studio

The window now combines a compact motion toolbar, searchable hierarchy and a responsive detail page. Each of the 84 indexed types has a live playground and a specific interaction recipe: Try it, Response, Look for and Behavior contract. Source, usage and related types remain available below the motion inspector. Supporting nonvisual types link to the real consumer that displays their behavior.

Use 1×, 0.5× and 0.25× to change the whole engine's playback speed. Reduce motion makes states immediate. Reset preview recreates the selected example without resetting global motion preferences. The Motion playground includes buttons, selection, focus, sliders, progress, layout spacing and passive-layer opacity.

`CatalogMotion.js` owns the explicit recipes; `MotionInspector.qml` renders them; `MotionLab.qml` demonstrates the common rhythm. `LVRSTests_catalog` checks QML inventory coverage, every preview, search and reset. See [motion policy](../../docs/motion.md).

Figma parity adds five detailed entries (84 entries total): HelpButton, ColorPickerButton, ContextMenuItem, ContextMenuDivider and Menu. Their live gallery compares authored sizes, compact/regular menu typography, focus, disabled state and shared elastic motion. See [audit](../../docs/figma-parity.md).

The material gallery now previews the user-directed menu coating: WindowMaterial, 12% tint, 64px blur, and 8% additional accent strength. Native A/B captures compare this with the earlier saturated menu treatment.

# Figma parity audit — 2026-09-12

Subsequent user-directed revisions: WindowMaterial and ApplicationWindow now use a uniform #0B0B0B fill at 50% without gradients. The current window treatment follows [the material contract](components/surfaces/Materials.md). Menu revision: ContextMenu and Menu now use a lighter WindowMaterial coating (12% tint / 64px blur / 8% additional accent strength). The Figma inventory below is the historical source snapshot; the current menu coating follows [the revised contract](components/navigation/ContextMenu.md#window-derived-frosted-menu).

Source of truth: [Layerd Visual Render System](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System). The audit reads the current components and bound variables. It does not change Figma.

The [family inventory](figma-family-inventory.json) records the root geometry, paints and effects of 65 component families and their variants. The audit also reads the 21 Color variables, Material/Alert/Card/ListItem/Slider/ColorPicker/Sheet variables, all 2,349 Iconset component names, and the relevant child-node typography and spacing. This is a structural and targeted visual audit, not a pixel comparison of every icon or every application screen.

## Corrections

| Figma | Source change | Authored values |
| --- | --- | --- |
| HelpButton `44:819` | New `HelpButton` | 21 × 21, radius 100, horizontal padding 4, vertical padding 2, Pretendard SemiBold 12 question-mark text, Description color, panelBackground04 |
| ColorPickerButton `892:62` | New `ColorPickerButton` | 22 / 28 / 36, padding 4, radius 4; default panel06, hover panel12, pressed panel03; 0.5px white 20% border, focused 2px Primary border, disabled opacity 0.32 |
| Color well `892:4` | Exact exported PNG hue ring | 320px export of the 20px vector ring; the center follows `currentColor`, default #7A5AF8 |
| ContextMenuItem `331:9282` | New compact row; ContextMenu uses it by default | 141 × 18, horizontal padding 4, vertical padding 0, radius 0; Inter Regular 12 white label and Pretendard SemiBold 12 Description shortcut |
| ContextMenuDivider `331:9283` | New compact divider | 145 × 3 standalone, 4px line inset, 1px line, white 30% |
| ContextMenu `331:9332` | Corrected container and width probe | 141px minimum content width + 8px padding on each side; 8px vertical padding; reference five rows + divider = 157 × 119 |
| Menu `110:857` | New regular menu type | 24px rows, 145px content width, padding 8 horizontal / 4 vertical; reference eight rows + two dividers = 161 × 224 |
| ListFooter `209:9199` | Preserved the dropdown button's asymmetric padding | 88 × 26, slots 22 / 22 / 40, menu padding 4 left / 2 right; radius 8; explicit per-slot overrides remain supported |
| Color variables | Preserved Figma float RGB and alpha values | All 12 panel colors and five white text alpha tokens; panelBackground04 displays as #181919 instead of #191919 |
| Iconset names | Case-safe aliases and fresh Figma SVG exports for nodesTest / wechat | nodesTest → nodestest, wechat → weChat; leading whitespace in “ volume” already normalizes |
| Material diffusion `975:5`, `975:9` | Added the missing inset highlight | White, offset (0, 1), blur 0; Dense 6%, Glass 14%; existing 64 / 16px blur and 40 / 11% radial values retained |

All five new public types inherit the existing LVRS motion policy. State changes and actions remain immediate; only presentation interpolates. ColorPickerButton is an action trigger: the consumer owns the color-picker popup or other action. No new animation package is needed.

## Family coverage

Alert, ApplicationWindow, Button, Card, Checkbox, ColorPicker, ComboBox, ContextMenu, Hierarchy, Input, List, Menus, Popover, Radio, SegmentedControl, Sheet, Slider, Stepper, Table, Toggle and Tooltip were inventoried. Existing Card, ListItem, Slider, Sheet and input variant APIs were checked against the captured root metrics and existing runtime contracts. Content-driven widths/heights in generic hosts are reference examples, not mandatory fixed window sizes. Internal nested variant sets and Sheet demo content are not additional public UI types.

The Avatar page `786:213` has no child nodes. There are no authored values to implement there. Icon inventory coverage verifies name resolution; the two explicitly exported glyphs and hue ring are checked against recorded SHA-256 values for asset identity. The Figma GLASS renderer and Qt backdrop blur are different renderers; native captures validate LVRS behavior without claiming pixel identity of the optical blur.

## Dependency and licensing

The ContextMenu label explicitly specifies Inter Regular. LVRS bundles only the required static Regular face from [Inter v4.1](https://github.com/rsms/inter/releases/tag/v4.1), 411,640 bytes, alongside the existing Pretendard family. Its [SIL Open Font License](../resources/font/Inter-OFL.txt) is included in the resource bundle. It adds no runtime package or network requirement. The application-wide font preference remains Pretendard; Inter is selected by the compact menu label.

## Verification

`tests/fixtures/figma-contract.json` stores the inspected variable and component-node values independently of implementation. `LVRSTests_figma_parity` covers exact colors, alias resolution, five component footprints, both menu densities and fonts, input activation, disabled/focus/color states, ListFooter overrides and native inset-highlight rendering. `LVRSTests_catalog` checks source-type coverage and loads every detailed preview. The import API suite retains the regular menu's previous behavior under its new correct name, Menu, while adding compact-menu coverage.

VisualCatalog includes HelpButton, ColorPickerButton, ContextMenuItem, ContextMenuDivider and Menu, with instructions and live interactions. Select these entries to inspect the corrected dimensions and motion; use “Open ContextMenu” and “Open Menu” to compare the two popup densities.

Validated on macOS with Qt 6.8.3: full LVRS CTest 53/53 passed, all 84 catalog entries loaded, and native Figma parity 12/12 passed (including rendered inset highlights). The regular menu retains the existing 33px natural Label metric by sharing TextMetrics font bindings with rendered text. Native catalog captures cover HelpButton, ColorPickerButton, ContextMenuItem and Menu.

Installed consumer verification: the staged library matches all 83 current QML resources. Society 16/16 and Dreamscapes 5/5 CTest passed after rebuilding against that package. Both macOS bundles were restarted and their live loaded LVRS paths were verified; Dreamscapes displayed the new compact ContextMenuItem.

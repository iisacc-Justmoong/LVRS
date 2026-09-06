# ListItem

Location: `qml/components/navigation/ListItem.qml`

`ListItem` implements the 17 variants of Figma component set `241:9253`. It composes existing LVRS controls; its inputs, selectors, buttons and selection controls are interactive.

## Variants and geometry

Set `type` to a `ListItem` enum value. The legacy `size` property aliases `type`; `Mini` and `Detail` retain their enum values and default dimensions.

| Type | Desktop size | Content |
| --- | --- | --- |
| Mini | 170 × 22 | Icon and label; optional legacy inline editor |
| Detail | 194 × 106 | Two-line title, bookmark, date, folders and tags |
| Navigation | 280 × 44 | Label, description, value and disclosure |
| Toggle | 280 × 44 | Label, description and switch |
| Checkable | 280 × 44 | Checkbox, label, description and value |
| Action | 400 × 44 | One trailing action |
| ActionGroup | 400 × 44 | Two actions and a dropdown menu |
| Stepper | 400 × 44 | Quantity and directional stepper |
| Select | 400 × 44 | Value selector |
| InlineEdit | 400 × 44 | Input and save action |
| DetailActions | 400 × 120 | Two-line description, bookmark, metadata and three actions |
| DetailQuantity | 400 × 118 | Format, quantity, units, switch and action |
| DetailSettings | 400 × 159 | Switch, selector, segments, status and two actions |
| Resource | 400 × 129 | Preview, description, metadata, menu and actions |
| Media | 400 × 113 | Preview, duration, source segments and playback actions |
| Task | 400 × 135 | Checkbox, priority, description, metadata, estimate and actions |
| Form | 400 × 148 | Selector, two labeled inputs, switch, status and actions |

The measured sizes above apply to default content and visible slots on desktop and mobile. Compound height follows content and optional slots. Standard rows have a 44px minimum height. Theme geometry and all typography retain their authored values on both targets; Body remains 13px.

Widths can be assigned by a parent layout. Default actions reserve 72px, selectors 97px, inputs 140px, and previews 48px. Labels and values elide inside their allocated space. These presets are designed for their listed widths; larger custom controls require a wider row or a custom delegate.

## Content and state

- Common: `label`, `description`, `iconName` / `iconSource`, `value`, `selected`.
- Detail: `detail`, `dateText`, `folderLabel1/2`, `tagLabel1/2`, `bookmarkIconName`, `folderIconName`, `tagIconName`.
- Compound: `metadata1`, `metadata2`, `statusText`, `quantityLabel`, `optionLabel`, `modeLabel`, `scopeLabel`, `durationText`, `mediaDetail`, `fieldLabel1/2`.
- Preview: `previewSource` for an image URL, or `previewIconName` for a Theme asset. Icons remain at their native 18px size, centered inside the 48px tile.
- Selection: `checked`, `toggleEnabled`, `selectionEnabled`. `selected` controls the row highlight independently of the checkbox/switch value.
- Quantity: `quantity`, `minimumQuantity` (0), `maximumQuantity` (unbounded), `stepSize` (1). User edits are finite and clamped to the configured range.
- Selector state: `selectorIndex`, `unitIndex` (both 0), `segmentIndex` (0).
- Input state: `inputText1`, `inputText2` (both `"Value"`). These are independent of the legacy Mini `inputResult`.

Optional elements use `showLeadingIcon`, `showDescription`, `showValue`, `showTrailingIcon`, `showBookmark`, `showDate`, `showFolders`, `showTags`, `showMetadata`, `showPrimaryAction`, `showSecondaryAction`, `showMoreMenu`, `showStepper`, `showSelector`, `showUnitSelector`, `showToggle`, `showSelection`, `showSegments`, `showInput1`, `showInput2`, and `showPreview`. All default to true. A flag affects only variants containing that element.

## Nested controls

Configuration objects are optional; unspecified fields retain preset defaults.

| Property | Supported configuration |
| --- | --- |
| `primaryAction`, `secondaryAction` | `text`, `tone`, `enabled`, `iconMode`, `iconName`, `method`, `methods`, `onTriggered`, `component` |
| `moreMenu` | Same button fields plus ContextMenu `items` |
| `selector`, `unitSelector` | `items`, fallback `text`, `tone` (ComboBox enum), `arrow` (Stepper enum), `enabled`, `component` |
| `stepper` | `tone`, `arrow`, `enabled` |
| `input1`, `input2` | `placeholderText`, `readOnly`, `enabled`, `style` (InputField enum), `clearButtonVisible`, `maximumLength`, `validator`, `component` |
| `segments` | Array of strings or `{ text/label, enabled, method }` entries |

Selector options accept arrays or QML list-like `count/get()` sources. Options may be strings or `{label, enabled, ...ContextMenu fields}` objects. Clicking opens an LVRS ContextMenu; keyboard Up/Down changes the selection and Enter/Space opens the menu. Disabled entries are skipped.

The quantity stepper uses its upper/lower half to increment/decrement. The focused quantity control also accepts keyboard Up/Down. A single-arrow Stepper has one direction for both pointer and keyboard input. Disabled steppers reject both input methods. Text fields retain LVRS material fill, input handling and the clear button's fixed 8px right inset.

## Events and methods

- `edited(field, value)` reports user changes to `checked`, `quantity`, `selectorIndex`, `unitIndex`, `segmentIndex`, `inputText1` or `inputText2`. Direct property assignment does not emit it.
- `actionTriggered(action, payload)` reports `primary`, `secondary`, `menu`, `menuItem` and `inputSubmitted`.
- `primaryAction.method` / `secondaryAction.method` receive the existing LVRS method event, augmented with `action`, `payload`, and `values` containing the current input, selection and quantity state. `methods` invokes multiple methods; `onTriggered` is an alternative to `method`.
- ContextMenu option callbacks retain the ContextMenu event contract. `menuItem` payload contains `{index, item}`; `inputSubmitted` contains `{field, text}`.
- Child actions do not click the enclosing row. `clicked` and the inherited row `method` remain available for row activation.
- `editValue(field, value)`, `stepQuantity(direction)` and `triggerAction(action, payload)` expose the same behavior for custom content. `typeValue(value)` resolves a variant name or numeric enum.

The legacy Mini `inputable`, `inputResult`, `inputEdited`, and `inputSubmitted` APIs remain supported.

## Custom composition

`leadingComponent` and `trailingComponent` replace the leading icon or the complete trailing cluster in standard rows. `previewComponent` replaces the Resource/Media preview. `footerComponent` adds content below the preset. Button, selector and input configuration objects can also provide a `component` replacement.

A replacement should provide its own implicit dimensions and may declare `property var listItem` to receive the owner. This property must be optional because Loader supplies it after construction. It may contain multiple LVRS controls and call the owner's edit/action methods.

```qml
import LVRS 1.0 as LV

LV.ListItem {
    type: LV.ListItem.DetailQuantity
    label: "Export images"
    quantity: 2
    minimumQuantity: 1
    maximumQuantity: 8
    selector: ({ items: ["PNG", "JPEG", "WebP"] })
    unitSelector: ({ items: ["Scale", "Pixels"] })
    primaryAction: ({
        text: "Export",
        method: function(event) { exporter.run(event.values.quantity) }
    })
}
```

## Verification

`LVRSTests_list_composites` uses Qt Test and Qt Quick. It checks all preset dimensions, desktop/mobile policy, mixed-height lists, pointer/keyboard edits, button callbacks, popup selection, delegate identity, dynamic type changes, fixed footers, custom trailing composition and actual rendering of every preset. Set `LVRS_LIST_CAPTURE_DIR` to a directory under `build/` to save `listitem-presets.png`. Existing Mini/Detail and model contracts remain covered by `LVRSTests_import_api` and `LVRSTests_platform_integration`.

Build and run with `cmake --build build`, then `ctest --test-dir build --output-on-failure`. On a development machine with an installed LVRS override, ensure the test process loads `build/libLVRS` (on macOS: `DYLD_LIBRARY_PATH="$PWD/build"`).

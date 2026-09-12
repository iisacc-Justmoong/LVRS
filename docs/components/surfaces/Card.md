# Card

`LV.Card` implements the eight Card designs: File, FilePreview, Folder, Project,
Device, Model, Member, and Link. It extends `LV.AbstractButton`, retaining its
`clicked`, keyboard focus, `method` and `methods` contracts. `AppCard` remains the
separate, general-purpose titled container.

```qml
import QtQuick
import LVRS as LV

LV.Card {
    type: LV.Card.File
    size: LV.Card.Large
    detail: LV.Card.Detailed
    previewSource: "file:///photos/coastal-house.png"
    filename: "Coastal house.png" // alias of title
    metadata: "PNG · 1536 × 1024"
    details: "Warm limestone, curved walls and a quiet view of the sea."
    showMenu: true
    onMenuRequested: fileMenu.open()
    onClicked: selectionController.setSelected(filename, selected)
}
```

## Types and measured sizes

| `type` | Figma node | Default logical size | Content |
|---|---|---|---|
| `Card.File` | [852:997](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=852-997) | Medium 256 × 320 | Image fills the card; caption stays at its bottom |
| `Card.FilePreview` | [852:993](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=852-993) | 480 × 320 | Unadorned image preview |
| `Card.Folder` | [786:414](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-414) | 256 × 280 | Icon rows, summary and action |
| `Card.Project` | [786:566](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-566) | 256 × 280 | Milestone progress, rows and participants |
| `Card.Device` | [786:696](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-696) | 256 × 280 | Storage progress, connection and sync details |
| `Card.Model` | [786:830](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-830) | 256 × 280 | Model specification rows and compatibility summary |
| `Card.Member` | [786:966](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-966) | 256 × 280 | Member role, projects, team and location |
| `Card.Link` | [786:1084](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-1084) | 256 × 280 | Domain preview, title, description and visit action |

File supports `Card.Small` (192 × 192), `Card.Medium` (256 × 320), and
`Card.Large` (480 × 280), each with `Card.Brief` or `Card.Detailed`. All seven
card types have default, hover and selected visuals. The current Theme uses
the same logical sizes on desktop and mobile. Explicit width/height overrides
remain responsive. Information cards grow in implicit height when more rows
are supplied; consumers should not force a height below that content minimum.

## Content API

| Properties | Purpose |
|---|---|
| `title`, `filename` | Card title; both address the same string |
| `description`, `showDescription` | Supporting line on information cards |
| `details`, `metadata`, `detail` | File caption body and metadata; detailed text is limited to 2 / 3 / 2 lines for Small / Medium / Large |
| `previewSource` | Local, resource, network or image-provider URL; uses `Image.PreserveAspectCrop` (Figma FILL) |
| `previewComponent` | Optional `Component` replacing the image; resized to the entire preview area |
| `previewItem`, `previewStatus` | Read-only current image/custom item and Image loading status |
| `asynchronous` | Image loading flag, defaults to true |
| `iconName`, `iconSource`, `iconComponent` | Override the default type icon; component/source/name take precedence in that order |
| `statusText`, `statusColor`, `showStatus` | Header status; color defaults to Theme.success for Device/Model/Member |
| `rows` | Array of `{label, value, iconName?, iconSource?}`; labels and values truncate independently |
| `progressLabel`, `progressText`, `progress`, `progressColor` | Project/Device progress, normalized 0–1 and clamped by LV.ProgressBar |
| `summary`, `footnote` | Lower body context and footer text |
| `domain`, `previewTitle` | Link preview contents |
| `showMenu`, `menuAccessibleName` | Menu visibility and accessible name; menu is hidden on File by default |
| `showAction`, `actionText` | Footer button visibility/text; defaults to Open/View/Manage/Use/Profile/Visit by type |

Content strings and rows default to empty. Supply application data; the library
does not ship hardcoded sample photos or domain records. The catalog bundles
the exact Architecture, Landscape and Document image assets from Figma and
demonstrates all three with both File and FilePreview.

```qml
LV.Card {
    type: LV.Card.Device
    title: device.name
    description: "macOS · Desktop host"
    statusText: device.online ? "Online" : "Offline"
    statusColor: device.online ? LV.Theme.success : LV.Theme.descriptionColor
    progressLabel: "Storage"
    progressText: "128 / 512 GB"
    progress: 128 / 512
    rows: [{label: "Connection", value: "Local network"}]
    summary: "Last synced just now"
    footnote: "384 GB available"
    onActionTriggered: deviceController.manage(device.id)
    onMenuRequested: deviceMenu.open()
}
```

## Interaction and rendering

`selected` aliases `checked`; `selectable` defaults to true except for FilePreview.
Click or Space toggles selection. Nested buttons emit `menuRequested()` and
`actionTriggered()` independently and do not select/deselect the card. Disabled
cards suppress input. `displayState` normally stays `Card.Automatic`; use
`Card.DefaultState`, `Card.HoverState` or `Card.SelectedState` for static design
previews. It overrides appearance only, not application selection state.

FilePreview has no caption, border, menu or selection marker. Its source is
cropped at the center. A custom preview is a display surface; place interactive
actions in the card's menu/action handling. Missing/loading/error images use the
file-image icon and retain the caption. Native `Image` status remains observable.

The card uses Theme colors, a 12 px radius, 1 px border (2 px selected), 18 px
padding (12 px Small File), and the measured Pretendard typography. Padding
includes the inside stroke, matching Figma. File caption heights are 37/39/39 px
for Brief and 75/99/81 px for Detailed. A three-stop bottom scrim preserves text
contrast. `QtQuick.Effects.MultiEffect` applies the rounded alpha mask to the
preview and scrim on the hardware scene graph. On Qt's software renderer, Canvas
loads the image URL into its cache and draws the same center crop, rounded clip and scrim.
Custom preview components remain live on software but must provide their own
rounded clipping; the hardware renderer masks arbitrary custom components.
No additional library is added. The internal QML files use module-root resource
aliases so compiled, filesystem and installed imports resolve the same URLs.

## Validation

`LVRSTests_card` uses Qt Test with real QML creation, pointer/keyboard events and
Qt Quick image captures. It covers the Figma size/state matrix on desktop and
mobile Theme targets, resizing and truncation, custom previews, progress clamps,
failed-image recovery, independent nested actions and rounded Fill rendering.

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON -DLVRS_BUILD_EXAMPLES=ON
cmake --build build -j 8
ctest --test-dir build --output-on-failure
LVRS_CARD_CAPTURE_DIR="$PWD/build/card-captures" ctest --test-dir build -R '^LVRSTests_card$' --output-on-failure
```

Open **Surfaces → Card** in VisualCatalog for the live eight-design gallery.

The existing installed-consumer test also instantiates all eight types and checks
their dimensions, including the internal QML resource aliases. For native Metal
render validation on macOS, run `build/tests/LVRSTests_card` with
`QT_QPA_PLATFORM=cocoa`, `QSG_RHI_BACKEND=metal`, and `DYLD_LIBRARY_PATH="$PWD/build"`.

## Shared motion

Card deformation is capped in pixels, selection fill blends, and nested actions remain independent. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.

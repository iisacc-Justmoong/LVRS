# Standard Materials

## Purpose

`WindowMaterial` and `PanelMaterial` started from the Figma Material components. The 2026-09-12 user revision replaces the Window gradients with a uniform near-black fill (#0B0B0B) at 50% opacity, both standalone and in `ApplicationWindow.background`. Native behind-window blur remains active on macOS. ContextMenu and Menu retain their lighter frosted coating; Tooltip and Popover retain Glass 25.

| Material | Fill | Background blur | Default use |
| --- | --- | --- | --- |
| ApplicationWindow / WindowMaterial | Uniform #0B0B0B at 50%, no gradients | 64 logical px; native frosted backdrop in ApplicationWindow | Main, auxiliary and standalone window surfaces |
| ContextMenu / Menu | Window fill color at 12% over a captured backdrop | 64 logical px | Compact and regular menus |
| Dense 75 MaterialSurface / PanelMaterial | #141414 at 75% | 64 logical px | Explicitly dense panels |
| Glass 25 | #141414 at 25% | 16 logical px | Tooltip, popover, default panel |

WindowMaterial uses Theme.materialWindowFill (#0B0B0B) as the full-surface color, independently of Primary, and disables both radial intensities. It remains uniform after resizing and during color changes. Its inherited Dense75 setting still supplies the 64px blur and dense edge defaults; its explicit fill opacity is 50%. Foreground content is not blurred or faded.

PanelMaterial keeps eight Primary-colored gradients at 40% and 11% intensity. The table below describes that panel treatment. Menus explicitly override these intensities to 8% of their normal strength.

| Accent | Center binding | X/Y radii | Intensity |
| --- | --- | --- | --- |
| Central intense | center −129 / −28 | 200 / 200 | 40% |
| Central faint | center +136 / +62 | 180 / 180 | 11% |
| Top left | left +48, top +32 | 200 / 200 | 40% |
| Top right | right −64, top +40 | 260 / 150 | 11% |
| Left edge | left, vertical center | 140 / 300 | 11% |
| Right edge | right, vertical center | 160 / 300 | 40% |
| Bottom edge | horizontal center, bottom | 380 / 150 | 11% |
| Bottom left | left +72, bottom −40 | 180 / 180 | 40% |

## API

`MaterialSurface` is the shared renderer. WindowMaterial supplies a uniform fill, while PanelMaterial supplies the radial treatment; both provide corner and elevation defaults.

- `density`: `MaterialSurface.Dense75` or `MaterialSurface.Glass25`.
- `primaryColor`: defaults to `Theme.primary`, which falls back to `#0A84FF`.
- `color`, `tintOpacity`: full-surface color and opacity. WindowMaterial defaults to #0B0B0B at 0.50; PanelMaterial defaults to a neutral tint.
- `blurRadius`, `blurEnabled`: background diffusion radius and switch.
- `radius`, `borderWidth`, `borderColor`: surface silhouette and edge.
- `intenseOpacity`, `faintOpacity`: radial intensities, 0 / 0 for WindowMaterial and 0.40 / 0.11 for PanelMaterial. Zero intensities create no radial shapes.
- `backdropSource`, `backdropBackground`: explicit sibling items to capture behind a transient surface. A popup captures application content and background separately so that the overlay never enters its own capture.
- `outlinePath`: optional SVG path in local coordinates; Tooltip uses its existing body-and-tail outline.
- `effectsActive`, `captureActive`, `resolvedBackdropSource`: read-only rendering/capture diagnostics.

## Usage

```qml
import LVRS as LV

LV.ApplicationWindow {
    primaryColor: "#A571E6"
    // Uniform #0B0B0B fill at 50% and native macOS backdrop blur are defaults.
    LV.Popover {
        id: details
        width: 320; height: 180
        contentItem: DetailsView {}
    }
}
```

Use a material as a background and place content above it. `ApplicationWindow.windowColor` defaults to Theme.materialWindowFill (#0B0B0B) and can override the full-surface fill. `windowBackgroundOpacity` overrides its 50% default without fading the window or its content. Consumers can still override the standard `background` property. Native chrome owns the full-window outer edge, so the default window background disables its own border, corner rounding and elevation.

## How It Works

The existing Qt Quick Shapes and Qt Quick Effects modules provide radial fills, silhouette masking, texture capture and `MultiEffect` diffusion. There is no additional library dependency. Background/content captures are composited with the accent before diffusion; the neutral tint and outline are drawn afterward. The capture follows ancestor transforms and rejects the material itself, its ancestors and descendants to prevent feedback.

The software scene graph keeps surface colors, transparency and outlines without GPU blur. Native RHI rendering (including macOS Metal) applies background diffusion. Transient surfaces blur their in-window sources. Separately, `ApplicationWindow` installs an AppKit `NSVisualEffectView` beneath Qt's native render view on macOS, using `UnderWindowBackground`, `BehindWindow` blending and an active frosted material. Qt clears to transparent, while the uniform near-black QML fill is 50%; the native window and foreground retain alpha 1. The effect follows native resizing, does not intercept input and is removed when `backgroundBlurEnabled` is false. Chrome updates preserve the native effect.

The 64px QML diffusion and 50% fill are explicit LVRS values. AppKit controls its own native blur kernel and adaptive material tint; there is no public pixel-radius setting for that kernel. Unsupported platforms retain the opaque window-color fallback and report `backgroundBlurSupported: false`. No screen capture, private APIs, or extra dependency is required.

Native references: [Apple NSVisualEffectView](https://developer.apple.com/documentation/appkit/nsvisualeffectview), [UnderWindowBackground](https://developer.apple.com/documentation/appkit/nsvisualeffectview/material-swift.enum/underwindowbackground), [Qt window alpha buffers](https://doc.qt.io/qt-6.8/qquickwindow.html#setDefaultAlphaBuffer).

Figma references: [Window](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=944-31), [Panel](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=944-28), [ApplicationWindow](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=997-3).

Validation: `LVRSTests_nativewindowblur` verifies the actual macOS view placement, alpha, blur mode, resize, input, toggling and native surface recreation (run with `QT_QPA_PLATFORM=cocoa QSG_RHI_BACKEND=metal`). `LVRSTests_materials` covers edge color after resizing to 900×600 and 1600×1000, defaults, app-accent changes, transient surfaces, unsafe capture rejection, native backdrop contrast reduction and popup interaction in VisualCatalog. `LVRSTests_tooltip` retains placement, tail and content behavior, including galleries hosted by a plain `QQuickWindow`. VisualCatalog exposes all four public material/popover types.

## Shared motion

Tint and opacity blend through MaterialSurface; the host window keeps native move and resize behavior. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.

The Figma diffusion layer includes a zero-blur white inset highlight at (0, 1): Dense 6%, Glass 14%. `innerHighlightOpacity` and `innerHighlightOffsetY` expose these authored values. Native RHI rendering subtracts the shifted silhouette using the existing mask, including custom tails; the software renderer retains its existing effect-free fallback.

ContextMenu and Menu use WindowMaterial with an explicit frosted coating: 12% of the window's fill color, 64px blur, and 8% of the normal radial accent intensity. Tooltip and Popover retain PanelMaterial Glass 25. See [ContextMenu](../navigation/ContextMenu.md#window-derived-frosted-menu).

Menus use the window’s `materialBackdropSource` content host plus its background as separate sources. The host excludes popup overlays, preserving the existing ancestor/descendant capture guard while allowing foreground content behind the menu to be diffused.

`backdropBaseColor` defaults to transparent. In-window menus use the opaque neutral Theme.materialTint as a capture base so the blurred composite replaces the covered pixels; otherwise the captured window alpha would let original sharp text bleed through. Keeping this base neutral also prevents the uniform colored window from becoming a second opaque accent fill. The visible menu coating remains 12%.

The near-black fill revision was verified on macOS/Metal on 2026-09-12: 53 LVRS CTest cases and 9 native material checks passed. Rendered fill samples remained #0B0B0B at 50% alpha with blue and purple accents at 900×600 and 1600×1000. All 83 installed QML resources matched the current source. Society passed 16 tests and Dreamscapes passed 5, including separate near-black fill and app-accent expectations. Society, Dreamscapes and VisualCatalog were rebuilt and relaunched with the updated workspace runtime; both app windows were visually inspected.

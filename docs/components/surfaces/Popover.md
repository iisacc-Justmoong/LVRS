# Popover

## Purpose

`Popover` is a composable Qt Quick Controls Popup using the standard [Glass 25 material](Materials.md). It inherits the application Primary color and leaves foreground content opaque.

## API

The normal Popup API applies: `open()`, `close()`, `x`, `y`, `width`, `height`, `contentItem`, `contentChildren` and dismissal policy. Added properties are `primaryColor`, `cornerRadius`, `surfaceColor`, `surfaceOpacity`, `backdropSource`, `backdropBackground` and read-only `material`.

Defaults are nonmodal, no dimming, focus enabled, 12px padding, 8px margins, and Escape / outside-press dismissal. The Item popup type keeps the captured application layers in the same scene graph.

## Usage

```qml
LV.Popover {
    id: details
    x: 80; y: 120
    width: 320; height: 180
    contentItem: DetailsView {}
}
// details.open()
```

## How It Works

`PanelMaterial` supplies the background with a 25% neutral tint, 16px diffusion and the current Primary radial color. Application content and background are captured separately from the popup overlay. For a custom host, provide safe sibling capture sources or leave them null for the tinted radial surface.

Figma: [Popover](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=997-24). VisualCatalog: `popover`; tests: `LVRSTests_materials`.

## Shared motion

The panel scales from 92% with rebound and fades away on dismissal. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.

# ContextMenu

Location: `qml/components/navigation/ContextMenu.qml`

`ContextMenu` is a popup menu with runtime-tuned open animation and global outside-dismiss bridging.

## Purpose

- Render heterogeneous menu models (string/object/divider).
- Emit normalized event signals and optional callback context.
- Keep close behavior deterministic across overlay/global-event paths.

## Core API

Data and selection:

- `items`
- `selectedIndex`
- `entryCount` (readonly)

Open/close behavior:

- `autoCloseOnTrigger`
- `dismissOnGlobalPress`
- `dismissOnGlobalContextRequest`
- `openAt(x, y)`
- `openFor(item, x, y)`
- `dismissIfOutsideGlobalEvent(eventData)`
- `triggerEntry(index)`

Visual/layout:

- `itemWidth`, `itemSpacing` (default `Theme.gap2`)
- `resolvedItemWidth` (readonly)
- `showIconSlot` (default `true`; forwarded to default `ContextMenuItem` rows)
- `itemDelegate`: optional component used for non-divider entries.
- `dividerDelegate`: optional component used for divider entries.
- `menuColor`, `menuOpacity`, `resolvedMenuColor` (default window background color, 12% surface tint)
- `menuBlurRadius` (64 logical pixels), `menuAccentStrength` (0.08 of the standard radial accent coating)
- `primaryColor`, `backdropSource`, `backdropBackground`: window accent and safe in-window capture sources; foreground content stays opaque.
- `dividerColor` (default `Theme.contextMenuDivider`, 30% white)
- `edgeMargin` (viewport inset used by auto placement; default `Theme.gap4`)
- `openHorizontalDirection`, `openVerticalDirection` (last resolved placement direction)

Animation tuning:

- `enableOpenBounce`, `autoTuneByBackend`
- `openBounceDuration`, `openSettleDuration`
- `openStartScale`, `openOvershootScale`
- resolved: `resolvedOpenBounceEnabled`, `resolvedOpenReboundDuration`, `resolvedOpenBackOvershoot`

Signals:

- `itemTriggered(index, item)`
- `itemEventTriggered(eventName, payload, index, item)`

## Entry Object Contract

Supported object fields include:

- canonical compact item fields: `icon`, `label`, `keyVisible`, `key`
- label/text: `label`, `text`, `title`
- icon: `iconName`/`icon`, `iconSource`/`source`
- icon slot visibility: `showIconSlot` or `iconSlotVisible`
- key text: `key`, `shortcut`, `keyText`
- key visibility: `keyVisible`, `shortcutVisible`, `showShortcut`
- state: `enabled`, `state`, `selected`
- chevron/children: `showChevron`, `hasChildItems`, `hasSubmenu`, `expanded`, `selectionDirection`
- event: `eventName`/`event`/`action`, `eventPayload`/`payload`, `events[]`
- callback: `onTriggered`, `onClicked`, `handler`
- close policy: `closeOnTrigger`, `autoClose`, `keepOpen`, `preventClose`
- divider: `type: "divider"` or `divider: true`

Callback receives context `{ index, item, menu, eventName, payload, emit(), close() }`.

Chevron render condition is `showChevron && hasChildItems` (resolved from entry fields).
Shortcut visibility defaults to `true` only when shortcut text exists; entries without `key`/`shortcut`/`keyText` do not reserve trailing shortcut space unless explicitly requested.
Icon slot visibility defaults to the menu-level `showIconSlot` value. Individual entries can override it with `showIconSlot` or the compatibility alias `iconSlotVisible`.

## Delegate Contract

- Each non-divider entry instantiates `itemDelegate`; each divider entry instantiates `dividerDelegate`.
- Both delegates receive one injected `modelData` object and should declare `property var modelData`.
- `modelData` includes `index`, `entry`, `divider`, `label`, `shortcut`, `keyVisible`, `showIconSlot`, `iconName`, `iconSource`, `showChevron`, `hasChildItems`, `expanded`, `selectionDirection`, `enabled`, `state`, and `trigger()`.
- `modelData.trigger()` invokes `triggerEntry(index)`, emits normalized menu signals, runs callbacks/events, and applies close policy.
- Custom delegates are responsible for wiring their own click/tap action to `modelData.trigger()`.

## Placement Contract

- `openAt(x, y)` performs built-in edge-aware placement against overlay bounds.
- Default preference is right/down from anchor point.
- If space is insufficient, placement flips to left and/or up.
- Final position is clamped to viewport with `edgeMargin`.
- `resolveOpenPlacement(...)` exposes the internal placement solver for deterministic testing and tooling.

## Width Contract

- `itemWidth` is the baseline minimum row width.
- `resolvedItemWidth` expands to the larger of baseline `itemWidth`, the widest delegate implicit width, and any explicit popup width supplied by the caller.
- The popup frame itself is promoted to at least `implicitWidth`, so a narrow explicit `width` cannot clamp the menu below its content-driven size.
- Delegate rows and dividers consume `resolvedItemWidth`, so a combo-triggered menu can grow wider than the trigger itself when content or caller sizing requires it.
- Delegate `MenuItem` rows remain responsive inside `resolvedItemWidth`, so constrained menus do not push label/shortcut/chevron content outside the popup bounds or collapse the internal spacer into negative geometry.
- Width probing uses each row's unconstrained natural content width, so visible text elision does not feed back into popup sizing.
- Width probing uses the same compact/regular font contract as rendered rows. Custom delegates that need wider chrome should set `itemWidth` or explicit popup `width`.
- Width probing forwards the same icon-slot visibility as rendered delegates, so `showIconSlot: false` reduces the content-driven menu width instead of leaving a hidden leading gutter.

## Visual Contract

- Figma source: `331:9332`, composed from `331:9282` and `331:9283`.
- Minimum item width 141px, 8px padding on all four sides, 2px row gap.
- Five 18px rows plus one 3px divider produce the reference 157 × 119px popup.
- Compact labels use Inter Regular 12, shortcuts use Pretendard SemiBold 12.
- The divider uses white at 30% opacity with a 4px horizontal inset.
- The popup uses WindowMaterial with a 12px menu radius, 12% tint, 64px blur and 8% of the standard accent-gradient strength.
- For the separate Figma Menu family (110:857), use Menu: 24px rows, 4px vertical padding and the panel08 full-width divider.

## Usage

```qml
import LVRS 1.0 as LV

LV.ContextMenu {
    id: menu
    showIconSlot: false
    items: [
        { label: "Copy", eventName: "menu.copy", showChevron: false },
        { type: "divider" },
        { label: "Inspect", showIconSlot: true, keepOpen: true, events: ["menu.inspect"] }
    ]
}
```

```qml
Component {
    id: destructiveMenuRow

    LV.MenuItem {
        property var modelData: ({})
        label: modelData.label || ""
        enabled: modelData.enabled === true
        onClicked: modelData.trigger()
    }
}

LV.ContextMenu {
    itemDelegate: destructiveMenuRow
    items: [{ label: "Delete", eventName: "delete" }]
}
```

## Shared motion

The menu scales in from its resolved origin with rebound, then fades on dismissal. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.

The Figma compact default uses 141px minimum item width, 18px rows and 8px padding on every side. Five reference rows and one divider produce 157 × 119px. `compactItems` defaults true and also controls the width probe typography. Use `Menu` to preserve the regular 24px rows and 4px vertical padding.

## Window-derived frosted menu

The 2026-09-12 visual revision follows the user's glass treatment request and supersedes the earlier Figma Glass 25 menu coating. ContextMenu and Menu share WindowMaterial, the window's fill color and primary palette, and its 48px/16px/30% elevation. The compact/regular row layout is independent of this material.

`menuOpacity` controls only neutral tint (default 0.12), `menuBlurRadius` controls backdrop blur (64 logical pixels), and `menuAccentStrength` scales the additional radial coating (0.08). The captured window already carries its accent lighting, so a second full-strength layer would make the menu overly blue. Glass edges and the inset highlight remain visible. Popup opacity remains 1, keeping labels and selected rows fully opaque. Explicit menuColor, backdrop sources and these three material values remain overridable.

Native material tests compare the old 25%/16px/full-accent treatment with the new treatment, measure reduced blue concentration, verify real backdrop diffusion, and test both menu types and overrides.

Default content capture uses `ApplicationWindow.materialBackdropSource`, which excludes the popup overlay. Capturing the full Qt window content ancestor is rejected by MaterialSurface to avoid feedback; using the safe app-content host ensures text and controls behind the menu actually participate in the blur. Other window implementations can continue supplying an explicit safe `backdropSource`.

The capture is composited over Theme.materialTint, an opaque neutral base, before diffusion. Keeping this base neutral prevents a colored Window fill from becoming a second opaque accent layer. This prevents a second, unblurred copy of the covered app text from showing through the capture alpha. Native tests measure the final visible window pixels over a stripe pattern, not only the isolated effect texture.

Verified on macOS/Metal on 2026-09-12: all 53 LVRS CTest cases and all 9 native material checks passed. The installed runtime matched 83 source QML files. Society (16 tests) and Dreamscapes (5 tests) were rebuilt, passed their suites, and were relaunched; their generation-count menus visibly diffused the underlying content while retaining crisp labels and selection. Both running apps loaded the updated workspace runtime. VisualCatalog was also rebuilt and relaunched with the same runtime.

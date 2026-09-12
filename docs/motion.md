# Interaction and motion

LVRS ships one motion policy for its component family. The implementation uses the existing Qt 6.8 Quick animation engine; there is no additional runtime, license or package dependency. Qt already provides interruptible property Behaviors and NumberAnimation, so a separate animation library would duplicate the installed framework. See [Qt Behavior](https://doc.qt.io/qt-6/qml-qtquick-behavior.html) and [Qt NumberAnimation](https://doc.qt.io/qt-6/qml-qtquick-numberanimation.html).

## Global policy

```qml
import LVRS as LV
// Shared within this QML engine; configure in the application root.
Component.onCompleted: {
    LV.Motion.speed = 1.0
    LV.Motion.reducedMotion = false
}
```

| Setting / token | Default | Meaning |
| --- | --- | --- |
| enabled | true | Enable animated presentation |
| reducedMotion | false | Application-controlled accessibility preference; no automatic OS detection is claimed |
| speed | 1 | Playback speed; finite values clamp to 0.1–4; invalid values use 1 |
| pressDuration | 90 ms | Immediate-feeling compression |
| hoverDuration | 160 ms | Hover and focus entry |
| releaseDuration | 360 ms | One elastic return |
| surfaceDuration | 420 ms | Larger popup / page presentation |
| exitDuration | 150 ms | Prompt dismissal |
| colorDuration | 130 ms | Semantic fill blending |
| overshoot | 1.45 | Shared OutBack curve |

`Motion.duration(ms)` applies speed and the global policy. Numeric and color Behaviors settle an in-flight animation when disabled. Controls that expose `motionEnabled` can opt out locally. ToggleSwitch `transitionDuration`, Sheet / Tooltip `animationDuration`, ContextMenu bounce settings and PageRouter gesture-settle duration remain explicit overrides.

## Interaction contracts

- AbstractButton supplies pointer, touch and keyboard compression to its button, card, menu, list and toolbar subclasses. Transforms are attached to the content and background slots, preserving the root hit target, layout width, height and implicit size. Visual displacement is capped for large targets. A keyboard focus ring does not intercept input.
- Stepper and ComboBox apply the same response to their compact composite. Signals and injected callbacks execute at activation, independently of animation completion.
- CheckBox marks and RadioButton dots grow into the selected state. ToggleSwitch retains its knob squash, travel stretch, native drag and endpoint rebound.
- Slider value and pointer mapping remain immediate during a drag. Programmatic / keyboard changes animate a displayed position clamped to the track; the visible thumb reacts to press. ProgressBar animates displayed progress while clamping the fill to its track and preserving the actual numeric model.
- AbstractInputBar / InputField, TextEditor and CodeEditor animate a focus outline. Text, caret, selection and IME coordinates are not scaled or delayed. ColorPicker composes those controls and sliders.
- TableCellItem blends selection/current tint; Table, TableHeader and TableRow inherit their delegates' input feedback. Resize geometry is direct.
- ContextMenu, Popover and Tooltip rebound on entry and fade on exit. Sheet uses bottom-edge translation on mobile and modest scale on desktop. Alert and Modal keep the closing visual layer alive until their exit finishes.
- Hierarchy disclosure arrows rotate on expansion; scrolling rebound uses the same curve. Dragging and model order remain synchronous. PageRouter uses a small scale transition so it does not compete with gesture-owned page x; Navigator and the transition helpers forward to it.
- HStack / VStack animate explicit spacing changes, Spacer animates minimum length, and ZStack / Label / MenuDivider animate explicit opacity changes. Layout-managed child geometry is never given a competing Behavior.
- MaterialSurface, PanelMaterial, WindowMaterial and AppCard blend authored tint changes. Native windows retain platform move/resize interactions; their drawers and content controls use LVRS motion.
- EventListener, guards, Theme, method dispatch and geometry observers are nonvisual. They forward state/events to the visible consumer rather than manufacturing an independent input surface.

## Reusable primitives

```qml
Rectangle {
    id: tile
    width: 160; height: 80
    property bool expanded: false
    color: expanded ? LV.Theme.primary : LV.Theme.panelBackground08
    LV.StateColorBehavior on color {}
    rotation: expanded ? 8 : 0
    LV.SpringBehavior on rotation {}
    transform: LV.InteractionMotion {
        target: tile
        pressed: pointer.pressed
        hovered: pointer.containsMouse
    }
    MouseArea { id: pointer; anchors.fill: parent; hoverEnabled: true; onClicked: tile.expanded = !tile.expanded }
    LV.FocusRing { anchors.fill: parent; active: tile.activeFocus }
}
```

Use a monotonic easing (OutCubic) for opacity and bounded data geometry where overshoot is inappropriate. Prefer native direct tracking for continuous input. Do not add animations to layout-owned child x/y/width/height or to backend model values.

## VisualCatalog verification

The Component Studio indexes every shipped QML file plus WindowSafeAreaObserver (84 entries). Search matches type, summary and source path. Each entry has an explicit recipe in `CatalogMotion.js`, a real consumer playground, a response description, inspection guidance and a usage/source reference. Internal renderers are marked as supporting types. Shared family previews show composition and variants.

Use the global 1× / 0.5× / 0.25× controls to inspect timing, Reduce motion for the immediate state, and Reset preview to recreate demo state. The Motion playground demonstrates action counts, selection, focus, value changes, layout spacing and passive opacity without changing production component APIs.

```bash
cmake -S . -B build -DLVRS_BUILD_TESTS=ON -DLVRS_BUILD_EXAMPLES=ON
cmake --build build -j 8
env DYLD_LIBRARY_PATH="$PWD/build" QML_IMPORT_PATH="$PWD/build" QML2_IMPORT_PATH="$PWD/build" ctest --test-dir build --output-on-failure
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

`LVRSTests_motion` exercises all 18 button families, pointer/keyboard/touch input, interrupted rebound, disabled/local opt-out, reduced motion during an animation, stable layout and direct slider dragging. `LVRSTests_catalog` verifies every QML file has a recipe, loads every indexed preview, and checks search/reset. The existing tests cover component semantics and embedded source-resource parity. Native Metal visual inspection is a separate check from offscreen tests. Installed packages must be validated separately from this build.

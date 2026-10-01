# Component instance states

The LVRS Figma library and runtime use a state owned by each complete component
instance. A label, icon, shortcut, disclosure and accessory remain inside that
instance through every input phase. A second interaction component is not placed
over the component to represent hover, press, release or keyboard focus.

## Figma contract — 2026-09-30

The [LVRS library](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System)
uses `Interaction state=default/hover/press/release/focus`. Row families additionally
have `Input=pointer/keyboard`, independent of their selected/inactive state.
Keyboard focus decoration belongs to the row and remains visible during press
and release. Root fills retain their bound variables. Prototype `CHANGE_TO`
actions target variants in the same component set.

| Family | Component sets | Complete variants | Review frame |
| --- | ---: | ---: | --- |
| PushButton | 2 (label/icon) | 50 | [Button](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-728) |
| DropdownButton | 1 | 30 | Button page |
| ListItem | 17 (one per type) | 510 | [ListItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-6737) |
| MenuItem | 2 (collapsed/expanded) | 60 | [MenuItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-7886) |
| ContextMenuItem | 1 | 30 | [ContextMenuItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-8219) |
| HierarchyItem | 2 (foldable/non-foldable) | 60 | [HierarchyItem](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1250-9422) |

Each set has at most 30 variants. Change a ListItem type or label/icon button
family by swapping its component set; change `Interaction state` on that
instance to preview input. The original component IDs remain in their sets.
The existing 539 consumer instances were migrated with their phase and custom
properties. Text/visibility property overrides were exercised on all 740 variants.
After splitting the families, unused properties from other types were removed;
each set exposes only its actual content properties and state axes. All 740
instance overrides were checked again after this cleanup.

The Button, List, Menus, ContextMenu and Hierarchy interaction helper sets were
removed after their references reached zero. An audit of 23 component pages found
no remaining references to those helpers. Alert, Card, ColorPicker and Tabs inherit
the converted button instances. Popup/modal overlays and imported platform
components' internal state-layer frames serve different purposes and are retained.
The remaining seven thumbnail, separator, Avatar, color, icon and text pages were
also inspected and contain no interaction helper components. Avatar is empty.

## Runtime API

`AbstractButton` creates one nonvisual `InteractionState` object per instance.
PushButton, DropdownButton, their named label/icon presets, MenuItem,
ContextMenuItem, ListItem and HierarchyItem inherit this ownership.

| Property | Meaning |
| --- | --- |
| `interaction` | The instance's state object; never a shared singleton |
| `interactionPhase` | `disabled`, `press`, `release`, `focus`, `hover` or `default`, in priority order |
| `interactionInput` | `keyboard` while native visual focus is active; otherwise `pointer` (including touch) |
| `interaction.focusVisible` | Enabled keyboard focus, independent of press/release |
| `interaction.surfacePhase` | `disabled`, `press`, `hover` or `default` for the existing surface color policy |

The enabled gate takes priority over input. HierarchyItem also gates interaction
when it cannot become active or is displaying a drag preview. Its existing drag
and selection APIs remain independent. MenuItem's integer `state`, ListItem's
`selected`/`type`/accessory values and HierarchyItem's `uxState`/expansion are not
replaced by an input phase.

The owned phase drives the component's existing background and internal focus
decoration. Focus and release do not introduce a new persistent fill. `surfacePhase`
keeps hover fill available while focus or release takes priority in `interactionPhase`.
Checked buttons retain their existing checked fill; embedded ListItem toggles do
not tint their enclosing row. Content clipping is scoped to content, leaving the
component's external focus outline visible without changing hit or layout bounds.

`InteractionMotion.releasing` reports a running release animation. Genuine pointer,
touch, Space or Enter release gives the authored families their existing 180ms
OutBack return. Cancellation skips release. Re-press interrupts it, and disabling
motion settles it immediately while logical press/focus continues to work.
See [motion policy](motion.md) for timing and accessibility settings.

```qml
import LVRS 1.0 as LV

LV.ListItem {
    label: "Document"
    selected: true
    onInteractionPhaseChanged: console.log(interactionPhase, interactionInput)
}
```

## Verification

`LVRSTests_instance_state` uses Qt Test with real Qt Quick windows to exercise
pointer and keyboard phases, separate ownership, all 17 ListItem types, activation
on release, cancellation, re-press, reduced motion, disabled gates, focus retention
and fixed hit geometry. It checks the recorded Figma contract in
`tests/fixtures/figma-instance-state.json`; that fixture is a dated design audit,
not a live Figma query. Run `ctest --test-dir build -R instance_state --output-on-failure`.
Existing motion, Figma parity, list composite and hierarchy suites cover visual
values, accessory routing and model behavior. Build outputs are independent of
an installed LVRS package; reinstalling is a separate consumer action.

For native rendered focus proof, set `LVRS_STATE_CAPTURE_DIR` to a directory under
`build/` and run `build/tests/LVRSTests_instance_state native_focus_capture` with
the native platform and this build's library/import paths. The capture checks
actual primary-colored pixels outside each of the six component families and
saves their focus frames. This check is opt-in in the normal offscreen suite.

Validation on macOS with Qt 6.8.3: the complete build passed. All 56 CTest targets
were exercised; the initial run passed 53, and the three failures (catalog counts
in examples/platform integration and an animation-start timing assertion) passed
after correction and targeted rerun. The instance-state suite and native rendered
focus checks passed. `build/figma-instance-state/verification.json` records these
separate runs, the Figma audit and the native captures. Installation is left to the
user's requested reinstall step.

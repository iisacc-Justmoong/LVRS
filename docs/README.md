# LVRS Documentation

This documentation set is organized around runtime behavior, API contracts, and integration policy.  
The primary objective is operational clarity: each document should answer what API exists, how to use it, and why the runtime behaves as it does.

## Reading Order

1. `docs/overview.md`
2. `docs/build.md`
3. `docs/mvvm.md`
4. `docs/architecture/event-pipeline.md`
5. `docs/architecture/rendering-backend.md`
6. `docs/architecture/performance-observability.md`
7. `docs/performance-baseline-p0.md`
8. `docs/quality-automation-p4.md`
9. `Chnagelog.md`
10. `src/rust-cli/README.md`

## Documentation Contract

Every API document follows the same structure:

- `Purpose`: what the module/component is responsible for.
- `API`: properties, methods, and signals that are intended for external usage.
- `Usage`: minimal and realistic call patterns.
- `How It Works`: internal data flow, normalization, fallback, and edge-case behavior.

This structure is intentional so the same question can be answered consistently regardless of whether the target is QML UI or C++ singleton backend.

## Core

- `docs/overview.md`
- `docs/build.md`
- `docs/theme.md`
- `docs/mvvm.md`

## Architecture

- `docs/architecture/event-pipeline.md`
- `docs/architecture/rendering-backend.md`
- `docs/architecture/performance-observability.md`

## Performance Baseline

- `docs/performance-baseline-p0.md`
- `docs/quality-automation-p4.md`

## Change Records and Tooling

- [Component instance states](instance-states.md): complete Figma variants, per-instance input ownership and regression checks.

- `Chnagelog.md` (repo root): commit-trace change summary by date.
- `src/rust-cli/README.md` (repo root): `lvrs` CLI entrypoints (`install`, `bootstrap`, `doctor`, `platform`).

## Backend Singletons

- `docs/backend/AppBootstrap.md`
- `docs/backend/BootstrapParallel.md`
- `docs/backend/ForegroundServices.md`
- `docs/backend/PermissionSequencer.md`
- `docs/backend/QmlContextBinder.md`
- `docs/backend/QmlTypeRegistrar.md`
- `docs/backend/Backend.md`
- `docs/backend/RuntimeEvents.md`
- `docs/backend/GestureEvents.md`
- `docs/backend/RenderQuality.md`
- `docs/backend/RenderMonitor.md`
- `docs/backend/RouteMatcher.md`
- `docs/backend/ModelAdapter.md`
- `docs/backend/StateModel.md`
- `docs/backend/ProgressModel.md`
- `docs/backend/PageMonitor.md`
- `docs/backend/ViewStateTracker.md`
- `docs/backend/ViewModels.md`
- `docs/backend/Platform.md`
- `docs/backend/Debug.md`
- `docs/backend/DebugOutput.md`
- `docs/backend/NativeWindowInteraction.md`

## Backend Model Types

- `docs/backend/ModelUndoStack.md`
- `docs/backend/ModelSource.md`
- `docs/backend/TextDocumentModel.md`
- `docs/backend/HierarchyModel.md`
- `docs/backend/TableHeaderModel.md`
- `docs/backend/TableModel.md`
- `docs/backend/NavigationStackModel.md`

## App Layer

- `docs/components/app/AppBootstrapWindow.md`
- `docs/components/app/ApplicationWindow.md`
- `docs/components/app/AppShell.md`
- `docs/components/app/Window.md`
- `docs/components/app/WindowChromeInteraction.md`
- `docs/components/app/WindowSafeAreaObserver.md`

## Control Components

- `docs/components/control/AbstractButton.md`
- `docs/components/control/PushButton.md`
- `docs/components/control/DropdownButton.md`
- `docs/components/control/ButtonMethodRegistry.md`
- `docs/components/control/LabelButton.md`
- `docs/components/control/IconButton.md`
- `docs/components/control/LabelSegmentedControl.md`
- `docs/components/control/IconSegmentedControl.md`
- `docs/components/control/LabelMenuButton.md`
- `docs/components/control/IconMenuButton.md`
- `docs/components/control/ComboBox.md`
- `docs/components/control/Stepper.md`
- `docs/components/control/Label.md`
- [Avatar](components/control/Avatar.md): circular, centered image profiles in three sizes.
- `docs/components/control/CheckBox.md`
- `docs/components/control/RadioButton.md`
- `docs/components/control/ToggleSwitch.md`
- `docs/components/control/InputField.md`
- `docs/components/control/Slider.md`
- `docs/components/control/ColorPicker.md`
- `docs/components/control/TextEditor.md`
- `docs/components/control/CodeEditor.md`
- `docs/components/control/ProgressBar.md`
- `docs/components/control/Table.md`
- `docs/components/control/TableHeader.md`
- `docs/components/control/TableRow.md`
- `docs/components/control/TableCellItem.md`
- `docs/components/control/EventListener.md`
- `docs/components/control/InputMethodGuard.md`
- `docs/components/control/WheelScrollGuard.md`

## Navigation Components

- `docs/components/navigation/Tabs.md`
- `docs/components/navigation/MobileNavigationBar.md`
- `docs/components/navigation/Navigator.md`
- `docs/components/navigation/PageRouter.md`
- `docs/components/navigation/PageTransitionController.md`
- `docs/components/navigation/Link.md`
- `docs/components/navigation/List.md`
- `docs/components/navigation/Hierarchy.md`
- `docs/components/navigation/HierarchyList.md`
- `docs/components/navigation/HierarchyItem.md`
- `docs/components/navigation/HierarchyToolbar.md`
- `docs/components/navigation/ContextMenu.md`
- `docs/components/navigation/MenuItem.md`
- `docs/components/navigation/MenuDivider.md`

## Surface Components

- `docs/components/surfaces/AppCard.md`
- `docs/components/surfaces/Card.md`
- `docs/components/surfaces/Sheet.md`
- `docs/components/surfaces/Tooltip.md`
- `docs/components/surfaces/Alert.md`

## Policies

- `docs/policies/event.md`
- `docs/policies/routing.md`
- `docs/policies/debug.md`

## Fast Lookup: Typical Questions

- How to wire global input event handling?  
  Start with `docs/architecture/event-pipeline.md`, then `docs/backend/GestureEvents.md`, then `docs/components/control/EventListener.md`.

- How to route pages with stack semantics and route params?  
  Start with `docs/components/navigation/PageRouter.md`, then `docs/policies/routing.md`.

- How to enforce MVVM ownership for writes?  
  Start with `docs/mvvm.md`, then `docs/backend/ViewModels.md`.

- How to inspect runtime behavior and debug output?  
  Start with `docs/backend/RuntimeEvents.md`, `docs/backend/Debug.md`, and `docs/backend/DebugOutput.md`.

- How to run install/bootstrap with current workflow?  
  Start with `docs/build.md`, then `src/rust-cli/README.md`.

## Publishing Notes for Static Site Generators

The docs are authored as standalone Markdown and can be consumed directly by common generators such as Docusaurus, MkDocs, VitePress, or Hugo.

Recommended navigation grouping:

- Core: `overview`, `build`, `theme`, `mvvm`
- Architecture: `event-pipeline`, `rendering-backend`
- Backend: singleton/runtime references
- Components: app, control, layout, navigation, surfaces
- Policies: event/routing/debug

For official publication, keep path names stable and avoid page title renames without redirect mapping.

## Authoring Quality Checklist

Before publishing updates:

1. Verify every API name matches source code declaration.
2. Verify every code snippet is syntactically valid for QML/C++ context.
3. Ensure default values and behavior notes are version-accurate.
4. Add at least one practical usage example for user-facing components.
5. Add failure/edge-case notes for runtime and backend APIs.

## Link Hygiene

To prevent broken links in static-site builds:

- Use workspace-relative links (`docs/...`) consistently.
- Keep file names in `PascalCase`/stable casing as they map to URLs in case-sensitive hosts.
- When moving pages, add redirect rules in the site generator configuration.

## Standard Materials

- [WindowMaterial, PanelMaterial and MaterialSurface](components/surfaces/Materials.md)
- [Popover](components/surfaces/Popover.md)
- [네이티브 InputField](native/InputField.md)

## Interaction and motion

See [the motion system](motion.md) for shared elastic feedback, accessibility controls, component coverage and VisualCatalog verification.

[Figma parity audit](figma-parity.md) · [HelpButton](components/control/HelpButton.md) · [ColorPickerButton](components/control/ColorPickerButton.md) · [Menu](components/navigation/Menu.md) · [ContextMenuItem](components/navigation/ContextMenuItem.md) · [ContextMenuDivider](components/navigation/ContextMenuDivider.md) · [ListFooter](components/navigation/ListFooter.md).

<!-- workspace-docs:document-map:start -->
## Human and agent document map

Markdown is the maintained human source. Agent JSON is a structured, source-traceable projection.

| Document | Agent JSON |
| --- | --- |
| [`AGENT.md`](../AGENT.md) | [JSON](entrypoints/AGENT.agent.json) |
| [`AGENTS.md`](../AGENTS.md) | [JSON](entrypoints/AGENTS.agent.json) |
| [`Chnagelog.md`](../Chnagelog.md) | [JSON](entrypoints/Chnagelog.agent.json) |
| [`README.md`](../README.md) | [JSON](entrypoints/README.agent.json) |
| [`docs/Chnagelog.md`](Chnagelog.md) | [JSON](Chnagelog.agent.json) |
| [`docs/FILE_PROVIDER.md`](FILE_PROVIDER.md) | [JSON](FILE_PROVIDER.agent.json) |
| [`docs/NATIVE_LISTS.md`](NATIVE_LISTS.md) | [JSON](NATIVE_LISTS.agent.json) |
| [`docs/PERFORMANCE.md`](PERFORMANCE.md) | [JSON](PERFORMANCE.agent.json) |
| [`docs/Windows-validation.md`](Windows-validation.md) | [JSON](Windows-validation.agent.json) |
| [`docs/WindowsBuildAndRun.md`](WindowsBuildAndRun.md) | [JSON](WindowsBuildAndRun.agent.json) |
| [`docs/WindowsValidation.md`](WindowsValidation.md) | [JSON](WindowsValidation.agent.json) |
| [`docs/architecture/event-pipeline.md`](architecture/event-pipeline.md) | [JSON](architecture/event-pipeline.agent.json) |
| [`docs/architecture/performance-observability.md`](architecture/performance-observability.md) | [JSON](architecture/performance-observability.agent.json) |
| [`docs/architecture/rendering-backend.md`](architecture/rendering-backend.md) | [JSON](architecture/rendering-backend.agent.json) |
| [`docs/backend/AppBootstrap.md`](backend/AppBootstrap.md) | [JSON](backend/AppBootstrap.agent.json) |
| [`docs/backend/Backend.md`](backend/Backend.md) | [JSON](backend/Backend.agent.json) |
| [`docs/backend/BootstrapParallel.md`](backend/BootstrapParallel.md) | [JSON](backend/BootstrapParallel.agent.json) |
| [`docs/backend/Debug.md`](backend/Debug.md) | [JSON](backend/Debug.agent.json) |
| [`docs/backend/DebugOutput.md`](backend/DebugOutput.md) | [JSON](backend/DebugOutput.agent.json) |
| [`docs/backend/ForegroundServices.md`](backend/ForegroundServices.md) | [JSON](backend/ForegroundServices.agent.json) |
| [`docs/backend/GestureEvents.md`](backend/GestureEvents.md) | [JSON](backend/GestureEvents.agent.json) |
| [`docs/backend/HierarchyModel.md`](backend/HierarchyModel.md) | [JSON](backend/HierarchyModel.agent.json) |
| [`docs/backend/ModelAdapter.md`](backend/ModelAdapter.md) | [JSON](backend/ModelAdapter.agent.json) |
| [`docs/backend/ModelSource.md`](backend/ModelSource.md) | [JSON](backend/ModelSource.agent.json) |
| [`docs/backend/ModelUndoStack.md`](backend/ModelUndoStack.md) | [JSON](backend/ModelUndoStack.agent.json) |
| [`docs/backend/NativeWindowInteraction.md`](backend/NativeWindowInteraction.md) | [JSON](backend/NativeWindowInteraction.agent.json) |
| [`docs/backend/NavigationStackModel.md`](backend/NavigationStackModel.md) | [JSON](backend/NavigationStackModel.agent.json) |
| [`docs/backend/PageMonitor.md`](backend/PageMonitor.md) | [JSON](backend/PageMonitor.agent.json) |
| [`docs/backend/PermissionSequencer.md`](backend/PermissionSequencer.md) | [JSON](backend/PermissionSequencer.agent.json) |
| [`docs/backend/Platform.md`](backend/Platform.md) | [JSON](backend/Platform.agent.json) |
| [`docs/backend/ProgressModel.md`](backend/ProgressModel.md) | [JSON](backend/ProgressModel.agent.json) |
| [`docs/backend/QmlContextBinder.md`](backend/QmlContextBinder.md) | [JSON](backend/QmlContextBinder.agent.json) |
| [`docs/backend/QmlTypeRegistrar.md`](backend/QmlTypeRegistrar.md) | [JSON](backend/QmlTypeRegistrar.agent.json) |
| [`docs/backend/RenderMonitor.md`](backend/RenderMonitor.md) | [JSON](backend/RenderMonitor.agent.json) |
| [`docs/backend/RenderQuality.md`](backend/RenderQuality.md) | [JSON](backend/RenderQuality.agent.json) |
| [`docs/backend/RouteMatcher.md`](backend/RouteMatcher.md) | [JSON](backend/RouteMatcher.agent.json) |
| [`docs/backend/RuntimeEvents.md`](backend/RuntimeEvents.md) | [JSON](backend/RuntimeEvents.agent.json) |
| [`docs/backend/StateModel.md`](backend/StateModel.md) | [JSON](backend/StateModel.agent.json) |
| [`docs/backend/TableHeaderModel.md`](backend/TableHeaderModel.md) | [JSON](backend/TableHeaderModel.agent.json) |
| [`docs/backend/TableModel.md`](backend/TableModel.md) | [JSON](backend/TableModel.agent.json) |
| [`docs/backend/TextDocumentModel.md`](backend/TextDocumentModel.md) | [JSON](backend/TextDocumentModel.agent.json) |
| [`docs/backend/ViewModels.md`](backend/ViewModels.md) | [JSON](backend/ViewModels.agent.json) |
| [`docs/backend/ViewStateTracker.md`](backend/ViewStateTracker.md) | [JSON](backend/ViewStateTracker.agent.json) |
| [`docs/build.md`](build.md) | [JSON](build.agent.json) |
| [`docs/components/app/AppBootstrapWindow.md`](components/app/AppBootstrapWindow.md) | [JSON](components/app/AppBootstrapWindow.agent.json) |
| [`docs/components/app/AppShell.md`](components/app/AppShell.md) | [JSON](components/app/AppShell.agent.json) |
| [`docs/components/app/ApplicationWindow.md`](components/app/ApplicationWindow.md) | [JSON](components/app/ApplicationWindow.agent.json) |
| [`docs/components/app/Window.md`](components/app/Window.md) | [JSON](components/app/Window.agent.json) |
| [`docs/components/app/WindowChromeInteraction.md`](components/app/WindowChromeInteraction.md) | [JSON](components/app/WindowChromeInteraction.agent.json) |
| [`docs/components/app/WindowSafeAreaObserver.md`](components/app/WindowSafeAreaObserver.md) | [JSON](components/app/WindowSafeAreaObserver.agent.json) |
| [`docs/components/control/AbstractButton.md`](components/control/AbstractButton.md) | [JSON](components/control/AbstractButton.agent.json) |
| [`docs/components/control/Avatar.md`](components/control/Avatar.md) | [JSON](components/control/Avatar.agent.json) |
| [`docs/components/control/ButtonMethodRegistry.md`](components/control/ButtonMethodRegistry.md) | [JSON](components/control/ButtonMethodRegistry.agent.json) |
| [`docs/components/control/CheckBox.md`](components/control/CheckBox.md) | [JSON](components/control/CheckBox.agent.json) |
| [`docs/components/control/CodeEditor.md`](components/control/CodeEditor.md) | [JSON](components/control/CodeEditor.agent.json) |
| [`docs/components/control/ColorPicker.md`](components/control/ColorPicker.md) | [JSON](components/control/ColorPicker.agent.json) |
| [`docs/components/control/ColorPickerButton.md`](components/control/ColorPickerButton.md) | [JSON](components/control/ColorPickerButton.agent.json) |
| [`docs/components/control/ComboBox.md`](components/control/ComboBox.md) | [JSON](components/control/ComboBox.agent.json) |
| [`docs/components/control/DropdownButton.md`](components/control/DropdownButton.md) | [JSON](components/control/DropdownButton.agent.json) |
| [`docs/components/control/EventListener.md`](components/control/EventListener.md) | [JSON](components/control/EventListener.agent.json) |
| [`docs/components/control/HelpButton.md`](components/control/HelpButton.md) | [JSON](components/control/HelpButton.agent.json) |
| [`docs/components/control/IconButton.md`](components/control/IconButton.md) | [JSON](components/control/IconButton.agent.json) |
| [`docs/components/control/IconMenuButton.md`](components/control/IconMenuButton.md) | [JSON](components/control/IconMenuButton.agent.json) |
| [`docs/components/control/IconSegmentedControl.md`](components/control/IconSegmentedControl.md) | [JSON](components/control/IconSegmentedControl.agent.json) |
| [`docs/components/control/InputField.md`](components/control/InputField.md) | [JSON](components/control/InputField.agent.json) |
| [`docs/components/control/InputMethodGuard.md`](components/control/InputMethodGuard.md) | [JSON](components/control/InputMethodGuard.agent.json) |
| [`docs/components/control/Label.md`](components/control/Label.md) | [JSON](components/control/Label.agent.json) |
| [`docs/components/control/LabelButton.md`](components/control/LabelButton.md) | [JSON](components/control/LabelButton.agent.json) |
| [`docs/components/control/LabelMenuButton.md`](components/control/LabelMenuButton.md) | [JSON](components/control/LabelMenuButton.agent.json) |
| [`docs/components/control/LabelSegmentedControl.md`](components/control/LabelSegmentedControl.md) | [JSON](components/control/LabelSegmentedControl.agent.json) |
| [`docs/components/control/ProgressBar.md`](components/control/ProgressBar.md) | [JSON](components/control/ProgressBar.agent.json) |
| [`docs/components/control/PushButton.md`](components/control/PushButton.md) | [JSON](components/control/PushButton.agent.json) |
| [`docs/components/control/RadioButton.md`](components/control/RadioButton.md) | [JSON](components/control/RadioButton.agent.json) |
| [`docs/components/control/Slider.md`](components/control/Slider.md) | [JSON](components/control/Slider.agent.json) |
| [`docs/components/control/Stepper.md`](components/control/Stepper.md) | [JSON](components/control/Stepper.agent.json) |
| [`docs/components/control/Table.md`](components/control/Table.md) | [JSON](components/control/Table.agent.json) |
| [`docs/components/control/TableCellItem.md`](components/control/TableCellItem.md) | [JSON](components/control/TableCellItem.agent.json) |
| [`docs/components/control/TableHeader.md`](components/control/TableHeader.md) | [JSON](components/control/TableHeader.agent.json) |
| [`docs/components/control/TableRow.md`](components/control/TableRow.md) | [JSON](components/control/TableRow.agent.json) |
| [`docs/components/control/TextEditor.md`](components/control/TextEditor.md) | [JSON](components/control/TextEditor.agent.json) |
| [`docs/components/control/ToggleSwitch.md`](components/control/ToggleSwitch.md) | [JSON](components/control/ToggleSwitch.agent.json) |
| [`docs/components/control/WheelScrollGuard.md`](components/control/WheelScrollGuard.md) | [JSON](components/control/WheelScrollGuard.agent.json) |
| [`docs/components/layout/AppHeader.md`](components/layout/AppHeader.md) | [JSON](components/layout/AppHeader.agent.json) |
| [`docs/components/layout/HStack.md`](components/layout/HStack.md) | [JSON](components/layout/HStack.agent.json) |
| [`docs/components/layout/Spacer.md`](components/layout/Spacer.md) | [JSON](components/layout/Spacer.agent.json) |
| [`docs/components/layout/VStack.md`](components/layout/VStack.md) | [JSON](components/layout/VStack.agent.json) |
| [`docs/components/layout/ZStack.md`](components/layout/ZStack.md) | [JSON](components/layout/ZStack.agent.json) |
| [`docs/components/navigation/ContextMenu.md`](components/navigation/ContextMenu.md) | [JSON](components/navigation/ContextMenu.agent.json) |
| [`docs/components/navigation/ContextMenuDivider.md`](components/navigation/ContextMenuDivider.md) | [JSON](components/navigation/ContextMenuDivider.agent.json) |
| [`docs/components/navigation/ContextMenuItem.md`](components/navigation/ContextMenuItem.md) | [JSON](components/navigation/ContextMenuItem.agent.json) |
| [`docs/components/navigation/Hierarchy.md`](components/navigation/Hierarchy.md) | [JSON](components/navigation/Hierarchy.agent.json) |
| [`docs/components/navigation/HierarchyItem.md`](components/navigation/HierarchyItem.md) | [JSON](components/navigation/HierarchyItem.agent.json) |
| [`docs/components/navigation/HierarchyList.md`](components/navigation/HierarchyList.md) | [JSON](components/navigation/HierarchyList.agent.json) |
| [`docs/components/navigation/HierarchyToolbar.md`](components/navigation/HierarchyToolbar.md) | [JSON](components/navigation/HierarchyToolbar.agent.json) |
| [`docs/components/navigation/Link.md`](components/navigation/Link.md) | [JSON](components/navigation/Link.agent.json) |
| [`docs/components/navigation/List.md`](components/navigation/List.md) | [JSON](components/navigation/List.agent.json) |
| [`docs/components/navigation/ListFooter.md`](components/navigation/ListFooter.md) | [JSON](components/navigation/ListFooter.agent.json) |
| [`docs/components/navigation/ListItem.md`](components/navigation/ListItem.md) | [JSON](components/navigation/ListItem.agent.json) |
| [`docs/components/navigation/Menu.md`](components/navigation/Menu.md) | [JSON](components/navigation/Menu.agent.json) |
| [`docs/components/navigation/MenuDivider.md`](components/navigation/MenuDivider.md) | [JSON](components/navigation/MenuDivider.agent.json) |
| [`docs/components/navigation/MenuItem.md`](components/navigation/MenuItem.md) | [JSON](components/navigation/MenuItem.agent.json) |
| [`docs/components/navigation/MobileNavigationBar.md`](components/navigation/MobileNavigationBar.md) | [JSON](components/navigation/MobileNavigationBar.agent.json) |
| [`docs/components/navigation/Navigator.md`](components/navigation/Navigator.md) | [JSON](components/navigation/Navigator.agent.json) |
| [`docs/components/navigation/PageRouter.md`](components/navigation/PageRouter.md) | [JSON](components/navigation/PageRouter.agent.json) |
| [`docs/components/navigation/PageTransitionController.md`](components/navigation/PageTransitionController.md) | [JSON](components/navigation/PageTransitionController.agent.json) |
| [`docs/components/navigation/Tabs.md`](components/navigation/Tabs.md) | [JSON](components/navigation/Tabs.agent.json) |
| [`docs/components/surfaces/Alert.md`](components/surfaces/Alert.md) | [JSON](components/surfaces/Alert.agent.json) |
| [`docs/components/surfaces/AppCard.md`](components/surfaces/AppCard.md) | [JSON](components/surfaces/AppCard.agent.json) |
| [`docs/components/surfaces/Card.md`](components/surfaces/Card.md) | [JSON](components/surfaces/Card.agent.json) |
| [`docs/components/surfaces/Materials.md`](components/surfaces/Materials.md) | [JSON](components/surfaces/Materials.agent.json) |
| [`docs/components/surfaces/Modal.md`](components/surfaces/Modal.md) | [JSON](components/surfaces/Modal.agent.json) |
| [`docs/components/surfaces/Popover.md`](components/surfaces/Popover.md) | [JSON](components/surfaces/Popover.agent.json) |
| [`docs/components/surfaces/Sheet.md`](components/surfaces/Sheet.md) | [JSON](components/surfaces/Sheet.agent.json) |
| [`docs/components/surfaces/Tooltip.md`](components/surfaces/Tooltip.md) | [JSON](components/surfaces/Tooltip.agent.json) |
| [`docs/documentation-structure.md`](documentation-structure.md) | [JSON](documentation-structure.agent.json) |
| [`docs/figma-iconset-import.md`](figma-iconset-import.md) | [JSON](figma-iconset-import.agent.json) |
| [`docs/figma-parity.md`](figma-parity.md) | [JSON](figma-parity.agent.json) |
| [`docs/figma-theme-tokens.md`](figma-theme-tokens.md) | [JSON](figma-theme-tokens.agent.json) |
| [`docs/hierarchy-ux-plan.md`](hierarchy-ux-plan.md) | [JSON](hierarchy-ux-plan.agent.json) |
| [`docs/instance-states.md`](instance-states.md) | [JSON](instance-states.agent.json) |
| [`docs/motion.md`](motion.md) | [JSON](motion.agent.json) |
| [`docs/mvvm.md`](mvvm.md) | [JSON](mvvm.agent.json) |
| [`docs/native/ActivationContent.md`](native/ActivationContent.md) | [JSON](native/ActivationContent.agent.json) |
| [`docs/native/Alert.md`](native/Alert.md) | [JSON](native/Alert.agent.json) |
| [`docs/native/AppSurfaces.md`](native/AppSurfaces.md) | [JSON](native/AppSurfaces.agent.json) |
| [`docs/native/Architecture.md`](native/Architecture.md) | [JSON](native/Architecture.agent.json) |
| [`docs/native/CodeEditor.md`](native/CodeEditor.md) | [JSON](native/CodeEditor.agent.json) |
| [`docs/native/ColorPicker.md`](native/ColorPicker.md) | [JSON](native/ColorPicker.agent.json) |
| [`docs/native/ColorPickerButton.md`](native/ColorPickerButton.md) | [JSON](native/ColorPickerButton.agent.json) |
| [`docs/native/EditorAppearance.md`](native/EditorAppearance.md) | [JSON](native/EditorAppearance.agent.json) |
| [`docs/native/FontPolicy.md`](native/FontPolicy.md) | [JSON](native/FontPolicy.agent.json) |
| [`docs/native/Hierarchy.md`](native/Hierarchy.md) | [JSON](native/Hierarchy.agent.json) |
| [`docs/native/InputField.md`](native/InputField.md) | [JSON](native/InputField.agent.json) |
| [`docs/native/HierarchyController.md`](native/HierarchyController.md) | [JSON](native/HierarchyController.agent.json) |
| [`docs/native/HierarchyDelegate.md`](native/HierarchyDelegate.md) | [JSON](native/HierarchyDelegate.agent.json) |
| [`docs/native/HierarchyViews.md`](native/HierarchyViews.md) | [JSON](native/HierarchyViews.agent.json) |
| [`docs/native/Link.md`](native/Link.md) | [JSON](native/Link.agent.json) |
| [`docs/native/Materials.md`](native/Materials.md) | [JSON](native/Materials.agent.json) |
| [`docs/native/Media.md`](native/Media.md) | [JSON](native/Media.agent.json) |
| [`docs/native/Modal.md`](native/Modal.md) | [JSON](native/Modal.agent.json) |
| [`docs/native/ModelSources.md`](native/ModelSources.md) | [JSON](native/ModelSources.agent.json) |
| [`docs/native/ModelUndoStack.md`](native/ModelUndoStack.md) | [JSON](native/ModelUndoStack.agent.json) |
| [`docs/native/Navigation.md`](native/Navigation.md) | [JSON](native/Navigation.agent.json) |
| [`docs/native/NavigationTracking.md`](native/NavigationTracking.md) | [JSON](native/NavigationTracking.agent.json) |
| [`docs/native/Navigator.md`](native/Navigator.md) | [JSON](native/Navigator.agent.json) |
| [`docs/native/Opacity.md`](native/Opacity.md) | [JSON](native/Opacity.agent.json) |
| [`docs/native/PageRouter.md`](native/PageRouter.md) | [JSON](native/PageRouter.agent.json) |
| [`docs/native/Parity.md`](native/Parity.md) | [JSON](native/Parity.agent.json) |
| [`docs/native/ParityPaths.md`](native/ParityPaths.md) | [JSON](native/ParityPaths.agent.json) |
| [`docs/native/PlatformInfo.md`](native/PlatformInfo.md) | [JSON](native/PlatformInfo.agent.json) |
| [`docs/native/Popover.md`](native/Popover.md) | [JSON](native/Popover.agent.json) |
| [`docs/native/PresentationModels.md`](native/PresentationModels.md) | [JSON](native/PresentationModels.agent.json) |
| [`docs/native/ProgressBar.md`](native/ProgressBar.md) | [JSON](native/ProgressBar.agent.json) |
| [`docs/native/RenderMonitor.md`](native/RenderMonitor.md) | [JSON](native/RenderMonitor.agent.json) |
| [`docs/native/Sheet.md`](native/Sheet.md) | [JSON](native/Sheet.agent.json) |
| [`docs/native/SvgManager.md`](native/SvgManager.md) | [JSON](native/SvgManager.agent.json) |
| [`docs/native/TableModel.md`](native/TableModel.md) | [JSON](native/TableModel.agent.json) |
| [`docs/native/TableTransfer.md`](native/TableTransfer.md) | [JSON](native/TableTransfer.agent.json) |
| [`docs/native/TableView.md`](native/TableView.md) | [JSON](native/TableView.agent.json) |
| [`docs/native/Tables.md`](native/Tables.md) | [JSON](native/Tables.agent.json) |
| [`docs/native/TextDocuments.md`](native/TextDocuments.md) | [JSON](native/TextDocuments.agent.json) |
| [`docs/native/Toolbars.md`](native/Toolbars.md) | [JSON](native/Toolbars.agent.json) |
| [`docs/native/Tooltip.md`](native/Tooltip.md) | [JSON](native/Tooltip.agent.json) |
| [`docs/native/Verification.md`](native/Verification.md) | [JSON](native/Verification.agent.json) |
| [`docs/native/ViewModelRegistry.md`](native/ViewModelRegistry.md) | [JSON](native/ViewModelRegistry.agent.json) |
| [`docs/native/WindowsValidation.md`](native/WindowsValidation.md) | [JSON](native/WindowsValidation.agent.json) |
| [`docs/overview.md`](overview.md) | [JSON](overview.agent.json) |
| [`docs/performance-baseline-p0.md`](performance-baseline-p0.md) | [JSON](performance-baseline-p0.agent.json) |
| [`docs/policies/debug.md`](policies/debug.md) | [JSON](policies/debug.agent.json) |
| [`docs/policies/event.md`](policies/event.md) | [JSON](policies/event.agent.json) |
| [`docs/policies/routing.md`](policies/routing.md) | [JSON](policies/routing.agent.json) |
| [`docs/quality-automation-p4.md`](quality-automation-p4.md) | [JSON](quality-automation-p4.agent.json) |
| [`docs/theme.md`](theme.md) | [JSON](theme.agent.json) |
| [`example/AndroidHello/README.md`](../example/AndroidHello/README.md) | [JSON](mirrors/example/AndroidHello/README.agent.json) |
| [`example/EventListener/README.md`](../example/EventListener/README.md) | [JSON](mirrors/example/EventListener/README.agent.json) |
| [`example/NativeAppSurfaces/AGENT.md`](../example/NativeAppSurfaces/AGENT.md) | [JSON](mirrors/example/NativeAppSurfaces/AGENT.agent.json) |
| [`example/NativeAppSurfaces/README.md`](../example/NativeAppSurfaces/README.md) | [JSON](mirrors/example/NativeAppSurfaces/README.agent.json) |
| [`example/NativeAppSurfaces/docs/Usage.md`](../example/NativeAppSurfaces/docs/Usage.md) | [JSON](mirrors/example/NativeAppSurfaces/docs/Usage.agent.json) |
| [`example/NativeCodeEditorGallery/AGENT.md`](../example/NativeCodeEditorGallery/AGENT.md) | [JSON](mirrors/example/NativeCodeEditorGallery/AGENT.agent.json) |
| [`example/NativeCodeEditorGallery/README.md`](../example/NativeCodeEditorGallery/README.md) | [JSON](mirrors/example/NativeCodeEditorGallery/README.agent.json) |
| [`example/NativeCodeEditorGallery/docs/Usage.md`](../example/NativeCodeEditorGallery/docs/Usage.md) | [JSON](mirrors/example/NativeCodeEditorGallery/docs/Usage.agent.json) |
| [`example/NativeCreationWorkbench/AGENT.md`](../example/NativeCreationWorkbench/AGENT.md) | [JSON](mirrors/example/NativeCreationWorkbench/AGENT.agent.json) |
| [`example/NativeCreationWorkbench/README.md`](../example/NativeCreationWorkbench/README.md) | [JSON](mirrors/example/NativeCreationWorkbench/README.agent.json) |
| [`example/NativeCreationWorkbench/docs/Behavior.md`](../example/NativeCreationWorkbench/docs/Behavior.md) | [JSON](mirrors/example/NativeCreationWorkbench/docs/Behavior.agent.json) |
| [`example/NativeCreationWorkbench/example/README.md`](../example/NativeCreationWorkbench/example/README.md) | [JSON](mirrors/example/NativeCreationWorkbench/example/README.agent.json) |
| [`example/NativeMaterialGallery/AGENT.md`](../example/NativeMaterialGallery/AGENT.md) | [JSON](mirrors/example/NativeMaterialGallery/AGENT.agent.json) |
| [`example/NativeMaterialGallery/README.md`](../example/NativeMaterialGallery/README.md) | [JSON](mirrors/example/NativeMaterialGallery/README.agent.json) |
| [`example/NativeMaterialGallery/docs/Usage.md`](../example/NativeMaterialGallery/docs/Usage.md) | [JSON](mirrors/example/NativeMaterialGallery/docs/Usage.agent.json) |
| [`example/NativeMediaGallery/AGENT.md`](../example/NativeMediaGallery/AGENT.md) | [JSON](mirrors/example/NativeMediaGallery/AGENT.agent.json) |
| [`example/NativeMediaGallery/README.md`](../example/NativeMediaGallery/README.md) | [JSON](mirrors/example/NativeMediaGallery/README.agent.json) |
| [`example/NativeMediaGallery/docs/Usage.md`](../example/NativeMediaGallery/docs/Usage.md) | [JSON](mirrors/example/NativeMediaGallery/docs/Usage.agent.json) |
| [`example/NativeNavigationGallery/AGENT.md`](../example/NativeNavigationGallery/AGENT.md) | [JSON](mirrors/example/NativeNavigationGallery/AGENT.agent.json) |
| [`example/NativeNavigationGallery/README.md`](../example/NativeNavigationGallery/README.md) | [JSON](mirrors/example/NativeNavigationGallery/README.agent.json) |
| [`example/NativeNavigationGallery/docs/Usage.md`](../example/NativeNavigationGallery/docs/Usage.md) | [JSON](mirrors/example/NativeNavigationGallery/docs/Usage.agent.json) |
| [`example/NativePageRouterGallery/AGENT.md`](../example/NativePageRouterGallery/AGENT.md) | [JSON](mirrors/example/NativePageRouterGallery/AGENT.agent.json) |
| [`example/NativePageRouterGallery/README.md`](../example/NativePageRouterGallery/README.md) | [JSON](mirrors/example/NativePageRouterGallery/README.agent.json) |
| [`example/NativePageRouterGallery/docs/Usage.md`](../example/NativePageRouterGallery/docs/Usage.md) | [JSON](mirrors/example/NativePageRouterGallery/docs/Usage.agent.json) |
| [`example/NativeTextDocumentGallery/AGENT.md`](../example/NativeTextDocumentGallery/AGENT.md) | [JSON](mirrors/example/NativeTextDocumentGallery/AGENT.agent.json) |
| [`example/NativeTextDocumentGallery/README.md`](../example/NativeTextDocumentGallery/README.md) | [JSON](mirrors/example/NativeTextDocumentGallery/README.agent.json) |
| [`example/NativeTextDocumentGallery/docs/Usage.md`](../example/NativeTextDocumentGallery/docs/Usage.md) | [JSON](mirrors/example/NativeTextDocumentGallery/docs/Usage.agent.json) |
| [`example/NativeTooltipGallery/AGENT.md`](../example/NativeTooltipGallery/AGENT.md) | [JSON](mirrors/example/NativeTooltipGallery/AGENT.agent.json) |
| [`example/NativeTooltipGallery/README.md`](../example/NativeTooltipGallery/README.md) | [JSON](mirrors/example/NativeTooltipGallery/README.agent.json) |
| [`example/NativeTooltipGallery/docs/Usage.md`](../example/NativeTooltipGallery/docs/Usage.md) | [JSON](mirrors/example/NativeTooltipGallery/docs/Usage.agent.json) |
| [`example/README.md`](../example/README.md) | [JSON](mirrors/example/README.agent.json) |
| [`example/TypingPractice/README.md`](../example/TypingPractice/README.md) | [JSON](mirrors/example/TypingPractice/README.agent.json) |
| [`example/VisualCatalog/README.md`](../example/VisualCatalog/README.md) | [JSON](mirrors/example/VisualCatalog/README.agent.json) |
| [`example/VisualCatalog/assets/cards/README.md`](../example/VisualCatalog/assets/cards/README.md) | [JSON](mirrors/example/VisualCatalog/assets/cards/README.agent.json) |
| [`example/iOSHello/README.md`](../example/iOSHello/README.md) | [JSON](mirrors/example/iOSHello/README.agent.json) |
| [`example/mvvm/README.md`](../example/mvvm/README.md) | [JSON](mirrors/example/mvvm/README.agent.json) |
| [`resources/images/card/README.md`](../resources/images/card/README.md) | [JSON](mirrors/resources/images/card/README.agent.json) |
| [`resources/images/colorpicker/README.md`](../resources/images/colorpicker/README.md) | [JSON](mirrors/resources/images/colorpicker/README.agent.json) |
| [`src/rust-cli/README.md`](../src/rust-cli/README.md) | [JSON](mirrors/src/rust-cli/README.agent.json) |
<!-- workspace-docs:document-map:end -->

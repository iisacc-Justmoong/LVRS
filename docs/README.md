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
10. `rust-cli/README.md`

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

- `Chnagelog.md` (repo root): commit-trace change summary by date.
- `rust-cli/README.md` (repo root): `lvrs` CLI entrypoints (`install`, `bootstrap`, `doctor`, `platform`).

## Backend Singletons

- `docs/backend/AppBootstrap.md`
- `docs/backend/Backend.md`
- `docs/backend/RuntimeEvents.md`
- `docs/backend/RenderQuality.md`
- `docs/backend/RenderMonitor.md`
- `docs/backend/RouteMatcher.md`
- `docs/backend/PageMonitor.md`
- `docs/backend/ViewStateTracker.md`
- `docs/backend/ViewModels.md`
- `docs/backend/Platform.md`
- `docs/backend/Debug.md`
- `docs/backend/DebugOutput.md`

## App Layer

- `docs/components/app/ApplicationWindow.md`
- `docs/components/app/AppShell.md`

## Control Components

- `docs/components/control/AbstractButton.md`
- `docs/components/control/LabelButton.md`
- `docs/components/control/IconButton.md`
- `docs/components/control/LabelSegmentedControl.md`
- `docs/components/control/IconSegmentedControl.md`
- `docs/components/control/LabelMenuButton.md`
- `docs/components/control/IconMenuButton.md`
- `docs/components/control/ComboBox.md`
- `docs/components/control/Stepper.md`
- `docs/components/control/Label.md`
- `docs/components/control/CheckBox.md`
- `docs/components/control/RadioButton.md`
- `docs/components/control/ToggleSwitch.md`
- `docs/components/control/InputField.md`
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

- `docs/components/navigation/Navigator.md`
- `docs/components/navigation/PageRouter.md`
- `docs/components/navigation/Link.md`
- `docs/components/navigation/Hierarchy.md`
- `docs/components/navigation/HierarchyList.md`
- `docs/components/navigation/HierarchyItem.md`
- `docs/components/navigation/HierarchyToolbar.md`
- `docs/components/navigation/ContextMenu.md`
- `docs/components/navigation/MenuItem.md`
- `docs/components/navigation/MenuDivider.md`

## Surface Components

- `docs/components/surfaces/AppCard.md`
- `docs/components/surfaces/Alert.md`

## Policies

- `docs/policies/event.md`
- `docs/policies/routing.md`
- `docs/policies/debug.md`

## Fast Lookup: Typical Questions

- How to wire global input event handling?  
  Start with `docs/architecture/event-pipeline.md`, then `docs/components/control/EventListener.md`.

- How to route pages with stack semantics and route params?  
  Start with `docs/components/navigation/PageRouter.md`, then `docs/policies/routing.md`.

- How to enforce MVVM ownership for writes?  
  Start with `docs/mvvm.md`, then `docs/backend/ViewModels.md`.

- How to inspect runtime behavior and debug output?  
  Start with `docs/backend/RuntimeEvents.md`, `docs/backend/Debug.md`, and `docs/backend/DebugOutput.md`.

- How to run install/bootstrap with current workflow?  
  Start with `docs/build.md`, then `rust-cli/README.md`.

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

# AppShell

Location: `qml/AppShell.qml`

`AppShell` is a compatibility wrapper that directly inherits `ApplicationWindow` without adding extra API.

## Purpose

- Preserve legacy import/usage path.
- Provide migration-safe alias while new code adopts `LV.ApplicationWindow` directly.

## API Contract

- No additional properties, methods, or signals are defined.
- All behavior is inherited from `ApplicationWindow`.

## Usage

```qml
import LVRS 1.0 as LV

LV.AppShell {
    visible: true
    width: 1100
    height: 720
    title: "LVRS"
    navItems: ["Overview", "Runs"]
}
```

## Recommendation

Use `LV.AppBootstrapWindow` for standard consumer app roots, `LV.ApplicationWindow` for lower-level shell control, and keep `AppShell` only for compatibility.

## Migration Note

When migrating from `AppShell` to `ApplicationWindow`, property names remain the same because `AppShell` is a direct wrapper.
Recommended migration path is replacing type usage only.

## Compatibility Scope

`AppShell` is intentionally thin.
Only compatibility is guaranteed; new feature switches are documented first in `ApplicationWindow`.

## FAQ

Q. Does `AppShell` expose a different navigation lifecycle?  
A. No. Navigation behavior is exactly the same as `ApplicationWindow`.

Q. Should new modules import only `AppShell` for stability?  
A. No. Use `AppBootstrapWindow` for the standard app-root bootstrap path or `ApplicationWindow` for direct shell control.

## Deprecation Strategy

If project policy deprecates `AppShell`, keep a compatibility window with:

1. codemod-assisted type rename,
2. release-note mapping table,
3. temporary lint rule warning on new `AppShell` usage.

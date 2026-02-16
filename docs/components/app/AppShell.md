# AppShell

Location: `qml/AppShell.qml`

Compatibility wrapper over `ApplicationWindow`.
`AppShell` adds no extra API; it inherits `ApplicationWindow` as-is.
New code should use `LV.ApplicationWindow` directly.

## API Notes

- All properties/signals come from `ApplicationWindow`.
- No additional aliases or behavior are defined in `AppShell.qml`.

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

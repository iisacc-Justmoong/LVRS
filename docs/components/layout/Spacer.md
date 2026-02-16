# Spacer

Location: `qml/components/layout/Spacer.qml`

`Spacer` is an axis-aware flexible filler for `HStack`, `VStack`, and `ZStack` contexts.

## Purpose

- Expand along the active stack axis.
- Reserve minimum axis length when requested.
- Fill entire parent in overlay (`ZStack`) contexts.

## API

- `minLength`
- `stackAxis` (`"horizontal"` or `"vertical"`, usually auto-assigned)

## Usage

```qml
import LVRS 1.0 as LV

LV.HStack {
    LV.Label { text: "Left" }
    LV.Spacer {}
    LV.Label { text: "Right" }
}
```

## How It Works

- Resolves axis from parent stack flags (`__isHStack`, `__isVStack`) or explicit `stackAxis`.
- In horizontal mode, fills width and applies `minimumWidth = minLength`.
- In vertical mode, fills height and applies `minimumHeight = minLength`.
- In `ZStack`, uses `anchors.fill = parent`.

## Practical Notes

- In stack contexts, `Spacer` affects only the active axis and keeps the cross-axis unchanged.
- In `ZStack`, `Spacer` behaves as full-fill overlay helper by design.
- Prefer explicit `stackAxis` only when composing custom containers that mimic stack behavior.

## FAQ

Q. Why does `Spacer` not expand in some custom containers?  
A. Axis inference depends on stack/layout context. For custom wrappers, set `stackAxis` explicitly.

Q. Can `Spacer` be used outside layouts?  
A. Yes, but expansion behavior is undefined unless parent exposes compatible layout semantics.

## Validation Checklist

- verify axis inference (`horizontal`/`vertical`) on runtime parent type,
- verify `minLength` is reflected in layout minimum size,
- verify no unexpected anchor conflicts in overlay contexts.

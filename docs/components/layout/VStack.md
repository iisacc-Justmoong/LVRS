# VStack

Location: `qml/components/layout/VStack.qml`

`VStack` is a SwiftUI-style vertical stack implemented with `ColumnLayout`.

## Purpose

- Provide predictable vertical composition with alignment-name shorthand.
- Propagate axis metadata to stack-aware children.

## API

- `spacing` (`-1` means default spacing)
- `defaultSpacing`
- `alignment`
- `alignmentName` (`leading | center | trailing`)
- default `content` slot

## Usage

```qml
import LVRS 1.0 as LV

LV.VStack {
    alignmentName: "leading"
    spacing: 8
    LV.Label { text: "Title"; style: title2 }
    LV.Label { text: "Description"; style: description }
}
```

## How It Works

- Name tokens map to horizontal layout alignment flags.
- Managed child alignment updates are applied only when child alignment is auto-managed.
- Children exposing `stackAxis` receive `"vertical"` for `Spacer` cooperation.

## Advanced Example: Trailing Alignment with Spacer

```qml
import LVRS 1.0 as LV

LV.VStack {
    alignmentName: "trailing"
    LV.Label { text: "Latency"; style: body }
    LV.Spacer { minLength: 16 }
    LV.Label { text: "12 ms"; style: header2 }
}
```

## Practical Tip

Use `spacing = -1` with `defaultSpacing` when wanting a design-token-driven vertical rhythm.

## FAQ

Q. Does `alignmentName` override `alignment`?  
A. Yes. Named alignment tokens take precedence when present.

Q. Why does `Spacer` not push as expected?  
A. Ensure child is inside stack-managed layout tree and not in a separately anchored subtree.

## Validation Checklist

- alignment token resolves to expected horizontal placement,
- spacing fallback works when `spacing == -1`,
- stack axis propagation enables spacer expansion.

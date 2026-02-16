# HStack

Location: `qml/components/layout/HStack.qml`

`HStack` is a SwiftUI-style horizontal stack implemented with `RowLayout`.

## Purpose

- Provide simple axis layout with optional name-based alignment semantics.
- Auto-annotate managed children for spacer behavior (`stackAxis`).

## API

- `spacing` (`-1` means default spacing)
- `defaultSpacing`
- `alignment`
- `alignmentName` (`top | center | bottom`)
- default `content` slot

## Usage

```qml
import LVRS 1.0 as LV

LV.HStack {
    alignmentName: "bottom"
    spacing: 12
    LV.LabelButton { text: "Cancel" }
    LV.LabelButton { text: "Save"; tone: LV.AbstractButton.Primary }
}
```

## How It Works

- Name-based alignment has priority over raw `alignment` flags.
- Alignment updates apply only to managed children with untouched layout alignment.
- Children exposing `stackAxis` receive `"horizontal"` to cooperate with `Spacer`.

## Advanced Example: Mixed Managed/Custom Alignment

```qml
import QtQuick
import LVRS 1.0 as LV

LV.HStack {
    alignmentName: "center"
    Rectangle { width: 32; height: 32 }
    Rectangle {
        width: 32; height: 48
        Layout.alignment: Qt.AlignTop
    }
}
```

In this case, explicitly assigned child alignment is preserved while auto-managed children follow stack alignment.

## FAQ

Q. Why does one child ignore `alignmentName`?  
A. Child may have explicit `Layout.alignment` set and no longer be auto-managed by stack.

Q. Is negative spacing allowed?  
A. Contract allows any integer, but negative spacing causes overlap and should be used deliberately.

## Validation Checklist

- alignment token resolves to expected vertical placement,
- spacing contract is consistent across responsive breakpoints,
- mixed explicit/auto child alignment behaves as intended.

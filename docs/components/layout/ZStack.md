# ZStack

Location: `qml/components/layout/ZStack.qml`

`ZStack` is a SwiftUI-style overlay stack that aligns children in two-dimensional anchor space.

## Purpose

- Overlay multiple children with a single alignment contract.
- Avoid accidental overrides on children already managed by external anchors.

## API

- `alignment`
- `alignmentName`
- default `content` slot

Supported `alignmentName` tokens:

- `topLeading`, `top`, `topTrailing`
- `leading`, `center`, `trailing`
- `bottomLeading`, `bottom`, `bottomTrailing`

## Usage

```qml
import LVRS 1.0 as LV

LV.ZStack {
    alignmentName: "topTrailing"
    Rectangle { anchors.fill: parent }
    Rectangle { width: 14; height: 14; radius: 7 }
}
```

## How It Works

- For unmanaged children, existing anchors are cleared and rebuilt from resolved alignment.
- Children with external anchors are skipped unless already tagged as internally managed.
- Child updates run on `childrenChanged` to keep alignment consistent after dynamic insertion.

## Advanced Example: Explicit Anchor Preservation

Children with explicit external anchors are intentionally skipped by auto-alignment logic unless they were already internally managed.
This prevents accidental anchor resets in composite overlays.

## Troubleshooting

If a child appears misaligned:

1. check whether it has manual anchors,
2. check `alignmentName` token correctness,
3. verify child was not re-parented outside stack content layer.

## FAQ

Q. Why are my manual anchors reset?  
A. Auto-managed children in `ZStack` have anchors rewritten by alignment contract.

Q. How to keep manual anchor ownership?  
A. Keep explicit anchors on child before stack auto-management applies, or use dedicated wrapper item.

## Validation Checklist

- child alignment follows `alignmentName` token expectations,
- externally anchored children remain unaffected when intended,
- dynamic child insertion preserves expected stacking order.

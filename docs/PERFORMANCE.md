# Hierarchy projection

`HierarchyVisibility::project` memoizes ancestor depth and expansion state within
one projection. Each ancestor is resolved once, replacing repeated walks of a
deep tree with expected O(nodes + parents) hash lookups. The cache is local to
the call, so changing parent relationships or expanded IDs takes effect on the
next projection without stale shared state.

Input ordering, duplicate-ID depth lookup behavior, missing-parent depth,
expanded-state behavior and row order are preserved. Iterative traversal handles
deep trees without using the call stack. Cyclic branches are omitted instead of
looping indefinitely. Other hierarchy editing/validation APIs are unchanged.

The hierarchy tests compare output against an independent traversal for
shuffled forests, missing parents, duplicate IDs and expansion combinations,
and cover deep chains and malformed cycles. Palette and rendering assets are
not involved in this optimization.

`TableModel::visibleCells` computes row/column prefix geometry once and tracks
only merge spans that can cover the current row. It no longer rescans every
earlier cell or adds all earlier row/column dimensions for each visible cell.
Ordinary unmerged tables take linear layout work in the number of cells. Merged
tables additionally inspect active spanning anchors; pathological overlapping
spans can still cost more. Explicit merge markers, first-anchor behavior, row
order and geometry rounding retain their existing semantics. All scratch state
is local, including when rows originate from a QAbstractItemModel.

Bulk text retrieval and saving reuse one provider-owned read handle for
file-backed lines. They avoid opening the same source once per line. Saving
closes that handle before the provider's atomic replacement on Windows. A
missing or shortened source now fails the save rather than silently publishing
empty or partial lines. A successful save replaces internal line storage without
resetting the list model: row identity, text roles and persistent indexes remain
unchanged, and the existing storage/dirty/save notifications report the save.

The SVG geometry test embeds the exact raw source SVG files as test-only
resources. It still checks every original asset, original XML dimensions and
all four raster sizes. Repeated external-storage opens and per-pixel rectangle
unions are eliminated; the installed artwork and production resources are not
rewritten by this test fixture. Source changes rebuild the fixture through RCC
dependencies.

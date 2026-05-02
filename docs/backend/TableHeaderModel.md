# TableHeaderModel

Location: `backend/model/tableheadermodel.h`, `backend/model/tableheadermodel.cpp`

`TableHeaderModel` owns header row source resolution and geometry for `TableHeader.qml`.

## Purpose

- Resolve `cellItems` and legacy `columns` into typed header descriptors.
- Normalize primitive and object-based header entries in C++.
- Infer column value types from string, integer, float, and boolean header values.
- Calculate column padding, width, and x-offsets for rendering.

## API

Input:

- `cellItems`
- `columns`
- `tableWidth`
- `rowHeight`
- `cellHorizontalPadding`
- `columnWidths`
- `fallbackCellWidth`
- `minColumnWidth`

Readonly:

- `descriptors`
- `columnCount`
- `revision`

Methods:

- `resolvedColumnSource()`
- `columnAt(index)`
- `normalizeColumnType(value)`
- `inferredColumnType(value)`
- `columnType(index)`
- `columnText(index)`
- `columnPadding(index)`
- `numericWidth(value, fallbackValue)`
- `autoColumnWidth()`
- `columnWidth(index)`
- `columnX(index)`
- `descriptorAt(index)`

## Descriptor Contract

Each descriptor contains:

- `index`
- `sourceData`
- `text`
- `valueType`
- `x`
- `width`
- `height`
- `padding`

## How It Works

- `cellItems` is preferred over `columns`.
- Object entries read label text from `label`, `text`, `title`, then `value`.
- Object entries read type metadata from `type`, `valueType`, `cellType`, then `dataType`.
- Primitive header entries infer type directly: boolean to `bool`, integral number to `int`, non-integral number to `float`, otherwise `string`.
- Column width uses explicit `columnWidths[index]`, then `fallbackCellWidth`, then equal auto width from `tableWidth / columnCount`.
- Widths are clamped to `minColumnWidth`.
- `descriptorAt(index)` returns a fallback `"Column"` descriptor for non-negative indices outside the current descriptor list so `TableHeader.qml` can still render its one-column empty-state fallback.

## QML Boundary

`TableHeader.qml` renders `descriptors` and forwards compatibility helper calls to `TableHeaderModel`. Header text resolution, type inference, padding, width, and x-offset calculation are model responsibilities.

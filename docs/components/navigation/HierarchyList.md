# HierarchyList

Location: `qml/components/navigation/HierarchyList.qml`

`HierarchyList` renders nested tree data as hierarchy rows and computes indentation from parent-child depth.

## Purpose

- Accept hierarchical model data and flatten it into visible rows.
- Support depth-based indentation for child, grandchild, and deeper nodes.
- Provide activation, expansion, and keyboard navigation for tree rows.

## API

Model input:

- `model` / `treeModel`
- `childrenRole` (default: `children`)
- `labelRole` (default: `label`)
- `iconNameRole` (default: `iconName`)

Common roles:

- `itemIdRole`, `itemKeyRole`
- `iconSourceRole`, `iconGlyphRole`
- `enabledRole`, `expandedRole`, `selectedRole`, `showChevronRole`

Layout tokens:

- `generatedIndentStep` (default: `13`)
- `generatedRowHeight` (default: `28`)
- `generatedItemWidth` (default: `200`)
- `generatedIconSize` (default: `16`)
- `generatedChevronSize` (default: `16`)

State:

- `activeItem`
- `activeItemId`
- `activeItemKey`
- `itemCount`
- `visibleItemCount`

Methods:

- `activateById(itemId)`
- `activateByKey(itemKey)`
- `expandAll()`
- `collapseAll(keepRootExpanded)`

Signals:

- `activeChanged(item, itemId, index)`
- `expansionChanged(item, expanded, index)`
- `ensureVisibleRequested(y, height)`

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyList {
    model: [
        {
            label: "Root",
            iconName: "projectStructure",
            children: [
                {
                    label: "Child",
                    iconName: "viewMoreSymbolicDefault",
                    children: [
                        { label: "Grandchild", iconName: "viewMoreSymbolicBorderless" }
                    ]
                }
            ]
        }
    ]
}
```

## Hierarchy Rule

- `indentLevel` is derived from depth of nested `children`.
- First level uses base left padding `8`.
- Each depth step adds `13` (`8`, `21`, `34`, `47`, ...).

## Failure Pattern

If role names are changed in model but matching `*Role` properties are not updated, labels/icons/children may render as empty.
Always keep model role names and `HierarchyList` role bindings aligned.

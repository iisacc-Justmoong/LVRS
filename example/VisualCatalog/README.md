# Visual Catalog Example

This example is the LVRS visual documentation browser driven by `Main.qml`.
The left sidebar is a `Hierarchy` index of the public QML surface, and the right panel shows the selected type as a live reference page with preview, source path, related types, and usage snippet.

## Run

From repository root:

```bash
./build.sh
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

The checked-in launcher resolves the build-tree executable from `build/example/VisualCatalog/bin/LVRSExampleVisualCatalog` during in-repository development and a sibling snapshot runtime after install.

## Structure

- `qml/CatalogRegistry.qml` holds the catalog tree, component metadata, and sidebar model.
- `qml/Main.qml` renders the application shell, hierarchy sidebar, and detail panels.

## Intended Use

- Browse the library by domain instead of memorizing file paths.
- Use section nodes for category overviews.
- Use component nodes for concrete visual documentation and usage examples.

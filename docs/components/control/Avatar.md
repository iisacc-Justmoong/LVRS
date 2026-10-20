# Avatar

## Purpose

`LV.Avatar` displays an image inside a circular user-profile surface. The image keeps its aspect ratio, fills the circle, and crops equally from opposite edges. Figma sizes are Large (64), Medium (40), and Small (24).

## API

Import `LVRS 1.0 as LV`.

| Property | Default | Description |
| --- | --- | --- |
| `source: url` | empty | Image URL, using the same loaders and codecs as Qt Quick `Image`. |
| `size: int` | `LV.Avatar.Large` | `LV.Avatar.Small`, `Medium`, or `Large`; implicit diameter 24, 40, or 64. |
| `backgroundColor: color` | `LV.Theme.surfaceRaised` | Surface behind transparent images, and while the source is empty, loading, or invalid. Set `"transparent"` to show only the image. |
| `asynchronous: bool` | `true` | Decode local images in the background. Network sources always load asynchronously. |
| `cache: bool` | `true` | Use the Qt Quick image cache. |
| `status: int` (read-only) | `Image.Null` | Current source state: `Image.Null`, `Loading`, `Ready`, or `Error`. |
| `progress: real` (read-only) | `0` | Current source-loading progress, from 0 to 1. |

`width` and `height` may override the implicit size. The circle uses the smaller dimension and remains centered horizontally and vertically, including inside a rectangular layout slot. Set `Accessible.name` to identify the user when the avatar conveys identity.

## Usage

```qml
import QtQuick
import LVRS 1.0 as LV

LV.Avatar {
    source: Qt.resolvedUrl("images/profile.webp")
    size: LV.Avatar.Medium
    Accessible.name: "Alex Kim"
}
```

```qml
Row {
    spacing: LV.Theme.gap12
    LV.Avatar { source: user.photoUrl; size: LV.Avatar.Large }
    LV.Avatar { source: "qrc:/images/profile.svg"; size: LV.Avatar.Medium }
    LV.Avatar { source: "image://profiles/current"; size: LV.Avatar.Small }
}
```

Relative asset URLs, `file:`, `qrc:`, HTTP(S), `data:`, and registered `image://` providers are accepted. Raster and vector formats are decoded by the Qt image plugins shipped with the application, including PNG, JPEG, WebP, BMP, GIF, and SVG when their plugins are present. Animated formats are displayed as a still image, following `Image`; this component does not play animations. Native `QImage`/`QPixmap` data can be supplied through a registered Qt Quick image provider.

For an asset relative to the consuming QML document, use `Qt.resolvedUrl(...)` at the call site, as in the first example. This keeps the image tied to that document's location when its URL is forwarded to internal loaders, following [Qt 6 URL resolution](https://doc.qt.io/qt-6.8/qml-url.html).

## How It Works

The source uses `Image.PreserveAspectCrop` with both alignment axes set to center. A circular alpha mask removes all pixels outside the circle instead of relying on rectangular `clip`. GPU rendering uses Qt Quick `MultiEffect`; the software renderer paints the same centered crop into a circular Canvas clip. Both paths preserve source transparency.

Changing the URL replaces the image. Empty or failed sources clear the previous image and expose `backgroundColor`; clients can inspect `status` for their own fallback behavior. The component does not introduce a hover state or intercept input.

## Validation

`LVRSTests_avatar` checks all three sizes, rectangular layout slots, horizontal and vertical crop centering, transparent corners, source replacement, errors, file formats, data URLs, resources, image providers, and a local HTTP response. Run the pixel checks on both scene graphs:

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON
cmake --build build --target LVRSTests_avatar
QT_QUICK_BACKEND=software ctest --test-dir build --output-on-failure -R '^LVRSTests_avatar$'
```

On macOS, exercise the GPU path with the native Cocoa window system (the offscreen platform does not support Metal windows):

```sh
QT_QPA_PLATFORM=cocoa QT_QUICK_BACKEND=rhi QSG_RHI_BACKEND=metal \
    QML_IMPORT_PATH="$PWD/build" DYLD_LIBRARY_PATH="$PWD/build" \
    LVRS_AVATAR_REQUIRE_GPU=1 build/tests/LVRSTests_avatar
```

The standalone project under `tests/installed-consumer` checks the installed package independently of the build-tree QML module. Its `installed_lvrs_avatar` test repeats the image and pixel contracts against the installed library, while `installed_lvrs_qml_consumer` checks the three sizes and (when `LVRS_CONSUMER_SOURCE_DIR` is set) compares every embedded QML source with the checkout.

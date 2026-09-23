# Figma Iconset SVG import

The 2026-09-19 snapshot of the LVRS [Iconset page](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=198-2)
contains 2,863 icons: 2,861 components and two standalone vectors. Every icon is
stored directly in `resources/iconset/`; category directories are not retained.

The SVG artwork comes from Figma's native SVG export, without path reconstruction,
recoloring, or content deduplication. The initial 614 Plugin API exports were
byte-identical to the corresponding native exports. On 2026-09-20 all 2,863 iconset
SVGs and 13 supplemental SVGs were normalized at the root: `width="100%"`,
`height="100%"`, `preserveAspectRatio="xMidYMid meet"`, and a square `viewBox`
centered on the visible artwork. Path data and all child elements remain unchanged.
The original viewport size is retained unless visible overflow requires a larger
square. Bounds are sampled with Qt SVG at eight pixels per SVG unit, including
stroke and embedded-image alpha. This is geometric visible-bounds centering,
not subjective optical weighting of asymmetric shapes.

Figma's native exporter writes category directories and can overwrite names that
differ only in case on macOS. The `nodesTest` / `nodestest` and `weChat` / `wechat`
pairs were separately exported to preserve both versions. The existing canonical
lookup names `nodestest.svg` and `weChat.svg` remain available; the other variants
use `nodesTest-1.svg` and `wechat-1.svg`. Other filename collisions created by
flattening receive the next available numeric suffix, without overwriting a
literal suffixed name or dropping identical artwork.

[figma-iconset-manifest.json](figma-iconset-manifest.json) records every final
filename, native export filename, original export dimensions, and current SHA-256.
`preResponsiveSha256` preserves the hash before root normalization. The two normalized
exports also record their original native SHA-256. The 44 source
groups were reconciled against the complete 2,863-file result. Grouping is only
used to check completeness and is not part of the resource layout.

The existing CMake resource discovery includes these files automatically.
`LVRSTests_svg_manager::figma_iconset_resources_are_complete_and_renderable`
checks the full inventory for unique flat filenames, packaged resource presence,
SHA-256 identity, valid SVG parsing, and nonempty Qt rasterization for all icons.
`all_svg_icons_scale_and_center` checks every resource SVG's responsive root and
visible bounds at 18, 36, 72, and 144 pixels, allowing raster antialiasing tolerance.

To normalize newly imported assets, run from the repository root:

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON
cmake --build build --target LVRSNormalizeSvgIcons
./build/tests/LVRSNormalizeSvgIcons .
```

Review the resulting SVG and manifest changes, then run the regression tests below.
The normalizer is an explicit maintenance target, never part of an ordinary build.
It also refreshes the SVG hashes in `tests/fixtures/figma-contract.json`. Sampling
origins are snapped to a 1/8-unit grid so repeating normalization is idempotent.
The two SVGs containing PNGs remain image-backed: resizing their SVG viewport does
not create additional raster detail.

The native `smartSelect.svg` and `translateObject.svg` contain an embedded PNG
referenced by a full-bounds SVG pattern. Qt 6.8.3 reports `<use> element ... in
wrong context` and renders those references empty. These two exports replace the
pattern indirection with a direct `<image>` covering the same 18 × 18 view box.
The embedded PNG data is byte-identical, including its alpha channel, and no
artwork is regenerated. Both normalized files remain standalone SVGs.

```sh
cmake --build build --target LVRSTests_svg_manager LVRSTests_figma_parity --parallel 1
ctest --test-dir build --output-on-failure -R 'LVRSTests_(svg_manager|figma_parity)$'
```

This snapshot covers the Iconset page. The 14 supplemental control SVGs already
deleted before this import (Stepper, checkbox, dropdown state variants, and the
input search glyph) are absent from that page and remain deleted.

Final verification on macOS with Qt 6.8.3: the LVRS build succeeded and both
`LVRSTests_svg_manager` and `LVRSTests_figma_parity` passed (2/2), including SVG
loading, resource hashes, and visible raster output for all 2,863 imported files.
The broader `LVRSTests_import_api` run reported 87 passed, 6 failed, and 1 skipped;
all six failing cases require the already-deleted supplemental control resources
listed above. Those deletions were preserved instead of being silently restored.

Responsive-root verification on 2026-09-20: both targeted CTest suites pass,
including all 2,876 SVGs at four raster sizes and the packaged iconset hashes.
The existing native-RHI-only highlight case is skipped in offscreen testing;
warnings for previously deleted supplemental controls remain outside this change.

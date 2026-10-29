<a id="figma-iconset-svg-import"></a>

# Figma Iconset SVG import

The 2026-10-12 [LVRS Iconset](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=1344-9163) snapshot contains 2,846 canonical icons: 2,844 component masters and two standalone vector studies, organized into 44 Figma categories. Resources remain flat in `resources/iconset/`; Figma category information, component keys, node IDs, dimensions, native export hashes, and packaged hashes are recorded in [figma-iconset-manifest.json](figma-iconset-manifest.json).

## 2026-10-29 색상 확인 범위

현재 Figma 원본 2,846개를 모두 확인했고 이 중 2,178개의 자산을 갱신했다. 일반적인
항목은 레이어와 도형을 대조하여 SVG 페인트만 교체하며 기존 형상과 반응형 뷰포트를
보존한다. 도형 경로의 내보내기 표현이 다른 경우에는 양쪽 경로를 래스터화하여 겹침을
대조한다. 경로 대응이 불가능한 9개는 현재 Figma 벡터를 사용하며 기존 정사각형
뷰포트는 유지한다. 기존에 이미지 기반이던 `smartSelect`와 `translateObject`는 마스크
호환성을 위해 Figma 8배 PNG를 SVG에 내장한다.

매니페스트의 `colorReadbackCoverage`는 확인 범위를, 각 `colorReadback`은 변수 ID와
원본 해시 또는 경로 대조 근거를 기록한다. `scripts/reconcile_figma_icon_colors.py`는
읽기 결과를 ID별로 대조하며, 현재 매니페스트와 다른 사용자 변경을 감지하면 중단한다.
지원하지 않는 효과나 외부 참조가 있는 벡터는 별도 처리를 위해 남긴다. 부분 읽기를
이어 처리할 때는 이전과 새로운 읽기 파일을 같은 입력 디렉터리에 유지한다.

This snapshot includes the 177 panel icons added to Figma, the consolidation of 194 duplicate components, and the 113 component name changes. Canonical resource filenames use the current Figma names and are unique under Unicode normalization and case-insensitive comparison. Existing control-specific resources remain separate from the Figma catalog.

The 892 formerly monochrome icons retain their Figma accents. Their selected vector, text, or original image silhouette uses the existing LVRS ThemeColors tokens: blue, cyan, teal, yellow, red, purple, green, palette orange, and palette pink. SVG exports resolve those bindings to their authored colors. The 2026-10-19 palette revision recolors the replaced accent paints and opacity composites; unrelated artwork remains as exported; no renderer tint or replacement glyph is introduced. Twelve Boolean-operation icons additionally bind their visible union fill or stroke to the same token: coloring a consumed child alone does not affect the exported union. Their node geometry, dimensions, and path data are preserved.

## Authored accent revision — 2026-10-19

The ten swatches `1477:30..38`, `1477:40` replace the former accent palette. Canonical variables and seven compatibility palette aliases retain their IDs. Matching SVG paint values follow the new accents, including alpha stops and existing dark composites. Path data, root scaling, dimensions and opacity remain intact. The manifest's `accentPaletteUpdate` records source nodes, exact RGBA, display HEX and the color-update method. Five Alert/Card helper SVGs are also recolored and recorded under `additionalResources`. Each changed vector icon records `paletteGeometrySha256`, computed after removing only paint attributes, and a refreshed packaged `sha256`. Original native export hashes remain historical provenance unless an icon is exported again.

The translation icon uses an embedded raster silhouette; its visible Cyan is verified separately from its unchanged per-pixel alpha. Native export provenance and packaged compatibility hashes remain distinct. Test coverage includes this raster exception as well as the vector palette migration.

## Importing a complete snapshot

Export component masters and the two standalone vectors using the Figma Plugin API with `SVG_STRING`, outlined text, and simplified strokes. The import JSON contains each icon's `id`, `name`, `key`, `type`, `category`, `width`, `height`, complete `svg`, and `nativeSha256`. Icons that received an accent additionally record `accentVariableId`. A previous native export may be reused only after its complete SHA-256 matches the current live export.

```sh
python3 scripts/import_figma_iconset.py \
    --export /absolute/path/native-icons.json \
    --aliases /absolute/path/legacy-aliases.json \
    --captured-on 2026-10-12 \
    --expected-count 2846 \
    --compatibility-dir /absolute/path/compatible-svg
cmake -S . -B build -DLVRS_BUILD_TESTS=ON
cmake --build build --target LVRSNormalizeSvgIcons
./build/tests/LVRSNormalizeSvgIcons .
```

The importer validates completeness, unique node IDs, filename safety, XML, native hashes, and compatibility alias targets before replacing the previously managed SVG snapshot. It preserves the 14 stepper, checkbox, dropdown, and input-search control SVGs. Removing managed files before writing the new snapshot also fixes case-only filename changes on case-insensitive filesystems.

Legacy filenames that were renamed or consolidated are packaged through `resources/figma_icon_aliases.qrc`. Each alias points to the current canonical SVG, so old QML resource URLs receive the current artwork and colors without retaining a second SVG file. Alias mappings are listed separately in the manifest.

## SVG and raster compatibility

All 2,873 physical resource SVGs, including the 14 control variants and 13 image helpers, use `width="100%"`, `height="100%"`, `preserveAspectRatio="xMidYMid meet"`, and a centered square `viewBox`. `scripts/normalize_svg_icons.cpp` samples visible alpha bounds at eight pixels per SVG unit, including stroke overflow and embedded images. It changes only root attributes and refreshes the manifest and Figma parity fixture hashes. Repeating normalization must report zero updated roots.

The native `smartSelect` and `translateObject` exports contain embedded PNG artwork. Qt compatibility exports use direct SVG images rather than an indirect image pattern. The translation icon retains the original image alpha silhouette and the authored cyan accent. Vector path geometry and outlined glyphs remain unchanged. Native and compatibility hashes remain separately recorded.

## Validation and installation

```sh
python3 tests/test_figma_iconset_import.py
cmake --build build --target LVRSTests_svg_manager LVRSTests_figma_parity LVRSTests_import_api --parallel 4
ctest --test-dir build --output-on-failure \
    -R 'LVRSTests_(svg_manager|figma_parity|import_api|figma_iconset_import)$'
lvrs install
```

The import regression checks reject incomplete or corrupted snapshots without replacing existing assets. SVG tests check every canonical icon's packaged hash and raster output, all 892 visible accents, current artwork at every legacy resource alias, retained control variants, and centering at 18, 36, 72, and 144 pixels. The default macOS installer rebuilds the installed macOS, iOS, Android, and WASM framework packages and refreshes the installed source snapshot. Consumer application bundles and already running processes have their own resource copies; SDK installation is verified separately from consumer rebuilds.

# File creation ownership in LVRS

Runtime diagnostic files, text-document persistence and native helper output use provider factories. Rendering and the LVRS component/layout contracts remain here.

The common [ownership contract](../../iiFileProvider/docs/FILE_OWNERSHIP.md) defines stream, staging, control-file and
installed-package rules. Source regression tests remain with this module; the
workspace creation audit also checks these boundaries. Build/deployment and target
runtime validation are separate evidence.

Changed boundaries:


- `scripts/normalize_svg_icons.cpp`
- `src/backend/graphics/svgmanager.cpp`
- `src/backend/io/backend.cpp`
- `src/backend/runtime/renderquality.cpp`
- `src/backend/text/textdocumentmodel.cpp`

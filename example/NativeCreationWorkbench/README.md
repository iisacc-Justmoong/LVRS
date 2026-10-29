# Native Creation Workbench

A Qt-free LVUI/iiGUI authoring probe for a creation workspace: native parameter
controls, C++ canvas and timeline sources, captured pointer input, asynchronous
procedural RGBA generation, keyed result rows, selection and retained frame playback.
It demonstrates SDK view capabilities; it does not migrate Dreamscapes or run an AI model.

Build with CMake in `build/`, using installed native LVRS/LVUI/iiGUI packages.
Run normally for interactive use, or use `--self-test` during the final verification batch.
The self-test verifies GPU frame pixels, view-only pan/seek, result selection,
responsive layout, retained input, display-frame callbacks and backend retirement.

See [the behavior contract](docs/Behavior.md) and [the usage example](example/README.md).

# Native Application Surfaces

AppHeader actions and AppCard body content are compiled from LVUI to ordinary
iiGUI C++ widgets on one native surface. Core Observable inputs update title,
subtitle, compact spacing, card geometry and shape without rebuilding the backend
or the retained editor. iiSDKCore owns the entry point, worker result, frame loop
and shutdown. No Qt or OS widget controls are used.

`install.sh [destination] [CMake options...]` configures, builds and installs under
`build/`; it never launches the app or invokes tests. The registered `--self-test`
is deferred to the final whole-goal runtime batch. Build evidence is separate from
runtime, visual and memory-safety verification. See [usage](docs/Usage.md).

The same scoped backend also drives ListToolbar, ListFooter, a model-backed
HierarchyToolbar and retained manual ToolbarButtons. Immutable shared configurations
update through Core Observable without comparing or interpreting callbacks.

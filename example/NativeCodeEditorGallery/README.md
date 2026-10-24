# Native CodeEditor Gallery

A C++23, Qt-free LVUI application backed by iiGUI's own CodeEditor widgets and an iiSDKCore application entry point. Native font/IME services provide glyph layout and text input; no OS text-edit control is constructed.

Two editors share one document with independent selections and viewport state. Edits rebase the sibling selection; undo/redo restores the originating view's selection while rebasing the other view. Use the in-memory sample by default or pass `--document PATH` to load a UTF-8 file through iiFileProvider Native. Editing never saves automatically. The Save action publishes explicitly and rejects incomplete or failed loads. Read-only, undo/redo, Unicode navigation and Core-owned background results share the retained view scope.

Build/install into this example's `build/` with `./install.sh` and the staged native LVRS package locations. Documentation: [build and behavior](docs/Usage.md). The registered self-test is compiled but deferred to the final whole-goal runtime batch. A built bundle is not proof of a displayed or tested editor.

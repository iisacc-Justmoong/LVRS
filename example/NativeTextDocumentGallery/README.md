# Native Text Document Gallery

A Qt-free C++23 document-model explorer compiled from LVUI and painted by iiGUI.
iiSDKCore owns the application entry, event loop, workers, frames and view lifetime.
The model loads file-backed lines, edits one range and publishes through iiFileProvider Native.

This example demonstrates the nonvisual foundation. The separate native CodeEditor
gallery provides a PlainText viewport; RichText TextEditor and full IME/source parity remain pending.

Configure against installed Core, LVUI, iiGUI, LVRS and iiFileProvider Native packages.
`install.sh` builds and installs into its first argument (default `build/install`) without
launching the app or tests. With no arguments the app keeps an in-memory sample.
`--document PATH` opens or attaches a destination; Save publishes only on request.

All test and app execution is deferred to the final whole-goal batch. The registered
self-test creates its fixture through iiFileProvider inside the supplied build directory.
See [usage](docs/Usage.md) and the [SDK document contract](../../docs/native/TextDocuments.md).

# Native media gallery

C++23, Qt-free LVUI AOT example covering all eight native Card types, three Avatar sizes,
immutable background image delivery, reactive properties/factories and retained preview content.
The process and frame loop belong to iiSDKCore. iiGUI paints its own widgets.

Build with the installed `LVRS::LVRS` Native package. `install.sh` compiles and stages into
`build/install` by default; it never launches the app or runs tests. Pass package locations
as extra CMake arguments. All runtime validation is deferred to the final whole-goal batch.
See [usage](docs/Usage.md) for scope and limitations.

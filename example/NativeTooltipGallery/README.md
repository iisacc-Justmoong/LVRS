# Native Tooltip Gallery

A C++23 LVUI application using iiGUI's own retained bubble widgets, native fonts, PanelMaterial and an iiSDKCore application entry point. No OS tooltip or Qt control is created.

Hover or focus either origin. The first bubble contains live labels and an interactive child; the second uses wrapped text. The origin keeps its own click behavior. Available-area, text, placement and delay inputs bind to one retained backend. Core owns background work and all tooltip deadlines.

Build and privately install with ./install.sh, passing the staged SDK package locations. [Usage and verification scope](docs/Usage.md) explains the runtime contract. Installation only compiles and copies artifacts. Applications, tests and the registered self-test remain deferred to the final whole-goal runtime batch.

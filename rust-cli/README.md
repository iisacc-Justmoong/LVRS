# lvrs-cli

LVRS repository utility CLI bootstrap written in Rust.

## Quick Start

```bash
cd rust-cli
cargo run -- doctor
```

## Commands

- `install`: host-aware LVRS framework install flow with Linux dependency preflight and optional package-manager fixups. Outside the repository tree it can recover the checkout from installed source metadata, re-locate stale paths after upper-directory renames, treat `LVRS_ROOT`/`LVRS_PROJECT_ROOT` install-prefix values as `<prefix>/src/LVRS` hints, or reuse the installed source snapshot as a last resort.
- `bootstrap`: run desktop/mobile all-platform bootstrap install flow with `main.cpp` Main-entry defaults.
- `doctor`: host/bootstrap readiness checks. On Linux, `lvrs doctor --fix` can install missing distro packages before build, and `lvrs doctor --bootstrap` fails when the requested bootstrap matrix is not ready yet.
- `platform`: prints runtime platform information.
- `echo <message>`: simple plumbing check.

On macOS, `install` discovers the host Qt kit under `/Volumes/Storage/Qt` or an explicit `QT_VERSION_ROOT`, supplies the initial CMake package path, and keeps explicit Qt/CMake prefix overrides. A failed configure leaves the previous framework installation intact. Run the checkout's `./install.sh` after changing CLI sources to rebuild and replace the installed executable.

## Development

```bash
cargo fmt
cargo check
cargo test --locked --target-dir build
```

The macOS `install_qt_discovery` integration tests invoke the CLI and real CMake with temporary Qt package fixtures, covering relocated kit discovery, explicit overrides, and preservation of an existing installation when configuration fails.

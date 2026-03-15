# lvrs-cli

LVRS repository utility CLI bootstrap written in Rust.

## Quick Start

```bash
cd rust-cli
cargo run -- doctor
```

## Commands

- `install`: host-aware LVRS framework install flow with Linux dependency preflight and optional package-manager fixups. Outside the repository tree it can recover the checkout from installed source metadata, re-locate stale paths after upper-directory renames, or reuse the installed source snapshot as a last resort.
- `bootstrap`: run desktop/mobile all-platform bootstrap install flow with `main.cpp` Main-entry defaults.
- `doctor`: host/bootstrap readiness checks. On Linux, `lvrs doctor --fix` can install missing distro packages before build, and `lvrs doctor --bootstrap` fails when the requested bootstrap matrix is not ready yet.
- `platform`: prints runtime platform information.
- `echo <message>`: simple plumbing check.

## Development

```bash
cargo fmt
cargo check
```

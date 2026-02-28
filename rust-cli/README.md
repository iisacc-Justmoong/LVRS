# lvrs-cli

LVRS repository utility CLI bootstrap written in Rust.

## Quick Start

```bash
cd rust-cli
cargo run -- doctor
```

## Commands

- `bootstrap`: run desktop/mobile all-platform bootstrap install flow with `main.cpp` Main-entry defaults.
- `doctor`: baseline workspace checks for local development.
- `platform`: prints runtime platform information.
- `echo <message>`: simple plumbing check.

## Development

```bash
cargo fmt
cargo check
```

#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Use system toolchains inherited from the shell. Only load an explicitly requested file.
if [ -n "${LVRS_TOOLCHAIN_ENV_FILE:-}" ]; then
    if [ ! -f "${LVRS_TOOLCHAIN_ENV_FILE}" ]; then
        echo "[LVRS] Toolchain environment file not found: ${LVRS_TOOLCHAIN_ENV_FILE}" >&2
        exit 1
    fi
    . "${LVRS_TOOLCHAIN_ENV_FILE}"
fi

MANIFEST_PATH="${SCRIPT_DIR}/src/rust-cli/Cargo.toml"
CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-${SCRIPT_DIR}/build/rust-cli}"

if command -v cargo >/dev/null 2>&1 && [ -f "${MANIFEST_PATH}" ]; then
    exec env LVRS_ROOT="${SCRIPT_DIR}" CARGO_TARGET_DIR="${CARGO_TARGET_DIR}" cargo run --manifest-path "${MANIFEST_PATH}" --bin lvrs -- install "$@"
fi

if command -v lvrs >/dev/null 2>&1; then
    exec env LVRS_ROOT="${SCRIPT_DIR}" lvrs install "$@"
fi

echo "[LVRS] lvrs command not found and cargo bootstrap is unavailable." >&2
echo "[LVRS] Build the CLI first: cargo build --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli --bin lvrs" >&2
exit 1

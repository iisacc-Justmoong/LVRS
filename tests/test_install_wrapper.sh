#!/bin/sh
set -eu
REPO=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$REPO/build"
TEST_ROOT=$(mktemp -d "$REPO/build/install-wrapper-test.XXXXXX")
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM
mkdir -p "$TEST_ROOT/SDK/LVRS/src/rust-cli" "$TEST_ROOT/SDK/Toolchains" "$TEST_ROOT/bin"
cp "$REPO/install.sh" "$TEST_ROOT/SDK/LVRS/install.sh"
: > "$TEST_ROOT/SDK/LVRS/src/rust-cli/Cargo.toml"
printf '%s\n' 'export LVRS_TEST_TOOLCHAIN=workspace' > "$TEST_ROOT/SDK/Toolchains/env.sh"
cat > "$TEST_ROOT/bin/cargo" <<'CARGO'
#!/bin/sh
printf '%s\n' "${LVRS_TEST_TOOLCHAIN:-missing}" "$CARGO_TARGET_DIR" "$@" > "$LVRS_TEST_OUTPUT"
CARGO
chmod +x "$TEST_ROOT/bin/cargo"
export LVRS_TEST_OUTPUT="$TEST_ROOT/output"
export PATH="$TEST_ROOT/bin:$PATH"
unset CARGO_TARGET_DIR LVRS_TOOLCHAIN_ENV_FILE LVRS_TEST_TOOLCHAIN
sh "$TEST_ROOT/SDK/LVRS/install.sh" --prefix '/path with spaces' -- '-DVALUE=a b'
test "$(sed -n '1p' "$LVRS_TEST_OUTPUT")" = missing
test "$(sed -n '2p' "$LVRS_TEST_OUTPUT")" = "$TEST_ROOT/SDK/LVRS/build/rust-cli"
grep -Fx '/path with spaces' "$LVRS_TEST_OUTPUT"
grep -Fx -- '-DVALUE=a b' "$LVRS_TEST_OUTPUT"
LVRS_TEST_TOOLCHAIN=system sh "$TEST_ROOT/SDK/LVRS/install.sh"
test "$(sed -n '1p' "$LVRS_TEST_OUTPUT")" = system
printf '%s\n' 'export LVRS_TEST_TOOLCHAIN=override' > "$TEST_ROOT/custom env.sh"
LVRS_TOOLCHAIN_ENV_FILE="$TEST_ROOT/custom env.sh" sh "$TEST_ROOT/SDK/LVRS/install.sh"
test "$(sed -n '1p' "$LVRS_TEST_OUTPUT")" = override
if LVRS_TOOLCHAIN_ENV_FILE="$TEST_ROOT/missing" sh "$TEST_ROOT/SDK/LVRS/install.sh" > "$TEST_ROOT/error" 2>&1; then
    echo 'Missing explicit toolchain file unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'Toolchain environment file not found' "$TEST_ROOT/error"
echo 'Install wrapper tests passed.'

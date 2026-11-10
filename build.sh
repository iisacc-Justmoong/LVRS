#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_EXAMPLES=OFF
BUILD_TESTS=ON
CLEAN_BUILD=0

log() {
    printf '%s\n' "[LVRS][build.sh] $*"
}

fail() {
    printf '%s\n' "[LVRS][build.sh] $*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage: ./build.sh [options] [-- <extra-cmake-configure-args...>]

Options:
  --build-dir <path>      Select ./build (the only supported build directory)
  --with-examples         Also build the runnable example applications
  --without-examples      Configure with LVRS_BUILD_EXAMPLES=OFF
  --without-tests         Configure with LVRS_BUILD_TESTS=OFF
  --clean                 Clean generated outputs with the existing build system
  --help                  Show this help

Examples:
  ./build.sh
  ./build.sh --without-tests
  ./build.sh --with-examples -- -DCMAKE_BUILD_TYPE=RelWithDebInfo
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir)
            [ $# -ge 2 ] || fail "--build-dir requires a path."
            BUILD_DIR="$2"
            shift 2
            ;;
        --without-examples)
            BUILD_EXAMPLES=OFF
            shift
            ;;
        --with-examples)
            BUILD_EXAMPLES=ON
            shift
            ;;
        --without-tests)
            BUILD_TESTS=OFF
            shift
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        *)
            fail "Unknown option: $1"
            ;;
    esac
done

case "${BUILD_DIR}" in
    /*) ;;
    *) BUILD_DIR="${SCRIPT_DIR}/${BUILD_DIR}" ;;
esac

[ "${BUILD_DIR}" = "${SCRIPT_DIR}/build" ] || fail "The build directory must be ${SCRIPT_DIR}/build."

for extra_cmake_arg in "$@"; do
    case "${extra_cmake_arg}" in
        -DLVRS_BUILD_EXAMPLES*|-DLVRS_BUILD_TESTS*|-DBUILD_TESTING*)
            fail "Do not override LVRS_BUILD_EXAMPLES/LVRS_BUILD_TESTS through extra CMake args. Use the corresponding build.sh options."
            ;;
    esac
done

if [ "${CLEAN_BUILD}" -eq 1 ] && [ -d "${BUILD_DIR}" ]; then
    [ -f "${BUILD_DIR}/CMakeCache.txt" ] || fail "Cannot clean an unconfigured build directory; inspect it first."
    log "Cleaning generated build outputs: ${BUILD_DIR}"
    cmake --build "${BUILD_DIR}" --target clean
fi

log "Configuring ${BUILD_DIR}"
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DLVRS_BUILD_EXAMPLES="${BUILD_EXAMPLES}" \
    -DLVRS_BUILD_TESTS="${BUILD_TESTS}" \
    -DBUILD_TESTING="${BUILD_TESTS}" \
    "$@"

log "Building default targets"
cmake --build "${BUILD_DIR}" --parallel

if [ "${BUILD_EXAMPLES}" = "ON" ] && grep -q '^LVRS_BACKEND:STRING=qt$' "${BUILD_DIR}/CMakeCache.txt"; then
    log "Ensuring host example outputs are populated"
    cmake --build "${BUILD_DIR}" --target lvrs_host_examples_all --parallel
fi

check_macos_runtime() {
    build_binary="$1"

    if ! command -v otool >/dev/null 2>&1; then
        return
    fi

    otool -L "${build_binary}" | grep -F "@rpath/libLVRS.dylib" >/dev/null 2>&1 \
        || fail "Missing @rpath/libLVRS.dylib linkage in ${build_binary}"

    otool -l "${build_binary}" | grep -F "@loader_path/../../../" >/dev/null 2>&1 \
        || fail "Missing build-tree runtime rpath in ${build_binary}"
}

check_launcher_consistency() {
    find "${SCRIPT_DIR}/example" -type f -path '*/bin/LVRSExample*' -print -quit | grep -q . \
        || fail "No checked-in example launchers were found under ${SCRIPT_DIR}/example."
    find "${SCRIPT_DIR}/example" -type f -path '*/bin/LVRSExample*' | while IFS= read -r launcher; do
        first_line=$(sed -n '1p' "${launcher}")
        [ "${first_line}" = "#!/bin/sh" ] \
            || fail "Launcher is not a shell script: ${launcher}"

        if [ "${BUILD_EXAMPLES}" != "ON" ]; then
            continue
        fi

        example_name=$(basename -- "$(dirname -- "$(dirname -- "${launcher}")")")
        launcher_name=$(basename -- "${launcher}")
        build_binary="${BUILD_DIR}/example/${example_name}/bin/${launcher_name}"

        [ -x "${build_binary}" ] \
            || fail "Missing built example runtime for launcher: ${launcher} -> ${build_binary}"

        if [ "$(uname -s)" = "Darwin" ]; then
            check_macos_runtime "${build_binary}"
        fi
    done
}

if [ "${BUILD_EXAMPLES}" = "ON" ] && grep -q '^LVRS_BACKEND:STRING=qt$' "${BUILD_DIR}/CMakeCache.txt"; then
    log "Checking launcher and build output consistency"
    check_launcher_consistency
fi

log "Build consistency checks passed"

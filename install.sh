#!/usr/bin/env sh
set -eu

# LVRS unified installer.
# Goal: after git clone + ./install.sh, downstream projects can use:
#   find_package(LVRS CONFIG REQUIRED)
# without manually adding LVRS to CMAKE_PREFIX_PATH.

usage() {
    cat <<'EOF'
Usage: ./install.sh [options]
Builds and installs LVRS for selected runtime platforms via bootstrap_lvrs_all.

Options:
  --prefix <path>      Install prefix (default: ~/.local/LVRS)
  --build-dir <path>   Deprecated. Build directory is fixed to <repo>/build
  --build-type <type>  CMake build type (default: Release)
  --platforms <list>   Bootstrap platforms (comma/semicolon list; default: host only)
  --clean              Deprecated no-op (clean reinstall is always enabled)
  --without-examples   Disable host configure-time example targets
  --without-tests      Disable host configure-time test targets
  --force-x86-qt-tools Deprecated (unsupported): Apple x86 paths are disabled
  --no-source-snapshot Skip source snapshot copy into <prefix>/src/LVRS
  --no-registry        Skip CMake user package registry registration
  --                   Pass remaining args to cmake configure
  -h, --help           Show this help
EOF
}

detect_host_platform() {
    if [ "${OS:-}" = "Windows_NT" ]; then
        echo "windows"
        return
    fi

    uname_s=$(uname -s 2>/dev/null || echo unknown)
    case "${uname_s}" in
        Darwin) echo "macos" ;;
        Linux) echo "linux" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "unknown" ;;
    esac
}

detect_bootstrap_framework_platforms() {
    host_platform="$1"
    case "${host_platform}" in
        macos|linux|windows|ios|android|wasm)
            echo "${host_platform}"
            ;;
        *)
            # Unknown host: keep legacy superset so caller can override explicitly.
            echo "macos;linux;windows;ios;android;wasm"
            ;;
    esac
}

normalize_platform_list() {
    echo "$1" | tr ',' ';'
}

lvrs_clean_recreate_dir() {
    target_dir="$1"
    target_name="$2"

    if [ ! -e "${target_dir}" ]; then
        cmake -E make_directory "${target_dir}"
        return 0
    fi

    if cmake -E rm -rf "${target_dir}"; then
        cmake -E make_directory "${target_dir}"
        return 0
    fi

    parent_dir=$(dirname "${target_dir}")
    base_dir=$(basename "${target_dir}")
    stale_dir="${parent_dir}/.${base_dir}.lvrs-stale-$$"

    if [ -e "${stale_dir}" ]; then
        cmake -E rm -rf "${stale_dir}" || true
    fi

    if ! mv "${target_dir}" "${stale_dir}"; then
        echo "[LVRS] Failed to relocate ${target_name} directory: ${target_dir}" >&2
        echo "[LVRS] Check running processes that keep the directory busy, then retry." >&2
        return 1
    fi

    cmake -E make_directory "${target_dir}"

    if ! cmake -E rm -rf "${stale_dir}"; then
        echo "[LVRS] Warning: stale ${target_name} directory remains: ${stale_dir}" >&2
        echo "[LVRS] New ${target_name} directory is clean; continuing reinstall." >&2
    fi

    return 0
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ "${HOME:-}" ]; then
    HOME_DIR="$HOME"
elif [ "${USERPROFILE:-}" ]; then
    HOME_DIR="$USERPROFILE"
else
    echo "HOME/USERPROFILE environment variable is required." >&2
    exit 1
fi

INSTALL_PREFIX="${HOME_DIR}/.local/LVRS"
PLATFORM_INSTALL_ROOT="${INSTALL_PREFIX}/platforms"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
SOURCE_SNAPSHOT=1
REGISTER_CMAKE_REGISTRY=1
BUILD_EXAMPLES=1
BUILD_TESTS=1
HOST_PLATFORM="$(detect_host_platform)"
HOST_INSTALL_PREFIX="${PLATFORM_INSTALL_ROOT}/${HOST_PLATFORM}"
BOOTSTRAP_FRAMEWORK_PLATFORMS="${LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS:-}"
if [ -z "${BOOTSTRAP_FRAMEWORK_PLATFORMS}" ]; then
    BOOTSTRAP_FRAMEWORK_PLATFORMS="$(detect_bootstrap_framework_platforms "${HOST_PLATFORM}")"
else
    BOOTSTRAP_FRAMEWORK_PLATFORMS="$(normalize_platform_list "${BOOTSTRAP_FRAMEWORK_PLATFORMS}")"
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            [ "$#" -ge 2 ] || { echo "Missing value for --prefix" >&2; exit 1; }
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --build-dir)
            [ "$#" -ge 2 ] || { echo "Missing value for --build-dir" >&2; exit 1; }
            BUILD_DIR_ARG="$2"
            if [ "${BUILD_DIR_ARG}" != "${PROJECT_ROOT}/build" ] && [ "${BUILD_DIR_ARG}" != "build" ] && [ "${BUILD_DIR_ARG}" != "./build" ]; then
                echo "[LVRS] --build-dir is deprecated. Use the fixed build directory: ${PROJECT_ROOT}/build" >&2
                exit 1
            fi
            BUILD_DIR="${PROJECT_ROOT}/build"
            shift 2
            ;;
        --build-type)
            [ "$#" -ge 2 ] || { echo "Missing value for --build-type" >&2; exit 1; }
            BUILD_TYPE="$2"
            shift 2
            ;;
        --platforms)
            [ "$#" -ge 2 ] || { echo "Missing value for --platforms" >&2; exit 1; }
            BOOTSTRAP_FRAMEWORK_PLATFORMS="$(normalize_platform_list "$2")"
            shift 2
            ;;
        --clean)
            # Deprecated: keep for backward compatibility.
            shift
            ;;
        --without-examples)
            BUILD_EXAMPLES=0
            shift
            ;;
        --without-tests)
            BUILD_TESTS=0
            shift
            ;;
        --force-x86-qt-tools)
            echo "[LVRS] --force-x86-qt-tools is unsupported. Apple x86 paths are disabled." >&2
            exit 1
            ;;
        --no-source-snapshot)
            SOURCE_SNAPSHOT=0
            shift
            ;;
        --no-registry)
            REGISTER_CMAKE_REGISTRY=0
            shift
            ;;
        --)
            shift
            break
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

PLATFORM_INSTALL_ROOT="${INSTALL_PREFIX}/platforms"
HOST_INSTALL_PREFIX="${PLATFORM_INSTALL_ROOT}/${HOST_PLATFORM}"
if [ -z "${BOOTSTRAP_FRAMEWORK_PLATFORMS}" ]; then
    BOOTSTRAP_FRAMEWORK_PLATFORMS="$(detect_bootstrap_framework_platforms "${HOST_PLATFORM}")"
fi

SOURCE_INSTALL_DIR="${INSTALL_PREFIX}/src/LVRS"
PACKAGE_CONFIG_DIR="${HOST_INSTALL_PREFIX}/lib/cmake/LVRS"

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake is required but not found in PATH." >&2
    exit 1
fi

echo "[LVRS] Project root : ${PROJECT_ROOT}"
echo "[LVRS] Build dir    : ${BUILD_DIR}"
echo "[LVRS] Install dir  : ${INSTALL_PREFIX}"
echo "[LVRS] Platforms dir: ${PLATFORM_INSTALL_ROOT}"
echo "[LVRS] Host platform: ${HOST_PLATFORM}"
echo "[LVRS] Bootstrap targets: ${BOOTSTRAP_FRAMEWORK_PLATFORMS}"
echo "[LVRS] Build type   : ${BUILD_TYPE}"
echo "[LVRS] Registry     : ${REGISTER_CMAKE_REGISTRY}"
echo "[LVRS] Snapshot     : ${SOURCE_SNAPSHOT}"
echo "[LVRS] Examples     : ${BUILD_EXAMPLES}"
echo "[LVRS] Tests        : ${BUILD_TESTS}"
echo "[LVRS] Clean mode   : forced reinstall"

echo "[LVRS] Cleaning build directory..."
if ! lvrs_clean_recreate_dir "${BUILD_DIR}" "build"; then
    exit 1
fi

echo "[LVRS] Cleaning previous LVRS install artifacts..."
cmake -E rm -rf \
    "${INSTALL_PREFIX}/platforms" \
    "${INSTALL_PREFIX}/include/LVRS" \
    "${INSTALL_PREFIX}/lib/cmake/LVRS" \
    "${INSTALL_PREFIX}/lib/qt6/qml/LVRS" \
    "${INSTALL_PREFIX}/lib/AGL.framework" \
    "${SOURCE_INSTALL_DIR}"
for _lvrs_binary in \
    "${INSTALL_PREFIX}/lib/libLVRS.dylib" \
    "${INSTALL_PREFIX}/lib/libLVRS.so" \
    "${INSTALL_PREFIX}/lib/libLVRS.a" \
    "${INSTALL_PREFIX}/lib/LVRS.lib" \
    "${INSTALL_PREFIX}/bin/LVRS.dll"
do
    if [ -e "${_lvrs_binary}" ]; then
        cmake -E rm -f "${_lvrs_binary}"
    fi
done

if [ "${BUILD_EXAMPLES}" -eq 1 ]; then
    LVRS_BUILD_EXAMPLES_VALUE=ON
else
    LVRS_BUILD_EXAMPLES_VALUE=OFF
fi

if [ "${BUILD_TESTS}" -eq 1 ]; then
    LVRS_BUILD_TESTS_VALUE=ON
else
    LVRS_BUILD_TESTS_VALUE=OFF
fi

if ! cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DLVRS_BUILD_SHARED_LIBS=ON \
    -DLVRS_BUILD_EXAMPLES="${LVRS_BUILD_EXAMPLES_VALUE}" \
    -DLVRS_BUILD_TESTS="${LVRS_BUILD_TESTS_VALUE}" \
    -DLVRS_BOOTSTRAP_INSTALL_ROOT="${PLATFORM_INSTALL_ROOT}" \
    -DLVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS="${BOOTSTRAP_FRAMEWORK_PLATFORMS}" \
    -DLVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES=OFF \
    -DLVRS_BOOTSTRAP_LVRS_BUILD_TESTS=OFF \
    -DLVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS=ON \
    -DLVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE=ON \
    "$@"; then
    echo "[LVRS] Configure failed." >&2
    echo "[LVRS] If Qt is not auto-detected, pass your Qt prefix, e.g.:" >&2
    echo "       CMAKE_PREFIX_PATH=/path/to/Qt ./install.sh" >&2
    exit 1
fi

if ! cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --target bootstrap_lvrs_all; then
    echo "[LVRS] Build failed." >&2
    echo "[LVRS] Apple targets never use x86. Check iOS/Qt kit architecture (arm64) and retry." >&2
    exit 1
fi

echo "[LVRS] Multi-platform framework install completed."

if [ "${SOURCE_SNAPSHOT}" -eq 1 ]; then
    echo "[LVRS] Installing source snapshot..."
    cmake -E rm -rf "${SOURCE_INSTALL_DIR}"
    cmake -E make_directory "${SOURCE_INSTALL_DIR}"

    for entry in "${PROJECT_ROOT}"/*; do
        if [ -f "${entry}" ]; then
            cmake -E copy "${entry}" "${SOURCE_INSTALL_DIR}/"
        fi
    done

    for entry in "${PROJECT_ROOT}"/*; do
        [ -d "${entry}" ] || continue
        name=$(basename "${entry}")
        case "${name}" in
            .git|build|build-*|cmake-build-*|.idea|.vscode)
                continue
                ;;
        esac
        cmake -E copy_directory "${entry}" "${SOURCE_INSTALL_DIR}/${name}"
    done

    if command -v git >/dev/null 2>&1 && git -C "${PROJECT_ROOT}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        SOURCE_REVISION=$(git -C "${PROJECT_ROOT}" rev-parse HEAD 2>/dev/null || echo unknown)
    else
        SOURCE_REVISION="unknown"
    fi

    {
        echo "LVRS source snapshot"
        echo "project_root=${PROJECT_ROOT}"
        echo "source_revision=${SOURCE_REVISION}"
        echo "installed_at=$(date '+%Y-%m-%d %H:%M:%S %z')"
    } > "${SOURCE_INSTALL_DIR}/INSTALL_SOURCE_INFO.txt"
fi

if [ "${REGISTER_CMAKE_REGISTRY}" -eq 1 ]; then
    if [ "${APPDATA:-}" ]; then
        CMAKE_USER_PACKAGE_DIR="${APPDATA}/CMake/packages/LVRS"
    else
        CMAKE_USER_PACKAGE_DIR="${HOME_DIR}/.cmake/packages/LVRS"
    fi
    cmake -E make_directory "${CMAKE_USER_PACKAGE_DIR}"

    # Remove stale entries that point to old LVRS installs under the same prefix root.
    if [ -d "${CMAKE_USER_PACKAGE_DIR}" ]; then
        for entry in "${CMAKE_USER_PACKAGE_DIR}"/*; do
            [ -f "${entry}" ] || continue
            if grep -Fq "${INSTALL_PREFIX}" "${entry}" 2>/dev/null; then
                cmake -E rm -f "${entry}"
            fi
        done
    fi

    if [ -d "${PACKAGE_CONFIG_DIR}" ]; then
        REGISTRY_ENTRY="${CMAKE_USER_PACKAGE_DIR}/$(date +%s)-$$"
        printf '%s\n' "${PACKAGE_CONFIG_DIR}" > "${REGISTRY_ENTRY}"
        echo "[LVRS] Registered CMake package: ${REGISTRY_ENTRY}"
    else
        echo "[LVRS] Registry skip: host package dir not found -> ${PACKAGE_CONFIG_DIR}"
    fi
fi

ENV_FILE="${INSTALL_PREFIX}/env.sh"
{
    echo "#!/usr/bin/env sh"
    echo "# LVRS environment helper"
    echo "export LVRS_PLATFORMS_ROOT=\"${PLATFORM_INSTALL_ROOT}\""
    echo "export LVRS_HOST_PLATFORM=\"${HOST_PLATFORM}\""
    echo "export LVRS_HOST_PREFIX=\"${HOST_INSTALL_PREFIX}\""
    echo "export CMAKE_PREFIX_PATH=\"${INSTALL_PREFIX}:\${CMAKE_PREFIX_PATH:-}\""
    echo "export QML2_IMPORT_PATH=\"${HOST_INSTALL_PREFIX}/lib/qt6/qml:\${QML2_IMPORT_PATH:-}\""
} > "${ENV_FILE}"
chmod +x "${ENV_FILE}"

echo "[LVRS] Install completed."
echo "[LVRS] CMake package dir : ${PACKAGE_CONFIG_DIR}"
echo "[LVRS] Platforms root    : ${PLATFORM_INSTALL_ROOT}"
echo "[LVRS] Env helper        : ${ENV_FILE}"
echo "[LVRS] Downstream CMake  : find_package(LVRS CONFIG REQUIRED)"

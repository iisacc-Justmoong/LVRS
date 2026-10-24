#!/usr/bin/env bash
set -euo pipefail
material_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
material_example_destination="${1:-${material_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${material_example_root}" -B "${material_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${material_example_destination}" "$@"
cmake --build "${material_example_root}/build" --parallel 4
cmake --install "${material_example_root}/build"

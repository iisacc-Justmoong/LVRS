#!/usr/bin/env bash
set -euo pipefail
surfaces_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
surfaces_example_destination="${1:-${surfaces_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${surfaces_example_root}" -B "${surfaces_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${surfaces_example_destination}" "$@"
cmake --build "${surfaces_example_root}/build" --parallel 4
cmake --install "${surfaces_example_root}/build"

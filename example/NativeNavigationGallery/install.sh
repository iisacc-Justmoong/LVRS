#!/usr/bin/env bash
set -euo pipefail
navigation_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
navigation_example_destination="${1:-${navigation_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${navigation_example_root}" -B "${navigation_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${navigation_example_destination}" "$@"
cmake --build "${navigation_example_root}/build" --parallel 4
cmake --install "${navigation_example_root}/build"

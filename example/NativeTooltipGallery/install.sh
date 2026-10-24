#!/usr/bin/env bash
set -euo pipefail
tooltip_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
tooltip_example_destination="${1:-${tooltip_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${tooltip_example_root}" -B "${tooltip_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${tooltip_example_destination}" "$@"
cmake --build "${tooltip_example_root}/build" --parallel 4
cmake --install "${tooltip_example_root}/build"

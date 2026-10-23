#!/usr/bin/env bash
set -euo pipefail
pages_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
pages_example_destination="${1:-${pages_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${pages_example_root}" -B "${pages_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${pages_example_destination}" "$@"
cmake --build "${pages_example_root}/build" --parallel 4
cmake --install "${pages_example_root}/build"

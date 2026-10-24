#!/usr/bin/env bash
set -euo pipefail
media_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
media_example_destination="${1:-${media_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${media_example_root}" -B "${media_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${media_example_destination}" "$@"
cmake --build "${media_example_root}/build" --parallel 4
cmake --install "${media_example_root}/build"

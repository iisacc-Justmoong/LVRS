#!/usr/bin/env bash
set -euo pipefail
document_example_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
document_example_destination="${1:-${document_example_root}/build/install}"
if (($#)); then shift; fi
cmake -S "${document_example_root}" -B "${document_example_root}/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "-DCMAKE_INSTALL_PREFIX=${document_example_destination}" "$@"
cmake --build "${document_example_root}/build" --parallel 4
cmake --install "${document_example_root}/build"

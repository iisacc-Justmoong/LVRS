#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "$project_dir/build" -j 2
cmake --install "$project_dir/build"

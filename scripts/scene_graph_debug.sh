#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE'
Scene Graph debug launcher

Usage:
  scripts/scene_graph_debug.sh [--mode overdraw|changes|none] [--app /path/to/app] [--dry-run] [-- app_args...]

Defaults:
  --mode overdraw
  --app  ./build/bin/LVRSExampleVisualCatalog

Behavior:
  - always exports QSG_RENDER_TIMING=1
  - exports QSG_VISUALIZE=overdraw|changes when mode is overdraw|changes
  - unsets QSG_VISUALIZE when mode is none
USAGE
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

mode="overdraw"
app_path="${repo_root}/build/bin/LVRSExampleVisualCatalog"
dry_run="0"
pass_through=()

while (($#)); do
    case "$1" in
        --help|-h)
            usage
            exit 0
            ;;
        --mode)
            if (($# < 2)); then
                echo "Missing value for --mode" >&2
                exit 2
            fi
            mode="$2"
            shift 2
            ;;
        --app)
            if (($# < 2)); then
                echo "Missing value for --app" >&2
                exit 2
            fi
            app_path="$2"
            shift 2
            ;;
        --dry-run)
            dry_run="1"
            shift
            ;;
        --)
            shift
            while (($#)); do
                pass_through+=("$1")
                shift
            done
            ;;
        *)
            pass_through+=("$1")
            shift
            ;;
    esac
done

case "${mode}" in
    overdraw|changes|none)
        ;;
    *)
        echo "Unsupported mode: ${mode}" >&2
        echo "Allowed modes: overdraw, changes, none" >&2
        exit 2
        ;;
esac

if [[ ! -x "${app_path}" ]]; then
    echo "App executable not found or not executable: ${app_path}" >&2
    exit 3
fi

export QSG_RENDER_TIMING=1
if [[ "${mode}" == "none" ]]; then
    unset QSG_VISUALIZE || true
else
    export QSG_VISUALIZE="${mode}"
fi

echo "[scene-graph-debug] QSG_RENDER_TIMING=${QSG_RENDER_TIMING}"
echo "[scene-graph-debug] QSG_VISUALIZE=${QSG_VISUALIZE:-<unset>}"
echo "[scene-graph-debug] APP=${app_path}"

if [[ "${dry_run}" == "1" ]]; then
    exit 0
fi

if ((${#pass_through[@]} > 0)); then
    exec "${app_path}" "${pass_through[@]}"
else
    exec "${app_path}"
fi

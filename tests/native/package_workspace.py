"""Owned package-test workspaces and installation manifest isolation."""
from contextlib import contextmanager
from pathlib import Path, PureWindowsPath
import re
import shutil
import ii_file_provider as provider


def checked_paths(source, raw_build, raw_workspace):
    source = Path(source).resolve(strict=True)
    raw_build, raw_workspace = Path(raw_build), Path(raw_workspace)
    state, root = source / "resources/.t", source / "build"
    if any(p.is_symlink() for p in (raw_build, raw_workspace, state, root)):
        raise ValueError("Build and resource roots must be direct directories")
    build, workspace, state, root = (p.resolve(strict=True) for p in (raw_build, raw_workspace, state, root))
    if not build.is_relative_to(root) or workspace.parent != state:
        raise ValueError("Package checks must use this project's build and resource workspace")
    if not re.fullmatch(r"[0-9a-f]{4}", workspace.name):
        raise ValueError("Unexpected resource workspace name")
    return build, workspace


def redirect_manifest(content, build, workspace):
    for name in ("install_local_manifest.txt", "${CMAKE_INSTALL_MANIFEST}"):
        old = build.as_posix() + "/" + name
        if content.count(old) != 1:
            raise ValueError("Cannot safely redirect the installation manifest")
        content = content.replace(old, workspace.as_posix() + "/" + name)
    return content


def validate_install_dirs(cache):
    for name in ("BINDIR", "DATADIR", "DATAROOTDIR", "INCLUDEDIR", "LIBDIR"):
        match = re.search(r"^CMAKE_INSTALL_" + name + r":[^=]*=(.*)$", cache, re.M)
        if match and match[1] and (Path(match[1]).is_absolute() or PureWindowsPath(match[1]).is_absolute()
                                   or ".." in Path(match[1]).parts or ".." in PureWindowsPath(match[1]).parts):
            raise ValueError("Unsafe install directory: " + name)


@contextmanager
def consumer_build(build, token, kind):
    if not re.fullmatch(r"[0-9a-f]{4}", token) or kind not in ("abi", "presentation"):
        raise ValueError("Unexpected consumer workspace name")
    path = build / (".lvrs-" + kind + "-" + token)
    provider.mkdir(path, exist_ok=False)
    try:
        yield path
    finally:
        shutil.rmtree(path)

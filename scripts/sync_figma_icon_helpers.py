#!/usr/bin/env python3
"""Resolve Figma token bindings for LVRS control and helper SVG resources."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

import reconcile_figma_icon_colors as reconcile

ROOT = Path(__file__).resolve().parents[1]
BINDINGS = ROOT / "resources/figma-icon-token-bindings.json"
TOKENS = ROOT / "resources/figma-color-tokens.json"
MANIFEST = ROOT / "docs/figma-iconset-manifest.json"


def hex_color(color):
    return "#" + "".join(f"{round(color[key] * 255):02X}" for key in ("r", "g", "b"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    data = json.loads(BINDINGS.read_text())
    tokens = {item["name"]: item["value"] for item in json.loads(TOKENS.read_text())["variables"]}
    manifest = json.loads(MANIFEST.read_text())
    assets = {item["path"]: item for item in manifest["accentPaletteUpdate"]["additionalResources"]}
    prepared = []
    for icon in data["icons"]:
        path = ROOT / "resources" / icon["path"]
        original = path.read_bytes()
        if icon["path"] in assets and hashlib.sha256(original).hexdigest() != assets[icon["path"]]["sha256"]:
            raise ValueError("Helper SVG changed since its manifest: " + icon["path"])
        elements = list(ET.fromstring(original).iter())
        edits = []
        for binding in icon["bindings"]:
            index, attribute = binding["element"], binding["attribute"]
            if attribute not in {"fill", "stroke"} or not 0 <= index < len(elements):
                raise ValueError("Invalid SVG token binding: " + icon["path"])
            current = elements[index].get(attribute)
            expected = hex_color(tokens[binding["token"]])
            if current not in {binding["resolved"], expected}:
                raise ValueError("Helper paint changed independently: " + icon["path"])
            if current != expected:
                edits.append({"index": index, "key": attribute, "before": current, "after": expected})
            binding["resolved"] = expected
        updated = reconcile.apply_paint_edits(original, edits) if edits else original
        prepared.append((icon, path, original, updated))
    if args.check:
        stale = [str(path.relative_to(ROOT)) for _, path, old, new in prepared if old != new]
        if stale or json.loads(BINDINGS.read_text()) != data:
            print("Out-of-date Figma helper bindings: " + ", ".join(stale), file=sys.stderr)
            return 1
    else:
        try:
            import ii_file_provider as provider
        except ModuleNotFoundError:
            sys.path.insert(0, str(Path.home() / ".local/SDK/iiFileProvider/share/iiFileProvider/python"))
            import ii_file_provider as provider
        for icon, path, old, new in prepared:
            if path.read_bytes() != old:
                raise ValueError("Helper SVG changed during reconciliation: " + icon["path"])
            if old != new:
                provider.write_bytes(path, new)
            if icon["path"] in assets:
                assets[icon["path"]]["sha256"] = hashlib.sha256(new).hexdigest()
                assets[icon["path"]]["tokenBindings"] = icon["bindings"]
        provider.write_text(BINDINGS, json.dumps(data, ensure_ascii=False, indent=2) + "\n")
        provider.write_text(MANIFEST, json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    print(f"Figma helper SVGs {'verified' if args.check else 'updated'}: {len(prepared)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

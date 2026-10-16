#!/usr/bin/env python3
"""Import a complete, verified LVRS Figma SVG snapshot without losing control assets."""

import argparse
import hashlib
import json
from pathlib import Path
import unicodedata
import xml.etree.ElementTree as ET


def digest(data):
    return hashlib.sha256(data).hexdigest()


def filename_key(name):
    return unicodedata.normalize("NFKC", name).casefold()


def import_snapshot(repo, export_path, aliases_path, captured_on, expected_count,
                    compatibility_dir=None):
    rows = json.loads(export_path.read_text())
    aliases = json.loads(aliases_path.read_text())
    if expected_count <= 0 or len(rows) != expected_count:
        raise ValueError("The export must contain the complete expected icon inventory")
    manifest_path = repo / "docs/figma-iconset-manifest.json"
    previous = json.loads(manifest_path.read_text())
    resource_dir = repo / "resources/iconset"
    managed = {filename_key(icon["filename"]) for icon in previous["icons"]}
    auxiliary = {filename_key(path.name) for path in resource_dir.glob("*.svg")
                 if filename_key(path.name) not in managed}
    prepared = []
    names, node_ids = set(), set()
    for row in rows:
        filename = row["name"] + ".svg"
        key = filename_key(filename)
        if (any(character in filename for character in '<>:"/\\|?*')
                or filename.rstrip(" .") != filename or key in names or key in auxiliary):
            raise ValueError(f"Unsafe or conflicting icon filename: {filename}")
        if row["type"] not in {"COMPONENT", "VECTOR"} or row["id"] in node_ids:
            raise ValueError(f"Duplicate Figma node: {row['id']}")
        native = row["svg"].encode()
        ET.fromstring(native)
        native_hash = digest(native)
        if native_hash != row["nativeSha256"]:
            raise ValueError(f"Native export hash mismatch: {filename}")
        content = native
        override = compatibility_dir / filename if compatibility_dir else None
        if override and override.is_file():
            if row["name"] not in {"smartSelect", "translateObject"}:
                raise ValueError(f"Unexpected raster compatibility override: {filename}")
            content = override.read_bytes()
            ET.fromstring(content)
        icon = {
            "filename": filename, "exportName": filename, "name": row["name"],
            "nodeId": row["id"], "componentKey": row["key"], "type": row["type"],
            "category": row["category"], "width": row["width"], "height": row["height"],
            "nativeExportSha256": native_hash, "preResponsiveSha256": digest(content),
            "sha256": digest(content),
        }
        if content != native:
            icon["rasterCompatibility"] = "direct-embedded-image"
        if "accentVariableId" in row:
            icon["accentVariableId"] = row["accentVariableId"]
        prepared.append((filename, content, icon))
        names.add(key)
        node_ids.add(row["id"])

    canonical = {name for name, _, _ in prepared}
    alias_names = set()
    for alias in aliases:
        name, target = alias["filename"], alias["target"]
        if (not name.endswith(".svg") or "/" in name or "\\" in name
                or filename_key(name) in names or filename_key(name) in auxiliary
                or filename_key(name) in alias_names
                or target not in canonical):
            raise ValueError(f"Invalid compatibility alias: {name} -> {target}")
        alias_names.add(filename_key(name))

    # Validation completes before replacing the previously managed snapshot.
    # Removing first also fixes case-only filenames on case-insensitive hosts.
    for path in resource_dir.glob("*.svg"):
        if filename_key(path.name) in managed:
            path.unlink()
    for filename, content, _ in prepared:
        (resource_dir / filename).write_bytes(content)

    qrc = ET.Element("RCC")
    group = ET.SubElement(qrc, "qresource", prefix="/qt/qml/LVRS/resources/iconset")
    for alias in sorted(aliases, key=lambda item: item["filename"]):
        ET.SubElement(group, "file", alias=alias["filename"]).text = "iconset/" + alias["target"]
    ET.indent(qrc, space="    ")
    (repo / "resources/figma_icon_aliases.qrc").write_text(
        ET.tostring(qrc, encoding="unicode") + "\n")
    inventory = {
        "schemaVersion": 2, "capturedOn": captured_on,
        "fileKey": previous["fileKey"], "pageId": previous["pageId"],
        "pageName": previous["pageName"], "rootId": "1344:9163",
        "sourceUrl": previous["sourceUrl"], "count": len(prepared),
        "componentCount": sum(row["type"] == "COMPONENT" for row in rows),
        "standaloneVectorCount": sum(row["type"] == "VECTOR" for row in rows),
        "icons": sorted((item[2] for item in prepared), key=lambda item: item["filename"]),
        "legacyAliases": sorted(aliases, key=lambda item: item["filename"]),
    }
    manifest_path.write_text(json.dumps(inventory, ensure_ascii=False, indent=2) + "\n")
    print(f"Imported {len(prepared)} icons, preserved {len(auxiliary)} control assets, "
          f"registered {len(aliases)} legacy resource aliases.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", required=True, type=Path)
    parser.add_argument("--aliases", required=True, type=Path)
    parser.add_argument("--captured-on", required=True)
    parser.add_argument("--expected-count", required=True, type=int)
    parser.add_argument("--compatibility-dir", type=Path)
    args = parser.parse_args()
    import_snapshot(Path(__file__).resolve().parents[1], args.export, args.aliases,
                    args.captured_on, args.expected_count, args.compatibility_dir)


if __name__ == "__main__":
    main()

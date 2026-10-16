"""Regression checks for safe whole-catalog replacement and stable resource URLs."""

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import sys


sys.dont_write_bytecode = True
SPEC = importlib.util.spec_from_file_location(
    "import_figma_iconset", Path(__file__).resolve().parents[1] / "scripts/import_figma_iconset.py")
IMPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORTER)
SVG = b'<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18"><path d="M1 1H8V8Z" fill="#40B6E0"/></svg>'


class IconsetImportTests(unittest.TestCase):
    def setUp(self):
        build_dir = Path(__file__).resolve().parents[1] / "build"
        build_dir.mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=build_dir, prefix="iconset-import-")
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        (self.repo / "resources/iconset").mkdir(parents=True)
        (self.repo / "docs").mkdir()
        (self.repo / "resources/iconset/old.svg").write_bytes(SVG)
        (self.repo / "resources/iconset/StepperChevron.svg").write_bytes(SVG)
        (self.repo / "docs/figma-iconset-manifest.json").write_text(json.dumps({
            "icons": [{"filename": "old.svg"}], "fileKey": "fixture", "pageId": "198:2",
            "pageName": "Iconset", "sourceUrl": "https://www.figma.com/design/fixture",
        }))
        self.row = {"id": "1:2", "name": "new", "key": "component-key", "type": "COMPONENT",
                    "width": 18, "height": 18, "category": "Navigation", "svg": SVG.decode(),
                    "nativeSha256": hashlib.sha256(SVG).hexdigest()}
        self.export = self.repo / "export.json"
        self.aliases = self.repo / "aliases.json"
        self.aliases.write_text(json.dumps([{"filename": "old.svg", "target": "new.svg"}]))

    def run_import(self, rows, count=1):
        self.export.write_text(json.dumps(rows))
        IMPORTER.import_snapshot(self.repo, self.export, self.aliases, "2026-10-12", count)

    def test_incomplete_export_preserves_previous_snapshot(self):
        with self.assertRaises(ValueError):
            self.run_import([])
        self.assertEqual((self.repo / "resources/iconset/old.svg").read_bytes(), SVG)
        self.assertFalse((self.repo / "resources/figma_icon_aliases.qrc").exists())

    def test_wrong_native_hash_preserves_previous_snapshot(self):
        with self.assertRaises(ValueError):
            self.run_import([{**self.row, "nativeSha256": "0" * 64}])
        self.assertEqual((self.repo / "resources/iconset/old.svg").read_bytes(), SVG)

    def test_catalog_replacement_preserves_controls_and_legacy_url(self):
        self.run_import([self.row])
        self.assertFalse((self.repo / "resources/iconset/old.svg").exists())
        self.assertEqual((self.repo / "resources/iconset/new.svg").read_bytes(), SVG)
        self.assertEqual((self.repo / "resources/iconset/StepperChevron.svg").read_bytes(), SVG)
        qrc = IMPORTER.ET.parse(self.repo / "resources/figma_icon_aliases.qrc")
        self.assertEqual(qrc.find("qresource").attrib["prefix"],
                         "/qt/qml/LVRS/resources/iconset")
        self.assertEqual(qrc.find("qresource/file").attrib["alias"], "old.svg")
        self.assertEqual(qrc.find("qresource/file").text, "iconset/new.svg")


if __name__ == "__main__":
    unittest.main()

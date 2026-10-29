"""Regression checks for safe whole-catalog replacement and stable resource URLs."""

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import struct
import unittest
import sys


sys.dont_write_bytecode = True
SPEC = importlib.util.spec_from_file_location(
    "import_figma_iconset", Path(__file__).resolve().parents[1] / "scripts/import_figma_iconset.py")
IMPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORTER)
COLOR_SPEC = importlib.util.spec_from_file_location(
    "reconcile_figma_icon_colors", Path(__file__).resolve().parents[1] / "scripts/reconcile_figma_icon_colors.py")
COLORS = importlib.util.module_from_spec(COLOR_SPEC)
COLOR_SPEC.loader.exec_module(COLORS)
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

    def test_color_readback_preserves_geometry_and_updates_each_paint(self):
        original = b'<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" viewBox="0 0 18 18"><path d="M1 1H8V8Z" fill="#40B6E0"/><path d="M2 2H9" stroke="#40B6E0"/></svg>'
        readback = b'<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 18 18"><path d="M1.001 1H8V8Z" fill="#21D3FF"/><path d="M2 2H9" stroke="#B6BBC2"/></svg>'
        actual = COLORS.reconcile(original, readback)
        self.assertEqual(actual, original.replace(b'fill="#40B6E0"', b'fill="#21D3FF"')
                         .replace(b'stroke="#40B6E0"', b'stroke="#B6BBC2"'))
        self.assertEqual(COLORS.reconcile(SVG.replace(b'#40B6E0', b'black'), SVG.replace(b'#40B6E0', b'#0C0C0D')),
                         SVG.replace(b'#40B6E0', b'#0C0C0D'))

    def test_color_readback_rejects_changed_structure_or_geometry(self):
        self.assertIsNone(COLORS.reconcile(SVG, SVG.replace(b'M1 1H8', b'M3 1H8')))
        self.assertIsNone(COLORS.reconcile(SVG, SVG.replace(b'</svg>', b'<circle r="2"/></svg>')))
        self.assertIsNone(COLORS.reconcile(SVG, SVG.replace(b'fill="#40B6E0"', b'fill="url(#paint)"')))
        altered = SVG.replace(b'M1 1H8', b'M1 1L8 1')
        old_path, new_path = b'M1 1H8V8Z', b'M1 1L8 1V8Z'
        key = hashlib.sha256(old_path).hexdigest() + ':' + hashlib.sha256(new_path).hexdigest()
        self.assertIsNone(COLORS.reconcile(SVG, altered, {key: .9}))
        self.assertEqual(COLORS.reconcile(SVG, altered, {key: .99}), SVG)

    def test_color_readback_keeps_existing_rules_and_gradient_references(self):
        original = b'<svg xmlns="http://www.w3.org/2000/svg"><path d="M1 1H8" fill-rule="evenodd" fill="url(#old)"/><defs><linearGradient id="old"><stop stop-color="#40B6E0"/></linearGradient></defs></svg>'
        current = b'<svg xmlns="http://www.w3.org/2000/svg"><path d="M1 1H8" fill="url(#new)"/><defs><linearGradient id="new"><stop stop-color="#21D3FF"/></linearGradient></defs></svg>'
        self.assertEqual(COLORS.reconcile(original, current), original.replace(b'#40B6E0', b'#21D3FF'))

    def test_compact_color_readback_checks_geometry_and_original_paint(self):
        base = COLORS.fingerprint_svg(SVG)
        moved = COLORS.fingerprint_svg(SVG.replace(b'M1 1H8', b'M1.001 1H8'))
        self.assertEqual(base['geometry'], moved['geometry'])
        different = COLORS.fingerprint_svg(SVG.replace(b'M1 1H8', b'M1.1 1H8'))
        self.assertNotEqual(base['geometry'], different['geometry'])
        edit = {'index': 1, 'key': 'fill', 'before': '#40B6E0', 'after': '#21D3FF'}
        self.assertEqual(COLORS.apply_paint_edits(SVG, [edit]), SVG.replace(b'#40B6E0', b'#21D3FF'))
        with self.assertRaises(ValueError):
            COLORS.apply_paint_edits(SVG, [{**edit, 'before': '#000000'}])

    def test_embedded_png_retains_viewport_and_rejects_wrong_scale(self):
        original = b'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 18 18"/>'
        png = b'\x89PNG\r\n\x1a\n' + b'\0' * 8 + struct.pack('>II', 144, 144)
        result = COLORS.embedded_png_svg(original, png, 18, 18)
        self.assertIn(b'viewBox="0 0 18 18"', result)
        self.assertIn(b'data:image/png;base64,', result)
        with self.assertRaises(ValueError):
            COLORS.embedded_png_svg(original, png, 16, 16)

    def test_vector_fallback_keeps_responsive_viewport_and_rejects_unsupported_effects(self):
        old = b'<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" viewBox="-1 -1 20 20"/>'
        new = b'<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 18 18"><path d="M1 1H8" fill="#21D3FF"/></svg>'
        result = COLORS.current_vector_svg(old, new)
        self.assertIn(b'width="100%"', result)
        self.assertIn(b'viewBox="-1 -1 20 20"', result)
        self.assertIn(b'fill="#21D3FF"', result)
        with self.assertRaises(ValueError):
            COLORS.current_vector_svg(old, new.replace(b'<path ', b'<filter/><path '))

    def test_readback_records_its_actual_coverage(self):
        repo = Path(__file__).resolve().parents[1]
        manifest = json.loads((repo / "docs/figma-iconset-manifest.json").read_text())
        coverage = manifest["colorReadbackCoverage"]
        verified = [entry for entry in manifest["icons"] if "colorReadback" in entry]
        self.assertEqual(len(verified), coverage["verified"])
        self.assertEqual(coverage["verified"] + len(coverage["unresolvedNodeIds"]), coverage["captured"])
        self.assertEqual(coverage["captured"] + coverage["notCaptured"], manifest["count"])
        chevron = next(entry for entry in verified if entry["name"] == "generalchevronDown")
        payload = (repo / "resources/iconset" / chevron["filename"]).read_bytes()
        self.assertEqual(hashlib.sha256(payload).hexdigest(), chevron["sha256"])
        self.assertIn(b'stroke="#B6BBC2"', payload)


if __name__ == "__main__":
    unittest.main()

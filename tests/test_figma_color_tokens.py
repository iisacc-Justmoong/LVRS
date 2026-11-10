"""Regression checks for the captured LVRS Figma color and icon token contract."""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
TOKENS = ROOT / "resources/figma-color-tokens.json"
FIXTURE = ROOT / "tests/fixtures/figma-accent-roles.json"


def color_hex(value):
    return "#" + "".join(f"{round(value[key] * 255):02X}" for key in ("r", "g", "b"))


class FigmaColorTokenTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.snapshot = json.loads(TOKENS.read_text())
        cls.tokens = {row["name"]: row for row in cls.snapshot["variables"]}
        cls.fixture = json.loads(FIXTURE.read_text())

    def test_inventory_and_saved_fixtures(self):
        self.assertEqual(self.snapshot["fileKey"], "0GkItQYSNIR0lZ3iJhfJzc")
        self.assertEqual(len(self.tokens), 141)
        for section, expected in (("variables", 90), ("iconVariables", 29), ("backgroundVariables", 22)):
            rows = self.fixture[section]
            self.assertEqual(len(rows), expected)
            for row in rows:
                self.assertEqual(row["id"], self.tokens[row["name"]]["id"])
                self.assertEqual(row["value"], self.tokens[row["name"]]["value"])

    def test_generated_native_and_qml_colors_are_current(self):
        result = subprocess.run([sys.executable, "-B", str(ROOT / "scripts/sync_figma_tokens.py"), "--check"],
                                cwd=ROOT, text=True, capture_output=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_darker_icon_tokens_preserve_alpha_and_source_hue(self):
        for row in self.fixture["iconVariables"]:
            _, _, color, role, suffix = row["name"].split("/")
            self.assertEqual(suffix, "Darker")
            source = self.tokens[f"Accent/{color}/{role}"]["value"]
            actual = row["value"]
            for channel in ("r", "g", "b"):
                self.assertAlmostEqual(actual[channel], source[channel] * .9, delta=1e-7, msg=row["name"])
            self.assertEqual(actual["a"], source["a"])

    def test_helper_svg_bindings_resolve_to_current_tokens(self):
        result = subprocess.run([sys.executable, "-B", str(ROOT / "scripts/sync_figma_icon_helpers.py"), "--check"],
                                cwd=ROOT, text=True, capture_output=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        bindings = json.loads((ROOT / "resources/figma-icon-token-bindings.json").read_text())
        self.assertEqual(len(bindings["icons"]), 6)
        for icon in bindings["icons"]:
            for binding in icon["bindings"]:
                self.assertEqual(binding["resolved"], color_hex(self.tokens[binding["token"]]["value"]))

    def test_icon_asset_uses_current_figma_token(self):
        manifest = json.loads((ROOT / "docs/figma-iconset-manifest.json").read_text())
        icons = {item["name"]: item for item in manifest["icons"]}
        blue = self.tokens["Accent/Blue/Hover"]
        for name in ("generalprojectStructure", "database"):
            item = icons[name]
            evidence = item["colorReadback"]
            self.assertTrue(any(binding[4] == blue["id"] for binding in evidence["tokenBindings"]), name)
            svg = (ROOT / "resources/iconset" / item["filename"]).read_text()
            self.assertIn(color_hex(blue["value"]), svg, name)
            self.assertNotIn("#1582FF", svg, name)

    def test_readback_covers_all_icons_and_references_known_tokens(self):
        manifest = json.loads((ROOT / "docs/figma-iconset-manifest.json").read_text())
        coverage = manifest["colorReadbackCoverage"]
        self.assertEqual((coverage["captured"], coverage["verified"], coverage["notCaptured"]), (2846, 2846, 0))
        self.assertEqual(coverage["unresolvedNodeIds"], [])
        self.assertGreater(coverage["changedThisReadback"], 0)
        known_ids = {token["id"] for token in self.tokens.values()}
        with_bindings = 0
        for icon in manifest["icons"]:
            readback = icon["colorReadback"]
            self.assertEqual(readback["capturedOn"], "2026-10-31")
            bindings = readback.get("tokenBindings", [])
            with_bindings += bool(bindings)
            for binding in bindings:
                self.assertIn(binding[4], known_ids)
        self.assertEqual(with_bindings, 2788)


if __name__ == "__main__":
    unittest.main()

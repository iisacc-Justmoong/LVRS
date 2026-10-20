"""Check authored Figma precision, compatibility and embedded SVG publication."""
import hashlib
import json
from pathlib import Path
import re
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PLAN = json.loads((ROOT / 'tests/fixtures/accent-palette.json').read_text())


class AccentPaletteTests(unittest.TestCase):
    def test_authored_precision_and_retained_names(self):
        source = (ROOT / 'src/qml/Theme.qml').read_text()
        self.assertEqual(len(PLAN['colors']), 10)
        self.assertEqual(len({r['nodeId'] for r in PLAN['colors']}), 10)
        for row in PLAN['colors']:
            with self.subTest(property=row['property']):
                match = re.search(r'property color ' + row['property'] + r': Qt\.rgba\(([^)]+)\)', source)
                self.assertIsNotNone(match, 'Use original RGBA, not rounded HEX')
                actual = [float(x.strip()) for x in match[1].split(',')]
                self.assertEqual(actual, [row['rgba'][k] for k in ('r', 'g', 'b', 'a')])
        for legacy, target in PLAN['legacyAliases'].items():
            self.assertRegex(source, r'property color ' + legacy + ': ' + target + r'\b')
        self.assertRegex(source, r'property color defaultPrimary: accentBlue\b')

    def test_retired_accent_paints_are_absent(self):
        retired = set(PLAN['replacements']) | set(PLAN['derivedReplacements'])
        for path in (ROOT / 'resources').rglob('*.svg'):
            with self.subTest(icon=str(path.relative_to(ROOT / 'resources'))):
                colors = set(re.findall(r'#[0-9a-fA-F]{6}\b', path.read_text()))
                self.assertFalse({c.upper() for c in colors} & retired)

    def test_icon_manifest_and_geometry_are_verified(self):
        manifest = json.loads((ROOT / 'docs/figma-iconset-manifest.json').read_text())
        self.assertEqual(manifest['count'], len(manifest['icons']))
        self.assertEqual(manifest['accentPaletteUpdate']['sourceNodes'], [r['nodeId'] for r in PLAN['colors']])
        resources = [dict(icon, path='iconset/' + icon['filename']) for icon in manifest['icons']]
        resources += manifest['accentPaletteUpdate'].get('additionalResources', [])
        for icon in resources:
            path = ROOT / 'resources' / icon['path']
            payload = path.read_bytes()
            self.assertEqual(hashlib.sha256(payload).hexdigest(), icon['sha256'], icon['path'])
            if 'paletteGeometrySha256' in icon:
                node = ET.fromstring(payload)
                for part in node.iter():
                    for key in ('fill', 'stroke', 'stop-color', 'flood-color', 'style'):
                        part.attrib.pop(key, None)
                actual = hashlib.sha256(ET.tostring(node)).hexdigest()
                self.assertEqual(actual, icon['paletteGeometrySha256'], icon['path'])


if __name__ == '__main__':
    unittest.main()

"""Cross-host source inventory paths and the generated native parity ledger."""
from pathlib import Path, PurePosixPath, PureWindowsPath
import json
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeParityPaths(unittest.TestCase):
    def test_inventory_paths_are_host_independent(self):
        inventory = json.loads((ROOT / 'catalog/native/parity.json').read_text(encoding='utf-8'))
        for row in inventory['native_api_files']:
            name = row['path']
            with self.subTest(path=name):
                self.assertNotIn('\\', name)
                self.assertFalse(PurePosixPath(name).is_absolute())
                self.assertEqual(PureWindowsPath(name).as_posix(), name)
                self.assertTrue((ROOT / name).is_file())

    def test_inventory_order_uses_serialized_paths(self):
        inventory = json.loads((ROOT / 'catalog/native/parity.json').read_text(encoding='utf-8'))
        names = [row['path'] for row in inventory['native_api_files']]
        self.assertEqual(names, sorted(names))
        self.assertEqual(len(names), len(set(names)))

    def test_generated_ledger_matches_every_source_hash(self):
        result = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/native_parity.py'), '--check'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()

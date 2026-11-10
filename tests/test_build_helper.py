"""Exercise the build helper without configuring or executing applications."""
from pathlib import Path
import os
import subprocess
import unittest
import uuid

import ii_file_provider as provider

ROOT = Path(__file__).resolve().parents[1]


class BuildHelperTest(unittest.TestCase):
    def setUp(self):
        self.bin = Path.cwd() / ("commands-" + uuid.uuid4().hex[:8])
        provider.mkdir(self.bin, exist_ok=False)
        self.cmake = self.bin / "cmake"
        provider.write_text(self.cmake, "#!/bin/sh\nprintf 'CMAKE'; printf ' <%s>' \"$@\"; printf '\\n'\n")
        self.cmake.chmod(0o755)
        self.env = dict(os.environ, PATH=str(self.bin) + os.pathsep + os.environ["PATH"])

    def invoke(self, *args):
        return subprocess.run([str(ROOT / "build.sh"), *args], env=self.env,
                              text=True, capture_output=True, check=False)

    def test_defaults_and_test_disable(self):
        default = self.invoke()
        self.assertEqual(default.returncode, 0, default.stderr)
        self.assertIn("<-DLVRS_BUILD_EXAMPLES=OFF>", default.stdout)
        self.assertIn("<-DLVRS_BUILD_TESTS=ON>", default.stdout)
        self.assertIn("<-DBUILD_TESTING=ON>", default.stdout)
        without = self.invoke("--without-tests")
        self.assertEqual(without.returncode, 0, without.stderr)
        self.assertIn("<-DLVRS_BUILD_TESTS=OFF>", without.stdout)
        self.assertIn("<-DBUILD_TESTING=OFF>", without.stdout)

    def test_rejects_alternate_build_and_conflicting_cmake_flags(self):
        self.assertNotEqual(self.invoke("--build-dir", "build-proof").returncode, 0)
        self.assertNotEqual(self.invoke("--", "-DBUILD_TESTING=ON").returncode, 0)

    def test_clean_delegates_to_build_system(self):
        if not (ROOT / "build/CMakeCache.txt").is_file():
            self.skipTest("Clean requires an already configured build")
        result = self.invoke("--clean")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("<--target> <clean>", result.stdout)
        self.assertTrue((ROOT / "build/CMakeCache.txt").is_file())


if __name__ == "__main__":
    unittest.main()

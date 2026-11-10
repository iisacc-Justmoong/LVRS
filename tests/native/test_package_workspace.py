"""A package test may not overwrite another build or the project manifest."""
from pathlib import Path
import unittest
import package_workspace as workspace
import ii_file_provider as provider

class PackageWorkspaceTest(unittest.TestCase):
    def test_manifest_redirect_requires_exact_paths(self):
        build, owner = Path("/project/build"), Path("/project/resources/.t/abcd")
        text = str(build / "install_local_manifest.txt") + "\n" + str(build / "${CMAKE_INSTALL_MANIFEST}")
        self.assertNotIn(str(build), workspace.redirect_manifest(text, build, owner))
        with self.assertRaises(ValueError): workspace.redirect_manifest(text + "\n" + text, build, owner)

    def test_collision_does_not_remove_existing_directory(self):
        owner = Path.cwd() / "package-policy"
        provider.mkdir(owner, exist_ok=False)
        try:
            with workspace.consumer_build(owner, "abcd", "abi") as existing:
                with self.assertRaises(FileExistsError):
                    with workspace.consumer_build(owner, "abcd", "abi"): pass
                self.assertTrue(existing.is_dir())
            self.assertFalse(existing.exists())
            with self.assertRaises(ValueError):
                with workspace.consumer_build(owner, "../escape", "abi"): pass
        finally:
            owner.rmdir()

    def test_absolute_or_escaping_install_paths_are_rejected(self):
        for value in ("/elsewhere", "../escape", "C:\\outside"):
            with self.assertRaises(ValueError):
                workspace.validate_install_dirs("CMAKE_INSTALL_LIBDIR:PATH=" + value)
        workspace.validate_install_dirs("CMAKE_INSTALL_LIBDIR:PATH=lib")

if __name__ == "__main__":
    unittest.main()

"""A same-version, stale widget ABI must fail before application compilation."""
from pathlib import Path
import subprocess
import sys
import ii_file_provider as provider
from package_workspace import checked_paths, consumer_build

source = Path(__file__).resolve().parents[2]
config, expected = Path(sys.argv[1]), sys.argv[2]
build, root = checked_paths(source, sys.argv[3], sys.argv[4])
if not config.resolve(strict=True).is_relative_to(build):
    raise ValueError("Package configuration must belong to the selected build")
package = root / "package"
provider.mkdir(package, exist_ok=False)
provider.write_text(package / "LVRSConfig.cmake", config.read_text())
provider.write_text(package / "LVRSTargets.cmake", "")
for name, version in (("iiGUI", "0.4.1"), ("iiSDKCore", "0.20.0"),
                      ("iiFileProviderNative", "0.9.0"), ("iiJson", "0.1.1"),
                      ("iiNetwork", "0.5.0"), ("iiVisualCodec", "0.2.0")):
    directory = root / name
    provider.mkdir(directory, exist_ok=False)
    provider.write_text(directory / (name + "ConfigVersion.cmake"),
                        'set(PACKAGE_VERSION "' + version + '")\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n')
    if name != "iiGUI": provider.write_text(directory / (name + "Config.cmake"), "")
project = root / "source"
provider.mkdir(project, exist_ok=False)
provider.write_text(project / "FindZLIB.cmake", "set(ZLIB_FOUND TRUE)\nadd_library(ZLIB::ZLIB INTERFACE IMPORTED)\n")
provider.write_text(project / "CMakeLists.txt", 'cmake_minimum_required(VERSION 3.31)\nproject(ABIProbe NONE)\n'
                    'list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")\nfind_package(LVRS CONFIG REQUIRED)\n')
with consumer_build(build, root.name, "abi") as binaries:
    for index, identity in enumerate((expected, "stale-widget-layout", "")):
        provider.write_text(root / "iiGUI/iiGUIConfig.cmake",
                            'add_library(iiGUI::iiGUI INTERFACE IMPORTED)\nset(iiGUI_NATIVE_ABI_ID "' + identity + '")\n')
        result = subprocess.run(["cmake", "-S", str(project), "-B", str(binaries / str(index)),
            "-DLVRS_DIR=" + str(package), "-DiiGUI_DIR=" + str(root / "iiGUI"),
            "-DiiSDKCore_DIR=" + str(root / "iiSDKCore"), "-DiiFileProviderNative_DIR=" + str(root / "iiFileProviderNative"),
            "-DiiJson_DIR=" + str(root / "iiJson"), "-DiiNetwork_DIR=" + str(root / "iiNetwork"),
            "-DiiVisualCodec_DIR=" + str(root / "iiVisualCodec")],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if index == 0 and result.returncode: raise SystemExit(result.stdout)
        if index and (not result.returncode or "native ABI mismatch" not in result.stdout):
            raise SystemExit("A stale same-version widget ABI was not rejected\n" + result.stdout)
print("NATIVE_ABI_OK matching=true stale_rejected=true missing_rejected=true")

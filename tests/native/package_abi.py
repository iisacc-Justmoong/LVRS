"""A same-version, stale widget ABI must fail before application compilation."""
import pathlib
import subprocess
import sys
import tempfile

config=pathlib.Path(sys.argv[1])
expected=sys.argv[2]
build=pathlib.Path(sys.argv[3])
build.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(dir=build) as temporary:
    root=pathlib.Path(temporary)
    package=root/'package'
    package.mkdir()
    (package/'LVRSConfig.cmake').write_text(config.read_text())
    (package/'LVRSTargets.cmake').write_text('')
    for name,version in [('iiGUI','0.4.1'),('iiSDKCore','0.20.0'),('iiFileProviderNative','0.9.0'),('iiJson','0.1.1'),('iiNetwork','0.5.0'),('iiVisualCodec','0.2.0')]:
        directory=root/name
        directory.mkdir()
        (directory/(name+'ConfigVersion.cmake')).write_text('set(PACKAGE_VERSION "'+version+'")\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n')
        if name!='iiGUI':(directory/(name+'Config.cmake')).write_text('')
    source=root/'source'
    source.mkdir()
    (source/'FindZLIB.cmake').write_text('set(ZLIB_FOUND TRUE)\nadd_library(ZLIB::ZLIB INTERFACE IMPORTED)\n')
    (source/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.31)\nproject(ABIProbe NONE)\nlist(PREPEND CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}\")\nfind_package(LVRS CONFIG REQUIRED)\n')
    for index,identity in enumerate([expected,'stale-widget-layout','']):
        (root/'iiGUI/iiGUIConfig.cmake').write_text('add_library(iiGUI::iiGUI INTERFACE IMPORTED)\nset(iiGUI_NATIVE_ABI_ID "'+identity+'")\n')
        result=subprocess.run(['cmake','-S',str(source),'-B',str(root/('case-'+str(index))),'-DLVRS_DIR='+str(package),'-DiiGUI_DIR='+str(root/'iiGUI'),'-DiiSDKCore_DIR='+str(root/'iiSDKCore'),'-DiiFileProviderNative_DIR='+str(root/'iiFileProviderNative'),'-DiiJson_DIR='+str(root/'iiJson'),'-DiiNetwork_DIR='+str(root/'iiNetwork'),'-DiiVisualCodec_DIR='+str(root/'iiVisualCodec')],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        if index==0 and result.returncode:
            raise SystemExit(result.stdout)
        if index and (not result.returncode or 'native ABI mismatch' not in result.stdout):
            raise SystemExit('A stale same-version widget ABI was not rejected\n'+result.stdout)
print('NATIVE_ABI_OK matching=true stale_rejected=true missing_rejected=true')

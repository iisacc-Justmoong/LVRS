# Build and resource layout

Windows SDK의 `min`·`max` 매크로가 켜진 상태에서도 계층 모델의 인덱스
상수를 선언할 수 있도록 표준 극한값 호출을 보호한다. 네이티브 테스트 빌드에는
해당 공개 헤더의 컴파일 전용 검사가 포함된다. 이번 변경의 Windows 빌드와
설치는 수행하지 않았다.

The supported CMake build directory is the repository's `build/`. The default
native and legacy configurations compile the module without examples. The
root `build.sh` adds tests by default; `--without-tests` disables them, and
`--with-examples` explicitly opts into example application targets. `--clean`
uses the configured build system's clean target and does not recursively erase
unrecognized files.

Test fixtures, temporary package installations and consumer sources belong to
the project `resources/` directory. The native package tests reserve isolated
short-lived directories in `resources/.t`, with consumer compiler output
confined to uniquely owned directories inside `build/`. They clean these
directories after use and leave the regular install manifest intact. Existing
user data and reference materials recovered from older builds are retained
under ignored resource directories; they are not inputs to the default build.

`LVRS.native_presentation_package_build` validates a relocated installed package
through compilation without starting a GUI application. The separate
`LVRS.native_presentation_package` additionally runs the installed consumer.
Installation into the user's SDK and application execution are separate actions.

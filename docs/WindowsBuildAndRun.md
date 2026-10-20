# LVRS: Windows 빌드 및 실행

Qt Quick 기반 iiSDK GUI 프레임워크이다. QML-only 소비자를 포함한 DLL·QML 등록을 제공한다.

검증 기록 갱신: `2026-10-20T02:45:37+09:00`. 변경 전 소스 기준 commit: `48b547564ea199277d9dbb6a6992a4200f3d66c9`. 아래 결과는 이번 변경을 포함한 작업 트리의 검증이다.

## 현재 검증 상태

Release 빌드·실행 검사·설치를 완료하였다. 등록 검사 59개 중 59개 통과, 0개 실패, 0개 skip이다.

## 이 프로젝트의 의존성과 실행 범위

선행 workspace SDK는 다음과 같다. 각각의 Windows 지침에 따라 실행 검증과 설치를 완료한 뒤 진행한다.

- [iiFileProvider](../../../SDK/iiFileProvider/docs/WindowsBuildAndRun.md)

- iiFileProvider와 Qt Quick/QuickControls2/Qml/Svg/Network/Test 6.8.3 및 Vulkan 개발 패키지를 사용한다.
- 중첩 CMake 검사에는 Ninja와 Git for Windows의 `sh`가 필요하다.

프로젝트별 Windows 조건은 다음과 같다.

- Qt resource URL의 대소문자는 Windows에서도 구분된다. javascript/nodestest 아이콘은 실제 소스 파일명의 대소문자와 별개로 Figma manifest의 정규 URL을 제공한다. SVG 회귀 검사는 manifest 전체의 경로·hash·실제 렌더링을 확인한다.
- 중첩 QVariantList fixture는 각 행을 QVariant로 감싸 MSVC가 내부 QList의 복사 생성자를 선택하여 행을 평탄화하지 않도록 한다.
- Label은 hinting 없는 글꼴 설계 치수와 올림한 논리 너비를 사용하여 Windows의 native text renderer에서도 Figma 레이아웃을 유지한다. native/Qt text rendering 전환 뒤 너비가 유지되는지 검사한다. [Qt font 문서](https://doc.qt.io/qt-6.8/qml-font.html)를 기준으로 한 설정이다.
- MSVC 자동 DLL export 스캐너는 일반 COFF object를 요구하므로 이 구성에서 IPO를 끈다.
- 네이티브 hover 검사는 노출뿐 아니라 활성화까지 기다린 뒤 바깥에서 안쪽으로 포인터를 이동한다. Windows의 비활성 창이나 이전 포인터 위치에 의존하여 hover 검사를 누락하지 않도록 한다.
- QML-only 소비자는 native resource initialization symbol을 유지해야 한다. 현재 `LVRSTests_windows_qml_link`가 이를 검증한다.
- 화면·visual 비교 fixture의 재현성을 위해 테스트에 basic render loop를 적용한다. 실제 앱의 모든 렌더링 기능을 검증한 것으로 해석하지 않는다.

## 환경 준비

PowerShell에서 이 프로젝트 루트로 이동한 뒤 순서대로 실행한다. workspace 배치는 `Workspace/SDK/<이름>`을 전제로 한다.

검증 도구는 MSVC 19.51 x64, Windows SDK 10.0.28000, CMake 4.4.4, Ninja 1.13.2이다. 개발 shell은 빌드 재현을 위해 Python 3.14.8을 선택하며 시스템 기본 Python 3.15.0과 구분한다.

```powershell
$ErrorActionPreference = 'Stop'
$project = (Get-Location).Path
$workspace = (Resolve-Path -LiteralPath '../..').Path
. (Join-Path $workspace 'SDK/Windows/Enter-DevEnvironment.ps1')
Set-Location -LiteralPath $project
$python = (Join-Path $workspace 'Tools/Python314/tools/python.exe').Replace('\', '/')
$compiler = (Get-Command cl.exe -ErrorAction Stop).Source.Replace('\', '/')
$toolchain = (Join-Path $workspace 'Tools/vcpkg/scripts/buildsystems/vcpkg.cmake').Replace('\', '/')
$msvcInclude = (Join-Path $workspace 'SDK/Windows/WindowsMsvcPrefix.cmake').Replace('\', '/')
$prefix = (Join-Path $env:USERPROFILE '.local/SDK/LVRS').Replace('\', '/')
```

`Enter-DevEnvironment.ps1`은 Qt MSVC kit, vcpkg, 설치된 SDK의 검색 경로와 UTF-8 설정을 준비한다. `CMAKE_PREFIX_PATH`의 디렉터리 존재만으로 의존성 검증을 대신하지 않는다. 선행 SDK의 실행 검사와 설치를 먼저 완료해야 한다.

## 구성과 빌드

```powershell
$configure = @(
    '-S', '.', '-B', 'build', '-G', 'Ninja'
    '-DCMAKE_BUILD_TYPE=Release'
    "-DCMAKE_C_COMPILER=$compiler"
    "-DCMAKE_CXX_COMPILER=$compiler"
    "-DCMAKE_INSTALL_PREFIX=$prefix"
    "-DCMAKE_PREFIX_PATH=$env:CMAKE_PREFIX_PATH"
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
    '-DVCPKG_TARGET_TRIPLET=x64-windows'
    "-DCMAKE_PROJECT_INCLUDE=$msvcInclude"
    "-DPython3_EXECUTABLE=$python"
    "-DPython_EXECUTABLE=$python"
    '-DQt6_DIR=C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6'
    '-DBUILD_TESTING=ON'
    '-DLVRS_BUILD_TESTS=ON'
    '-DLVRS_BUILD_EXAMPLES=ON'
    '-DLVRS_BUILD_LEGACY_EXAMPLES=ON'
    '-DLVRS_INSTALL_EXAMPLES=ON'
    '-DLVRS_ENABLE_IPO=OFF'
)
& cmake @configure
if ($LASTEXITCODE -ne 0) { throw 'CMake 구성 실패' }
$cache = Get-Content -LiteralPath 'build/CMakeCache.txt' -Raw
if ($cache -notmatch '(?m)^CMAKE_BUILD_TYPE:STRING=Release\r?$') {
    throw 'Release 구성이 유실되었음. 같은 구성 인수에 --fresh를 추가하여 다시 구성할 것'
}
& $python (Join-Path $workspace 'SDK/Windows/Configure-WindowsNinja.py') (Join-Path $project 'build')
if ($LASTEXITCODE -ne 0) { throw 'Ninja MSVC 의존성 설정 실패' }
cmake --build build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw '빌드 실패' }
```

모든 생성 파일은 이 프로젝트의 `build/` 아래에 둔다. 컴파일러는 위에서 확인한 절대 경로를 사용한다. 기존 캐시가 다른 compiler 또는 generator를 사용한다면 같은 구성 인수에 `--fresh`를 추가하여 다시 구성한다. MSVC 키트와 MinGW 또는 Debug/Release 라이브러리를 혼합하지 않는다.

`WindowsMsvcPrefix.cmake`와 `Configure-WindowsNinja.py`는 MSVC include 출력 언어를 Ninja에 맞추고 UTF-8 실행 manifest를 적용한다. 빌드 병렬도는 4로 제한하며 실행 검사는 직렬로 수행한다.

## 실행과 확인

17종 목록 캡처는 모니터 높이에 따라 Windows가 조정하는 창 크기 대신 QML 루트의 논리 크기를 사용한다. 1x 화면의 1px inset highlight는 픽셀 셀의 내림 좌표에서 검사한다. 같은 검사를 고해상도 화면에도 유지하며 GPU 효과를 비활성화하여 통과시키지 않는다.

공통 PNG 기준을 비교하는 `LVRSTests_visual_regression`만 고정된 software rasterizer를 사용한다. 기존 golden PNG와 0.1%/채널 8 허용치는 유지한다. 비교기 자체는 색상 변경과 1px 위치 변경을 검출하는 별도 검사를 포함한다. Figma 재질·backdrop·목록 등 실제 컴포넌트 검사는 Windows의 네이티브 RHI에서 계속 실행한다.

Windows DLL export·QML-only 링크, UI 계약, navigation, SVG와 visual regression을 검증한다.

```powershell
$dllDirectories = @(Get-ChildItem -LiteralPath build -Recurse -Filter '*.dll' -File |
    Select-Object -ExpandProperty DirectoryName | Sort-Object -Unique)
$env:PATH = (@($dllDirectories) + @($env:PATH)) -join ';'
$env:QSG_RENDER_LOOP = 'basic'
ctest --test-dir build -C Release --output-on-failure --parallel 1 --no-tests=error
if ($LASTEXITCODE -ne 0) { throw '전체 실행 검사 실패' }
```

현재 빌드에 실제 등록된 실행 경로의 예는 `LVRSTests_materials`, `LVRSTests_windows_exports`, `LVRSTests_windows_qml_link`이다. 전체 목록은 `ctest --test-dir build -N`으로 확인한다.

## 설치와 배포 범위

전체 테스트가 통과한 구성만 설치한다. skip된 선택 기능은 지원·실행 범위를 별도 기록한다.

```powershell
cmake --install build --config Release --prefix "$prefix"
if ($LASTEXITCODE -ne 0) { throw '설치 실패' }
```

설치 파일은 `build/install_manifest.txt`와 실제 prefix의 파일을 대조한다. 소비자 실행 시 prefix의 `bin`과 필요한 vcpkg DLL을 검색할 수 있어야 한다.

## 검증 근거

- 빌드 정의: [CMakeLists.txt](../CMakeLists.txt)와 프로젝트 테스트 등록이다.
- 이번 실행 결과: [Windows SDK 전체 검증 기록](https://github.com/iisacc-Justmoong/SDK/blob/main/Windows/Verification-20261020.md)이다.
- 프로젝트별 실행 로그와 JUnit은 같은 검증 디렉터리에 보관한다. 로그의 종료 코드, 실패·skip 목록과 실제 실행 범위를 함께 확인한다.

전체 SDK의 실행·설치와 커밋 조건은 [공통 Windows 검증 도구](https://github.com/iisacc-Justmoong/SDK/blob/main/Windows/README.md)에서 재현한다. 원본 JSON·JUnit·상세 로그는 로컬 `Workspace/build/maintenance/20261019-221920-sdk-recovery/final/`에 보관하며 Git에는 포함하지 않는다. 이 프로젝트의 성공은 제품·다른 플랫폼·별도 계정 및 물리 장치의 실연동을 대신하지 않는다.

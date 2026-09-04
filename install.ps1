#requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ManifestPath = Join-Path $ScriptDir "rust-cli\Cargo.toml"

function Find-CommandPath {
    param([Parameter(Mandatory = $true)][string]$Name)

    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $command) {
        return $null
    }

    return $command.Source
}

function Prepend-PathEntry {
    param([Parameter(Mandatory = $true)][string]$PathEntry)

    if ([string]::IsNullOrWhiteSpace($PathEntry) -or -not (Test-Path -LiteralPath $PathEntry)) {
        return
    }

    $entries = @()
    if (-not [string]::IsNullOrWhiteSpace($env:PATH)) {
        $entries = $env:PATH -split ";"
    }

    foreach ($entry in $entries) {
        if ($entry -ieq $PathEntry) {
            return
        }
    }

    $env:PATH = "$PathEntry;$env:PATH"
}

function Test-QtPrefix {
    param([Parameter(Mandatory = $true)][string]$Prefix)

    if ([string]::IsNullOrWhiteSpace($Prefix)) {
        return $false
    }

    $qtConfig = Join-Path $Prefix "lib\cmake\Qt6\Qt6Config.cmake"
    $qtPaths = Join-Path $Prefix "bin\qtpaths.exe"
    return (Test-Path -LiteralPath $qtConfig) -or (Test-Path -LiteralPath $qtPaths)
}

function Resolve-QtPrefixFromQt6Dir {
    param([Parameter(Mandatory = $true)][string]$Qt6Dir)

    if ([string]::IsNullOrWhiteSpace($Qt6Dir)) {
        return $null
    }

    $candidate = $Qt6Dir
    if ((Split-Path -Leaf $candidate) -ieq "Qt6Config.cmake") {
        $candidate = Split-Path -Parent $candidate
    }

    if (-not (Test-Path -LiteralPath (Join-Path $candidate "Qt6Config.cmake"))) {
        return $null
    }

    return (Resolve-Path -LiteralPath (Join-Path $candidate "..\..\..")).Path
}

function Resolve-QtPrefix {
    $candidates = New-Object System.Collections.Generic.List[string]

    foreach ($name in @(
        "LVRS_BOOTSTRAP_QT_PREFIX_WINDOWS",
        "LVRS_BOOTSTRAP_QT_PREFIX",
        "QT_HOST_PREFIX",
        "QTDIR"
    )) {
        $value = [Environment]::GetEnvironmentVariable($name)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $candidates.Add($value)
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($env:Qt6_DIR)) {
        $prefix = Resolve-QtPrefixFromQt6Dir -Qt6Dir $env:Qt6_DIR
        if ($null -ne $prefix) {
            $candidates.Add($prefix)
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($env:CMAKE_PREFIX_PATH)) {
        foreach ($entry in ($env:CMAKE_PREFIX_PATH -split ";")) {
            if (-not [string]::IsNullOrWhiteSpace($entry)) {
                $candidates.Add($entry)
            }
        }
    }

    foreach ($known in @(
        "C:\Qt\6.8.3\mingw_64",
        "C:\Qt\6.8.3\llvm-mingw_64"
    )) {
        $candidates.Add($known)
    }

    if (Test-Path -LiteralPath "C:\Qt") {
        Get-ChildItem -LiteralPath "C:\Qt" -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object {
                foreach ($kit in @("mingw_64", "llvm-mingw_64", "msvc2022_64")) {
                    $candidates.Add((Join-Path $_.FullName $kit))
                }
            }
    }

    foreach ($candidate in $candidates) {
        if (Test-QtPrefix -Prefix $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Resolve-NinjaPath {
    if (-not [string]::IsNullOrWhiteSpace($env:CMAKE_MAKE_PROGRAM) -and
        (Test-Path -LiteralPath $env:CMAKE_MAKE_PROGRAM)) {
        return (Resolve-Path -LiteralPath $env:CMAKE_MAKE_PROGRAM).Path
    }

    foreach ($candidate in @(
        "C:\Users\Windows\AppData\Local\Programs\CLion\bin\ninja\win\x64\ninja.exe",
        "C:\Program Files\JetBrains\CLion\bin\ninja\win\x64\ninja.exe"
    )) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return Find-CommandPath -Name "ninja.exe"
}

function Resolve-CMakePath {
    foreach ($candidate in @(
        "C:\Users\Windows\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe",
        "C:\Program Files\JetBrains\CLion\bin\cmake\win\x64\bin\cmake.exe"
    )) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return Find-CommandPath -Name "cmake.exe"
}

function Resolve-CxxCompiler {
    if (-not [string]::IsNullOrWhiteSpace($env:CXX) -and (Test-Path -LiteralPath $env:CXX)) {
        return (Resolve-Path -LiteralPath $env:CXX).Path
    }

    foreach ($candidate in @(
        "C:\Users\Windows\AppData\Local\Programs\CLion\bin\mingw\bin\g++.exe",
        "C:\Qt\Tools\mingw1310_64\bin\g++.exe",
        "C:\Qt\Tools\llvm-mingw1706_64\bin\g++.exe"
    )) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return Find-CommandPath -Name "g++.exe"
}

function Contains-Option {
    param(
        [string[]]$Values = @(),
        [Parameter(Mandatory = $true)][string]$Name
    )

    foreach ($value in $Values) {
        if ($value -eq $Name -or $value.StartsWith("$Name=", [System.StringComparison]::Ordinal)) {
            return $true
        }
    }
    return $false
}

function Get-OptionValue {
    param(
        [string[]]$Values = @(),
        [Parameter(Mandatory = $true)][string]$Name
    )

    for ($index = 0; $index -lt $Values.Length; $index++) {
        $value = $Values[$index]
        if ($value.StartsWith("$Name=", [System.StringComparison]::Ordinal)) {
            return $value.Substring($Name.Length + 1)
        }
        if ($value -eq $Name -and ($index + 1) -lt $Values.Length) {
            return $Values[$index + 1]
        }
    }

    return $null
}

function Get-CMakePassthroughArgs {
    param([string[]]$Values = @())

    for ($index = 0; $index -lt $Values.Length; $index++) {
        if ($Values[$index] -eq "--") {
            if (($index + 1) -lt $Values.Length) {
                return $Values[($index + 1)..($Values.Length - 1)]
            }
            return @()
        }
    }

    return @()
}

function Remove-LvrsPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

function ConvertTo-CMakePath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return $Path.Replace("\", "/")
}

function Invoke-DirectInstall {
    param(
        [Parameter(Mandatory = $true)][string[]]$InstallArgs,
        [Parameter(Mandatory = $true)][string]$CMakePath,
        [Parameter(Mandatory = $true)][string]$QtPrefix,
        [Parameter(Mandatory = $true)][string]$NinjaPath,
        [Parameter(Mandatory = $true)][string]$CxxCompiler
    )

    $homeDir = if (-not [string]::IsNullOrWhiteSpace($env:USERPROFILE)) {
        $env:USERPROFILE
    } elseif (-not [string]::IsNullOrWhiteSpace($env:HOME)) {
        $env:HOME
    } else {
        throw "[LVRS] USERPROFILE/HOME environment variable is required."
    }

    $prefix = Get-OptionValue -Values $InstallArgs -Name "--prefix"
    if ([string]::IsNullOrWhiteSpace($prefix)) {
        $prefix = $env:LVRS_INSTALL_PREFIX
    }
    if ([string]::IsNullOrWhiteSpace($prefix)) {
        $prefix = Join-Path $homeDir ".local\SDK\LVRS"
    }
    if (-not [System.IO.Path]::IsPathRooted($prefix)) {
        $prefix = Join-Path (Get-Location).Path $prefix
    }

    $buildType = Get-OptionValue -Values $InstallArgs -Name "--build-type"
    if ([string]::IsNullOrWhiteSpace($buildType)) {
        $buildType = $env:CMAKE_BUILD_TYPE
    }
    if ([string]::IsNullOrWhiteSpace($buildType)) {
        $buildType = "Release"
    }

    $platforms = Get-OptionValue -Values $InstallArgs -Name "--platforms"
    if ([string]::IsNullOrWhiteSpace($platforms)) {
        $platforms = $env:LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS
    }
    if ([string]::IsNullOrWhiteSpace($platforms)) {
        $platforms = "windows"
    }
    $platforms = $platforms.Replace(",", ";")

    $buildDir = Join-Path $ScriptDir "build"
    $platformInstallRoot = Join-Path $prefix "platforms"
    $windowsInstallPrefix = Join-Path $platformInstallRoot "windows"

    Write-Host "[LVRS] Project root : $ScriptDir"
    Write-Host "[LVRS] Build dir    : $buildDir"
    Write-Host "[LVRS] Install dir  : $prefix"
    Write-Host "[LVRS] Windows dir  : $windowsInstallPrefix"
    $scriptDirCMake = ConvertTo-CMakePath -Path $ScriptDir
    $buildDirCMake = ConvertTo-CMakePath -Path $buildDir
    $prefixCMake = ConvertTo-CMakePath -Path $prefix
    $platformInstallRootCMake = ConvertTo-CMakePath -Path $platformInstallRoot
    $qtPrefixCMake = ConvertTo-CMakePath -Path $QtPrefix
    $ninjaPathCMake = ConvertTo-CMakePath -Path $NinjaPath
    $cxxCompilerCMake = ConvertTo-CMakePath -Path $CxxCompiler

    Write-Host "[LVRS] Qt prefix    : $QtPrefix"
    Write-Host "[LVRS] Build type   : $buildType"
    Write-Host "[LVRS] Platforms    : $platforms"
    Write-Host "[LVRS] CLI fallback : direct CMake bootstrap"

    Remove-LvrsPath -Path $buildDir
    foreach ($path in @(
        $platformInstallRoot,
        (Join-Path $prefix "include\LVRS"),
        (Join-Path $prefix "lib\cmake\LVRS"),
        (Join-Path $prefix "lib\qt6\qml\LVRS"),
        (Join-Path $prefix "bin\LVRS.dll")
    )) {
        Remove-LvrsPath -Path $path
    }

    $configureArgs = @(
        "-S", $scriptDirCMake,
        "-B", $buildDirCMake,
        "-G", "Ninja",
        "-DCMAKE_MAKE_PROGRAM=$ninjaPathCMake",
        "-DCMAKE_CXX_COMPILER=$cxxCompilerCMake",
        "-DCMAKE_INSTALL_PREFIX=$prefixCMake",
        "-DCMAKE_BUILD_TYPE=$buildType",
        "-DCMAKE_PREFIX_PATH=$qtPrefixCMake",
        "-DQt6_DIR=$qtPrefixCMake/lib/cmake/Qt6",
        "-DLVRS_BUILD_SHARED_LIBS=ON",
        "-DLVRS_BUILD_EXAMPLES=OFF",
        "-DLVRS_BUILD_TESTS=OFF",
        "-DLVRS_BOOTSTRAP_INSTALL_ROOT=$platformInstallRootCMake",
        "-DLVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS=$platforms",
        "-DLVRS_BOOTSTRAP_QT_PREFIX_WINDOWS=$qtPrefixCMake",
        "-DLVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES=OFF",
        "-DLVRS_BOOTSTRAP_LVRS_BUILD_TESTS=OFF",
        "-DLVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS=ON",
        "-DLVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE=ON"
    )
    $configureArgs += Get-CMakePassthroughArgs -Values $InstallArgs

    & $CMakePath @configureArgs
    if ($LASTEXITCODE -ne 0) {
        throw "[LVRS] Configure failed."
    }

    & $CMakePath --build $buildDir --config $buildType --target bootstrap_lvrs_all
    if ($LASTEXITCODE -ne 0) {
        throw "[LVRS] Build/install failed."
    }

    $envHelper = Join-Path $prefix "env.ps1"
    New-Item -ItemType Directory -Force -Path $prefix | Out-Null
    @"
`$env:LVRS_PLATFORMS_ROOT = "$platformInstallRoot"
`$env:LVRS_HOST_PLATFORM = "windows"
`$env:LVRS_HOST_PREFIX = "$windowsInstallPrefix"
`$env:CMAKE_PREFIX_PATH = "$windowsInstallPrefix;`$env:CMAKE_PREFIX_PATH"
`$env:QML2_IMPORT_PATH = "$windowsInstallPrefix\lib\qt6\qml;`$env:QML2_IMPORT_PATH"
"@ | Set-Content -LiteralPath $envHelper -Encoding UTF8

    Write-Host "[LVRS] Install completed."
    Write-Host "[LVRS] CMake package dir : $(Join-Path $windowsInstallPrefix 'lib\cmake\LVRS')"
    Write-Host "[LVRS] Env helper        : $envHelper"
}

if ($env:OS -ne "Windows_NT") {
    throw "[LVRS] install.ps1 is the Windows installer. Use ./install.sh on Unix-like hosts."
}

$qtPrefix = Resolve-QtPrefix
if ($null -eq $qtPrefix) {
    throw "[LVRS] Qt 6 MinGW prefix was not found. Set CMAKE_PREFIX_PATH or LVRS_BOOTSTRAP_QT_PREFIX_WINDOWS."
}

$cmakePath = Resolve-CMakePath
if ($null -eq $cmakePath) {
    throw "[LVRS] cmake.exe was not found. Install CMake or use the CLion bundled CMake."
}

$ninjaPath = Resolve-NinjaPath
if ($null -eq $ninjaPath) {
    throw "[LVRS] ninja.exe was not found. Install Ninja or use the CLion bundled Ninja."
}

$cxxCompiler = Resolve-CxxCompiler
if ($null -eq $cxxCompiler) {
    throw "[LVRS] g++.exe was not found. Install a MinGW toolchain compatible with the selected Qt kit."
}

Prepend-PathEntry -PathEntry (Join-Path $qtPrefix "bin")
Prepend-PathEntry -PathEntry (Split-Path -Parent $cxxCompiler)
Prepend-PathEntry -PathEntry (Split-Path -Parent $ninjaPath)
Prepend-PathEntry -PathEntry (Split-Path -Parent $cmakePath)

$env:LVRS_ROOT = $ScriptDir
$env:CMAKE_GENERATOR = "Ninja"
$env:CMAKE_MAKE_PROGRAM = $ninjaPath
$env:CXX = $cxxCompiler
$qtPrefixForCMake = ConvertTo-CMakePath -Path $qtPrefix
$env:LVRS_BOOTSTRAP_QT_PREFIX_WINDOWS = $qtPrefixForCMake
$env:CMAKE_PREFIX_PATH = if ([string]::IsNullOrWhiteSpace($env:CMAKE_PREFIX_PATH)) {
    $qtPrefixForCMake
} else {
    "$qtPrefixForCMake;$env:CMAKE_PREFIX_PATH"
}

$installArgs = New-Object System.Collections.Generic.List[string]
$installArgs.AddRange([string[]]$args)

if (-not (Contains-Option -Values ([string[]]$installArgs) -Name "--platforms") -and
    [string]::IsNullOrWhiteSpace($env:LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS)) {
    $installArgs.Insert(0, "windows")
    $installArgs.Insert(0, "--platforms")
}

if (-not (Contains-Option -Values ([string[]]$installArgs) -Name "--without-examples")) {
    $installArgs.Add("--without-examples")
}
if (-not (Contains-Option -Values ([string[]]$installArgs) -Name "--without-tests")) {
    $installArgs.Add("--without-tests")
}

$cargo = Find-CommandPath -Name "cargo.exe"
if ($null -ne $cargo -and (Test-Path -LiteralPath $ManifestPath)) {
    if ([string]::IsNullOrWhiteSpace($env:CARGO_TARGET_DIR)) {
        $env:CARGO_TARGET_DIR = Join-Path $ScriptDir "rust-cli\build"
    }
    & $cargo run --manifest-path $ManifestPath --bin lvrs -- install @installArgs
    exit $LASTEXITCODE
}

$lvrs = Find-CommandPath -Name "lvrs.exe"
if ($null -ne $lvrs) {
    & $lvrs install @installArgs
    exit $LASTEXITCODE
}

Invoke-DirectInstall `
    -InstallArgs ([string[]]$installArgs) `
    -CMakePath $cmakePath `
    -QtPrefix $qtPrefix `
    -NinjaPath $ninjaPath `
    -CxxCompiler $cxxCompiler

cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

set(_lvrs_install_script "${LVRS_SOURCE_DIR}/install.ps1")
if(NOT EXISTS "${_lvrs_install_script}")
    message(FATAL_ERROR "Expected Windows install helper at ${_lvrs_install_script}.")
endif()

find_program(_lvrs_powershell powershell)
if(_lvrs_powershell)
    execute_process(
        COMMAND "${_lvrs_powershell}" -NoProfile -NonInteractive -Command
            "$errors=$null;[System.Management.Automation.PSParser]::Tokenize((Get-Content -Raw '${_lvrs_install_script}'),[ref]$errors) > $null;if($errors){$errors | ForEach-Object { Write-Error $_ }; exit 1 }"
        RESULT_VARIABLE _lvrs_syntax_result
        OUTPUT_VARIABLE _lvrs_syntax_stdout
        ERROR_VARIABLE _lvrs_syntax_stderr
    )
    if(NOT _lvrs_syntax_result EQUAL 0)
        message(FATAL_ERROR
            "install.ps1 failed PowerShell syntax validation.\nstdout:\n${_lvrs_syntax_stdout}\nstderr:\n${_lvrs_syntax_stderr}")
    endif()
endif()

file(READ "${_lvrs_install_script}" _lvrs_install_script_content)

foreach(_lvrs_required_snippet IN ITEMS
    ".local\\SDK\\LVRS"
    "CARGO_TARGET_DIR"
    "rust-cli\\build"
    "Resolve-QtPrefix"
    "Resolve-CMakePath"
    "LVRS_BOOTSTRAP_QT_PREFIX_WINDOWS"
    "CMAKE_GENERATOR"
    "CMAKE_MAKE_PROGRAM"
    "--platforms"
    "windows"
    "--without-examples"
    "--without-tests"
    "Invoke-DirectInstall"
    "bootstrap_lvrs_all"
    "cargo run --manifest-path"
    "lvrs install"
)
    string(FIND "${_lvrs_install_script_content}" "${_lvrs_required_snippet}" _lvrs_required_index)
    if(_lvrs_required_index EQUAL -1)
        message(FATAL_ERROR
            "Expected install.ps1 to contain '${_lvrs_required_snippet}'.")
    endif()
endforeach()

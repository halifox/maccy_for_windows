if (NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "The Windows x64 package must be built on Windows.")
endif ()

if (NOT DEFINED MACCY_VERSION OR MACCY_VERSION STREQUAL "")
    set(MACCY_VERSION "1.0.0")
endif ()

if (NOT MACCY_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
            "MACCY_VERSION must use MAJOR.MINOR.PATCH, got: ${MACCY_VERSION}")
endif ()

if (DEFINED VCPKG_ROOT AND NOT VCPKG_ROOT STREQUAL "")
    file(TO_CMAKE_PATH "${VCPKG_ROOT}" _vcpkg_root)
elseif (DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _vcpkg_root)
else ()
    message(FATAL_ERROR
            "Set VCPKG_ROOT or pass -DVCPKG_ROOT=<path> to this command.")
endif ()

if (NOT EXISTS "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
    message(FATAL_ERROR "Invalid VCPKG_ROOT: ${_vcpkg_root}")
endif ()
set(ENV{VCPKG_ROOT} "${_vcpkg_root}")

find_program(_makensis_executable NAMES makensis makensis.exe)
if (NOT _makensis_executable)
    message(FATAL_ERROR
            "NSIS was not found. Install NSIS and add its directory to PATH.")
endif ()
get_filename_component(_nsis_bin_dir "${_makensis_executable}" DIRECTORY)
set(ENV{PATH} "${_nsis_bin_dir};$ENV{PATH}")

if (NOT DEFINED ENV{VSINSTALLDIR} OR "$ENV{VSINSTALLDIR}" STREQUAL "")
    message(FATAL_ERROR
            "Run this command from a Visual Studio Developer PowerShell so VSINSTALLDIR is available.")
endif ()

file(TO_CMAKE_PATH "$ENV{VSINSTALLDIR}" _vs_install_dir)
cmake_path(NORMAL_PATH _vs_install_dir)
cmake_path(APPEND _vs_install_dir "Common7" "Tools" "VsDevCmd.bat"
        OUTPUT_VARIABLE _vs_dev_cmd)
if (NOT EXISTS "${_vs_dev_cmd}")
    message(FATAL_ERROR "Could not find VsDevCmd.bat at ${_vs_dev_cmd}")
endif ()

get_filename_component(_source_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_build_dir "${_source_dir}/build/windows-x64-release-vcpkg")
set(_package_dir "${_source_dir}/build/packages")
file(MAKE_DIRECTORY "${_source_dir}/build/package-driver" "${_package_dir}")

file(TO_NATIVE_PATH "${_vcpkg_root}" _vcpkg_root_native)
file(TO_NATIVE_PATH "${_source_dir}" _source_dir_native)
file(TO_NATIVE_PATH "${_vs_dev_cmd}" _vs_dev_cmd_native)
file(TO_NATIVE_PATH "${CMAKE_COMMAND}" _cmake_executable_native)

set(_driver_file "${_source_dir}/build/package-driver/package-x64.cmd")
set(_driver_content "@echo off\r\nsetlocal\r\n")
string(APPEND _driver_content "set \"VCPKG_ROOT=${_vcpkg_root_native}\"\r\n")
string(APPEND _driver_content "cd /d \"${_source_dir_native}\"\r\n")
string(APPEND _driver_content "if errorlevel 1 exit /b 1\r\n")
string(APPEND _driver_content
        "call \"${_vs_dev_cmd_native}\" -arch=x64 -host_arch=x64\r\n")
string(APPEND _driver_content "if errorlevel 1 exit /b 1\r\n")

set(_commands
        "\"${_cmake_executable_native}\" --fresh --preset windows-x64-release \"-DMACCY_VERSION=${MACCY_VERSION}\""
        "\"${_cmake_executable_native}\" --build --preset windows-x64-release --target maccy --parallel"
)
foreach (_command IN LISTS _commands)
    string(APPEND _driver_content "${_command}\r\n")
    string(APPEND _driver_content "if errorlevel 1 exit /b 1\r\n")
endforeach ()

file(WRITE "${_driver_file}" "${_driver_content}")
execute_process(
        COMMAND "$ENV{COMSPEC}" /D /S /C CALL "${_driver_file}"
        WORKING_DIRECTORY "${_source_dir}"
        RESULT_VARIABLE _build_result
)
file(REMOVE "${_driver_file}")
if (NOT _build_result STREQUAL "0")
    message(FATAL_ERROR "Windows x64 Release build failed: ${_build_result}")
endif ()

get_filename_component(_cmake_bin_dir "${CMAKE_COMMAND}" DIRECTORY)
set(_cpack_executable "${_cmake_bin_dir}/cpack.exe")
if (NOT EXISTS "${_cpack_executable}")
    unset(_cpack_executable)
    find_program(_cpack_executable NAMES cpack cpack.exe)
endif ()
if (NOT _cpack_executable OR NOT EXISTS "${_cpack_executable}")
    message(FATAL_ERROR "Could not find cpack.exe next to CMake or in PATH.")
endif ()

set(_cpack_config "${_build_dir}/CPackConfig.cmake")
if (NOT EXISTS "${_cpack_config}")
    message(FATAL_ERROR "CMake did not create the expected CPack config: ${_cpack_config}")
endif ()

foreach (_generator IN ITEMS NSIS ZIP)
    execute_process(
            COMMAND "${_cpack_executable}" --config "${_cpack_config}"
                    -C Release -G "${_generator}" -B "${_package_dir}"
            WORKING_DIRECTORY "${_source_dir}"
            RESULT_VARIABLE _package_result
    )
    if (NOT _package_result STREQUAL "0")
        message(FATAL_ERROR "CPack ${_generator} packaging failed: ${_package_result}")
    endif ()
endforeach ()

set(_package_base "maccy-${MACCY_VERSION}-win64")
set(_installer "${_package_dir}/${_package_base}.exe")
set(_portable_zip "${_package_dir}/${_package_base}.zip")
if (NOT EXISTS "${_installer}")
    message(FATAL_ERROR "CPack did not create the expected installer: ${_installer}")
endif ()
if (NOT EXISTS "${_portable_zip}")
    message(FATAL_ERROR "CPack did not create the expected ZIP package: ${_portable_zip}")
endif ()

file(SHA256 "${_installer}" _installer_hash)
file(SHA256 "${_portable_zip}" _zip_hash)
file(WRITE "${_package_dir}/SHA256SUMS.txt"
        "${_installer_hash}  ${_package_base}.exe\n${_zip_hash}  ${_package_base}.zip\n")

message(STATUS "Installer: ${_installer}")
message(STATUS "Portable ZIP: ${_portable_zip}")
message(STATUS "SHA-256: ${_package_dir}/SHA256SUMS.txt")

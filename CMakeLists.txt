cmake_minimum_required(VERSION 3.25)

project(app VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(sqlite3 STATIC
        third_party/sqlite/sqlite3.c
)

target_include_directories(sqlite3 PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite"
)

target_compile_definitions(sqlite3 PRIVATE
        SQLITE_THREADSAFE=1
)

add_executable(app WIN32
        src/main.cpp
        src/Database.cpp
        src/Settings.cpp
        src/SettingsWindow.cpp
        src/PreviewWindow.cpp
        src/SettingsWindow.rc
        src/MainWindow.rc
)

set_target_properties(app PROPERTIES
        OUTPUT_NAME "clipboard"
)

target_include_directories(app PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WTL10_01_Release/Include"
)

target_link_libraries(app PRIVATE
        sqlite3
        user32
        gdi32
        shell32
        advapi32
        comdlg32
        comctl32
        windowscodecs
        ole32
)

target_compile_definitions(app PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(app PRIVATE /utf-8)
endif()

include(GNUInstallDirs)

install(TARGETS app
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Include the MSVC runtime DLLs used by the /MD build in the installer.  The
# bundled CMake module currently assumes an older VC redist folder name for
# this toolchain, so locate the matching redist directory from cl.exe instead.
if(MSVC AND WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_app_msvc_arch "x64")
    else()
        set(_app_msvc_arch "x86")
    endif()

    get_filename_component(_app_msvc_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_app_msvc_host_bin_dir "${_app_msvc_bin_dir}" DIRECTORY)
    get_filename_component(_app_msvc_toolset_bin_dir "${_app_msvc_host_bin_dir}" DIRECTORY)
    get_filename_component(_app_msvc_toolset_dir "${_app_msvc_toolset_bin_dir}" DIRECTORY)
    get_filename_component(_app_msvc_tools_dir "${_app_msvc_toolset_dir}" DIRECTORY)
    get_filename_component(_app_vc_tools_dir "${_app_msvc_tools_dir}" DIRECTORY)
    get_filename_component(_app_vc_dir "${_app_vc_tools_dir}" DIRECTORY)

    file(GLOB _app_msvc_runtime_dirs LIST_DIRECTORIES true
         "${_app_vc_dir}/Redist/MSVC/*/${_app_msvc_arch}/Microsoft.VC*.CRT")

    set(_app_msvc_runtime_dir "")
    foreach(_runtime_dir IN LISTS _app_msvc_runtime_dirs)
        if(EXISTS "${_runtime_dir}/vcruntime140.dll")
            set(_app_msvc_runtime_dir "${_runtime_dir}")
            break()
        endif()
    endforeach()

    if(_app_msvc_runtime_dir)
        file(GLOB _app_msvc_runtime_files LIST_DIRECTORIES false
             "${_app_msvc_runtime_dir}/concrt140*.dll"
             "${_app_msvc_runtime_dir}/msvcp140*.dll"
             "${_app_msvc_runtime_dir}/vcruntime140*.dll")
        install(FILES ${_app_msvc_runtime_files}
                DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    else()
        message(WARNING "MSVC runtime redistributable DLLs were not found; the installer will require the x64 VC runtime on the target machine.")
    endif()
endif()

set(CPACK_PACKAGE_NAME "Clipboard")
set(CPACK_PACKAGE_VENDOR "Clipboard")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Clipboard system tray application")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Clipboard")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_app_package_platform "win64")
else()
    set(_app_package_platform "win32")
endif()
set(CPACK_PACKAGE_FILE_NAME "Clipboard-${PROJECT_VERSION}-${_app_package_platform}")

set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_DISPLAY_NAME "Clipboard")
set(CPACK_NSIS_PACKAGE_NAME "Clipboard")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH OFF)

# Create a Start Menu shortcut and a desktop shortcut for the installed app.
set(CPACK_PACKAGE_EXECUTABLES "app;Clipboard")
set(CPACK_CREATE_DESKTOP_LINKS "app")

include(CPack)

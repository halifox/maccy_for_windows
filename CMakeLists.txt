cmake_minimum_required(VERSION 3.25)

project(app VERSION 1.0.0 LANGUAGES C CXX)

if(MSVC)
    # Preserve the compiler's actual output bytes. Some installations only
    # have a Chinese language pack, so forcing VSLANG=1033 does not work.
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/clipboard_include_probe.h" "\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/clipboard_include_probe.cpp" "#include \"clipboard_include_probe.h\"\n")
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /EP /utf-8
        "${CMAKE_CURRENT_BINARY_DIR}/clipboard_include_probe.cpp"
        OUTPUT_VARIABLE _clipboard_include_output ERROR_VARIABLE _clipboard_include_error ENCODING NONE)
    string(APPEND _clipboard_include_output "${_clipboard_include_error}")
    string(REGEX MATCH "[^\r\n]*[A-Za-z]:[/\\\\][^\r\n]*clipboard_include_probe.h" _clipboard_include_line "${_clipboard_include_output}")
    if(_clipboard_include_line)
        string(REGEX REPLACE "[A-Za-z]:[/\\\\].*" "" CMAKE_CL_SHOWINCLUDES_PREFIX "${_clipboard_include_line}")
    endif()
endif()

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
        src/MainWindow.cpp
        src/ClipboardMonitor.cpp
        src/HistoryRenderer.cpp
        src/KeyboardHandler.cpp
        src/Database.cpp
        src/PinKeys.cpp
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
        imm32
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
    target_compile_options(sqlite3 PRIVATE /utf-8)
    target_compile_options(app PRIVATE /utf-8)
    target_link_options(app PRIVATE "/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'")
endif()

include(GNUInstallDirs)

#include(CTest)
#if(BUILD_TESTING)
#    set_property(SOURCE tests/PopupTests.cpp APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
#    add_executable(popup_tests tests/PopupTests.cpp src/Database.cpp src/PinKeys.cpp src/Settings.cpp
#        src/SettingsWindow.cpp src/PreviewWindow.cpp src/MainWindow.rc src/SettingsWindow.rc)
#    target_include_directories(popup_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WTL10_01_Release/Include")
#    target_compile_definitions(popup_tests PRIVATE UNICODE _UNICODE)
#    target_link_libraries(popup_tests PRIVATE sqlite3 user32 imm32 gdi32 shell32 advapi32 comdlg32 comctl32 windowscodecs ole32)
#    if(MSVC)
#        target_compile_options(popup_tests PRIVATE /utf-8)
#        target_link_options(popup_tests PRIVATE "/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'")
#    endif()
#    add_test(NAME popup_regressions COMMAND popup_tests)
#endif()

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

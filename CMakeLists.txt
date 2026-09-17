cmake_minimum_required(VERSION 3.25)

project(maccy VERSION 1.0.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "maccy is a Windows application.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if(MSVC)
    # CMake must know the localized /showIncludes prefix so Ninja can track
    # headers when the installed Visual Studio language is not English.
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

set(MACCY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(MACCY_ASSET_DIR "${MACCY_SOURCE_DIR}/assets")
set(MACCY_APP_ICON_PNG "${MACCY_ASSET_DIR}/app/maccy.png")
set(MACCY_APP_ICON_ICO "${MACCY_ASSET_DIR}/app/maccy.ico")
set(MACCY_THIRD_PARTY_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES/Maccy-LICENSE.txt")
set(WTL_THIRD_PARTY_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES/WTL-MS-PL.txt")
set(MACCY_RESOURCE_FILE "${MACCY_SOURCE_DIR}/TrayIcon.rc")

set(MACCY_SOURCES
        "${MACCY_SOURCE_DIR}/main.cpp"
        "${MACCY_SOURCE_DIR}/MainWindow.cpp"
        "${MACCY_SOURCE_DIR}/ClipboardMonitor.cpp"
        "${MACCY_SOURCE_DIR}/HistoryRenderer.cpp"
        "${MACCY_SOURCE_DIR}/KeyboardHandler.cpp"
        "${MACCY_SOURCE_DIR}/Database.cpp"
        "${MACCY_SOURCE_DIR}/PinKeys.cpp"
        "${MACCY_SOURCE_DIR}/Settings.cpp"
        "${MACCY_SOURCE_DIR}/SettingsWindow.cpp"
        "${MACCY_SOURCE_DIR}/PreviewWindow.cpp"
        "${MACCY_SOURCE_DIR}/TrayIcon.cpp"
        "${MACCY_SOURCE_DIR}/SettingsWindow.rc"
        "${MACCY_SOURCE_DIR}/MainWindow.rc"
        "${MACCY_RESOURCE_FILE}"
)

set(MACCY_RESOURCE_ASSETS
        "${MACCY_APP_ICON_PNG}"
        "${MACCY_APP_ICON_ICO}"
        "${MACCY_ASSET_DIR}/tray/scissors_light_32.png"
        "${MACCY_ASSET_DIR}/tray/scissors_light_16.png"
        "${MACCY_ASSET_DIR}/tray/scissors_dark_32.png"
        "${MACCY_ASSET_DIR}/tray/scissors_dark_16.png"
        "${MACCY_ASSET_DIR}/tray/paperclip_light_32.png"
        "${MACCY_ASSET_DIR}/tray/paperclip_light_16.png"
        "${MACCY_ASSET_DIR}/tray/paperclip_dark_32.png"
        "${MACCY_ASSET_DIR}/tray/paperclip_dark_16.png"
        "${MACCY_ASSET_DIR}/tray/maccy_light_32.png"
        "${MACCY_ASSET_DIR}/tray/maccy_light_16.png"
        "${MACCY_ASSET_DIR}/tray/maccy_dark_32.png"
        "${MACCY_ASSET_DIR}/tray/maccy_dark_16.png"
        "${MACCY_ASSET_DIR}/tray/clipboard_light_32.png"
        "${MACCY_ASSET_DIR}/tray/clipboard_light_16.png"
        "${MACCY_ASSET_DIR}/tray/clipboard_dark_32.png"
        "${MACCY_ASSET_DIR}/tray/clipboard_dark_16.png"
)

foreach(MACCY_ASSET IN LISTS MACCY_RESOURCE_ASSETS)
    if(NOT EXISTS "${MACCY_ASSET}")
        message(FATAL_ERROR "Missing application resource: ${MACCY_ASSET}")
    endif()
endforeach()

set_source_files_properties(
        "${MACCY_RESOURCE_FILE}"
        PROPERTIES OBJECT_DEPENDS "${MACCY_RESOURCE_ASSETS}"
)

add_library(maccy_sqlite STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/sqlite3.c"
)

target_include_directories(maccy_sqlite PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite"
)

target_compile_definitions(maccy_sqlite PRIVATE
        SQLITE_THREADSAFE=1
        SQLITE_ENABLE_FTS5
)

add_executable(maccy WIN32 ${MACCY_SOURCES})

target_include_directories(maccy PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wtl"
)

target_link_libraries(maccy PRIVATE
        maccy_sqlite
        user32
        imm32
        gdi32
        shell32
        advapi32
        comdlg32
        comctl32
        dwmapi
        windowscodecs
        ole32
)

target_compile_definitions(maccy PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(maccy_sqlite PRIVATE /utf-8)
    target_compile_options(maccy PRIVATE /utf-8)
    target_link_options(maccy PRIVATE "/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'")
endif()

include(GNUInstallDirs)

install(TARGETS maccy
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES
        "${MACCY_THIRD_PARTY_LICENSE}"
        "${WTL_THIRD_PARTY_LICENSE}"
        DESTINATION "THIRD_PARTY_NOTICES"
)

if(MSVC)
    # The bundled CMake module can assume an older VC redist folder name, so
    # locate the runtime next to the active compiler toolset instead.
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(MACCY_MSVC_ARCH "x64")
    else()
        set(MACCY_MSVC_ARCH "x86")
    endif()

    get_filename_component(MACCY_MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(MACCY_MSVC_HOST_BIN_DIR "${MACCY_MSVC_BIN_DIR}" DIRECTORY)
    get_filename_component(MACCY_MSVC_TOOLSET_BIN_DIR "${MACCY_MSVC_HOST_BIN_DIR}" DIRECTORY)
    get_filename_component(MACCY_MSVC_TOOLSET_DIR "${MACCY_MSVC_TOOLSET_BIN_DIR}" DIRECTORY)
    get_filename_component(MACCY_MSVC_TOOLS_DIR "${MACCY_MSVC_TOOLSET_DIR}" DIRECTORY)
    get_filename_component(MACCY_VC_TOOLS_DIR "${MACCY_MSVC_TOOLS_DIR}" DIRECTORY)
    get_filename_component(MACCY_VC_DIR "${MACCY_VC_TOOLS_DIR}" DIRECTORY)

    file(GLOB MACCY_MSVC_RUNTIME_DIRS LIST_DIRECTORIES true
         "${MACCY_VC_DIR}/Redist/MSVC/*/${MACCY_MSVC_ARCH}/Microsoft.VC*.CRT")

    set(MACCY_MSVC_RUNTIME_DIR "")
    foreach(MACCY_RUNTIME_DIR IN LISTS MACCY_MSVC_RUNTIME_DIRS)
        if(EXISTS "${MACCY_RUNTIME_DIR}/vcruntime140.dll")
            set(MACCY_MSVC_RUNTIME_DIR "${MACCY_RUNTIME_DIR}")
            break()
        endif()
    endforeach()

    if(MACCY_MSVC_RUNTIME_DIR)
        file(GLOB MACCY_MSVC_RUNTIME_FILES LIST_DIRECTORIES false
             "${MACCY_MSVC_RUNTIME_DIR}/concrt140*.dll"
             "${MACCY_MSVC_RUNTIME_DIR}/msvcp140*.dll"
             "${MACCY_MSVC_RUNTIME_DIR}/vcruntime140*.dll")
        install(FILES ${MACCY_MSVC_RUNTIME_FILES}
                DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    else()
        message(WARNING "MSVC runtime redistributable DLLs were not found; the package will require the Visual C++ runtime on the target machine.")
    endif()
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "maccy")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} system tray application")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_app_package_platform "win64")
else()
    set(_app_package_platform "win32")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${_app_package_platform}")

set(CPACK_GENERATOR "NSIS;ZIP")
set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY "${CMAKE_INSTALL_BINDIR}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_INSTALL_BINDIR}/maccy.exe")
set(CPACK_NSIS_MUI_ICON "${MACCY_APP_ICON_ICO}")
set(CPACK_NSIS_MUI_UNIICON "${MACCY_APP_ICON_ICO}")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH OFF)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
else()
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
endif()

# Create Start Menu and desktop shortcuts that use the icon embedded in maccy.exe.
set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME};${PROJECT_NAME}")
set(CPACK_CREATE_DESKTOP_LINKS "${PROJECT_NAME}")

include(CPack)

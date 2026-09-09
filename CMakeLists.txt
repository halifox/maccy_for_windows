cmake_minimum_required(VERSION 3.25)

project(lowmem_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(sqlite3 STATIC
        third_party/sqlite/sqlite3.c
)

target_include_directories(sqlite3 PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite"
)

target_compile_definitions(sqlite3 PRIVATE
        SQLITE_THREADSAFE=1
)

add_executable(lowmem_app WIN32
        src/main.cpp
        src/Database.cpp
)

target_include_directories(lowmem_app PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WTL10_01_Release/Include"
)

target_link_libraries(lowmem_app PRIVATE
        sqlite3
        user32
        gdi32
        shell32
)

target_compile_definitions(lowmem_app PRIVATE UNICODE _UNICODE)

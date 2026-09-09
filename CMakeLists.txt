cmake_minimum_required(VERSION 3.25)

project(lowmem_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lowmem_app WIN32
        src/main.cpp
)

target_include_directories(lowmem_app PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WTL10_01_Release/Include"
)

target_link_libraries(lowmem_app PRIVATE
        user32
        gdi32
)

target_compile_definitions(lowmem_app PRIVATE UNICODE _UNICODE)
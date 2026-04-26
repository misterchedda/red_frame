cmake_minimum_required(VERSION 3.27)

project(RedFrame VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/MP /EHsc)
endif()

option(RED4EXT_HEADER_ONLY "" ON)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../RED4ext.SDK" "${CMAKE_CURRENT_BINARY_DIR}/RED4ext.SDK")
set_target_properties(RED4ext.SDK PROPERTIES FOLDER "Dependencies")

add_library(RedFrame SHARED
    src/Main.cpp
    src/AudioCaptureService.cpp
    src/CaptureOutput.cpp
    src/EngineViewportProbe.cpp
    src/FrameDumpService.cpp
    src/Harness.cpp
    src/HarnessConfig.cpp
    src/HarnessScriptApi.cpp
    src/PathUtils.cpp
    src/Plugin.cpp
    src/PluginState.cpp
    src/PublicScriptBindings.cpp
    src/ScreenshotService.cpp
    src/ScriptApi.cpp
    src/Internal.hpp
)

target_compile_definitions(RedFrame PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

target_link_libraries(RedFrame PRIVATE RED4ext::SDK)

set_target_properties(RedFrame PROPERTIES
    OUTPUT_NAME "RedFrame"
)

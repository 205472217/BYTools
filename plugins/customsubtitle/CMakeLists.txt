cmake_minimum_required(VERSION 3.16)

project(CustomSubtitlePlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

# Optional: Qt6 WebEngine for embedded browser in step 1
find_package(Qt6 QUIET COMPONENTS WebEngineQuick WebEngineCore)

add_library(CustomSubtitlePlugin MODULE
    CustomSubtitlePlugin.cpp
    CustomSubtitlePlugin.h
    CustomSubtitleController.cpp
    CustomSubtitleController.h
    SubtitleMatcher.cpp
    SubtitleMatcher.h
    FFmpegMergeService.cpp
    FFmpegMergeService.h
    VideoReplaceService.cpp
    VideoReplaceService.h
    plugin.json
)

target_include_directories(CustomSubtitlePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(CustomSubtitlePlugin PRIVATE
    Qt6::Core
    PluginCommon
)

if(Qt6WebEngineQuick_FOUND)
    target_link_libraries(CustomSubtitlePlugin PRIVATE Qt6::WebEngineQuick)
    target_compile_definitions(CustomSubtitlePlugin PRIVATE HAS_QTWEBENGINE)
    message(STATUS "CustomSubtitle: Qt6 WebEngine found - embedded browser enabled")
else()
    message(STATUS "CustomSubtitle: Qt6 WebEngine not found - using system browser fallback")
endif()

set_target_properties(CustomSubtitlePlugin PROPERTIES
    OUTPUT_NAME "CustomSubtitle"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/customsubtitle"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/customsubtitle"
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/customsubtitle/plugin.json
    COPYONLY
)

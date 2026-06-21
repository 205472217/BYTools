cmake_minimum_required(VERSION 3.16)

project(CustomSubtitlePlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(CustomSubtitlePlugin MODULE
    CustomSubtitlePlugin.cpp
    CustomSubtitlePlugin.h
    CustomSubtitleController.cpp
    CustomSubtitleController.h
    CustomSubtitleSettings.cpp
    CustomSubtitleSettings.h
    SubBrowserController.cpp
    SubBrowserController.h
    SubSearchResult.h
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
    Qt6::Gui
    PluginCommon
)

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

# ── Python 脚本拷贝（每次构建时自动同步） ──
set(PYTHON_SRC_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/python")
set(PYTHON_DST_DIR  "${CMAKE_BINARY_DIR}/plugins/customsubtitle/python")

file(GLOB_RECURSE PYTHON_SCRIPTS "${PYTHON_SRC_DIR}/*.py")
add_custom_command(
    OUTPUT  "${PYTHON_DST_DIR}/.copy_stamp"
    DEPENDS ${PYTHON_SCRIPTS}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PYTHON_SRC_DIR}" "${PYTHON_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${PYTHON_DST_DIR}/.copy_stamp"
    COMMENT "Copying Python scripts to build directory"
)
add_custom_target(PythonScripts ALL DEPENDS "${PYTHON_DST_DIR}/.copy_stamp")
add_dependencies(CustomSubtitlePlugin PythonScripts)

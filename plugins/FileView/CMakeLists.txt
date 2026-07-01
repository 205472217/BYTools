cmake_minimum_required(VERSION 3.21)

project(FileViewPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui)

set(PLUGIN_SOURCES
    FileViewPlugin.cpp
    FileViewPlugin.h
    FileViewController.cpp
    FileViewController.h
    FileViewSettings.cpp
    FileViewSettings.h
    FileListModel.cpp
    FileListModel.h
    VideoThumbnailGenerator.cpp
    VideoThumbnailGenerator.h
)

add_library(FileViewPlugin MODULE
    ${PLUGIN_SOURCES}
)

target_include_directories(FileViewPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(FileViewPlugin
    PRIVATE Qt6::Core Qt6::Gui PluginCommon
)

set_target_properties(FileViewPlugin PROPERTIES
    OUTPUT_NAME "FileView"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/FileView"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/FileView"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/FileView")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/FileView/plugin.json
    COPYONLY
)

if(WIN32)
    target_compile_definitions(FileViewPlugin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ── Copy mpv/ directory (if present) ──
set(MPV_SRC_DIR  "${CMAKE_SOURCE_DIR}/third/mpv")
set(MPV_DST_DIR  "${CMAKE_BINARY_DIR}/plugins/FileView/mpv")
if(EXISTS "${MPV_SRC_DIR}")
    file(GLOB_RECURSE MPV_FILES "${MPV_SRC_DIR}/*")
    add_custom_command(
        OUTPUT  "${MPV_DST_DIR}/.copy_stamp"
        DEPENDS ${MPV_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MPV_SRC_DIR}" "${MPV_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${MPV_DST_DIR}/.copy_stamp"
        COMMENT "Copying mpv to build directory (FileView)"
    )
    add_custom_target(FileViewMpv ALL DEPENDS "${MPV_DST_DIR}/.copy_stamp")
    add_dependencies(FileViewPlugin FileViewMpv)
    install(DIRECTORY "${MPV_SRC_DIR}/" DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/fileview/mpv")
endif()

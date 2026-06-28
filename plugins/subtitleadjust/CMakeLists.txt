cmake_minimum_required(VERSION 3.21)

project(SubtitleAdjustPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

set(PLUGIN_SOURCES
    SubtitleAdjustPlugin.cpp
    SubtitleAdjustPlugin.h
    SubtitleAdjustController.cpp
    SubtitleAdjustController.h
    SubtitleAdjustSettings.cpp
    SubtitleAdjustSettings.h
    MatchPairModel.cpp
    MatchPairModel.h
)

add_library(SubtitleAdjustPlugin MODULE
    ${PLUGIN_SOURCES}
)

target_include_directories(SubtitleAdjustPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(SubtitleAdjustPlugin
    PRIVATE Qt6::Core PluginCommon
)

set_target_properties(SubtitleAdjustPlugin PROPERTIES
    OUTPUT_NAME "SubtitleAdjust"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/subtitleadjust"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/subtitleadjust"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/subtitleadjust")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/subtitleadjust/plugin.json
    COPYONLY
)

if(WIN32)
    target_compile_definitions(SubtitleAdjustPlugin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ── Copy mpv/ directory (if present) ──
set(MPV_SRC_DIR  "${CMAKE_SOURCE_DIR}/third/mpv")
set(MPV_DST_DIR  "${CMAKE_BINARY_DIR}/plugins/subtitleadjust/mpv")
if(EXISTS "${MPV_SRC_DIR}")
    file(GLOB_RECURSE MPV_FILES "${MPV_SRC_DIR}/*")
    add_custom_command(
        OUTPUT  "${MPV_DST_DIR}/.copy_stamp"
        DEPENDS ${MPV_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MPV_SRC_DIR}" "${MPV_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${MPV_DST_DIR}/.copy_stamp"
        COMMENT "Copying mpv to build directory (SubtitleAdjust)"
    )
    add_custom_target(SubtitleAdjustMpv ALL DEPENDS "${MPV_DST_DIR}/.copy_stamp")
    add_dependencies(SubtitleAdjustPlugin SubtitleAdjustMpv)
    install(DIRECTORY "${MPV_SRC_DIR}/" DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/subtitleadjust/mpv")
endif()

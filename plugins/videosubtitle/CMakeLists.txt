cmake_minimum_required(VERSION 3.16)

project(VideoSubtitlePlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

add_library(VideoSubtitlePlugin MODULE
    VideoSubtitlePlugin.cpp
    VideoSubtitlePlugin.h
    VideoSubtitleController.cpp
    VideoSubtitleController.h
    VideoSubtitleSettings.cpp
    VideoSubtitleSettings.h
    WhisperService.cpp
    WhisperService.h
    TranslateService.cpp
    TranslateService.h
    SubtitleService.cpp
    SubtitleService.h
    FFmpegService.cpp
    FFmpegService.h
    PluginLogger.h
    plugin.json
)

target_include_directories(VideoSubtitlePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(VideoSubtitlePlugin PRIVATE
    Qt6::Core Qt6::Network Qt6::Widgets
)

set_target_properties(VideoSubtitlePlugin PROPERTIES
    OUTPUT_NAME "VideoSubtitle"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/videosubtitle"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/videosubtitle"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/videosubtitle/models")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/videosubtitle/plugin.json
    COPYONLY
)

# 部署自带 ffmpeg
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg.exe")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/videosubtitle/ffmpeg")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg.exe
        ${CMAKE_BINARY_DIR}/plugins/videosubtitle/ffmpeg/ffmpeg.exe
        COPYONLY
    )
endif()

# 部署自带 whisper (如果有)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/whisper/whisper-cli.exe")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/videosubtitle/whisper")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/whisper/whisper-cli.exe
        ${CMAKE_BINARY_DIR}/plugins/videosubtitle/whisper/whisper-cli.exe
        COPYONLY
    )
endif()

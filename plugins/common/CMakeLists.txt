cmake_minimum_required(VERSION 3.21)

project(PluginCommon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(PluginCommon STATIC
    Config.h
    FileRenameUtils.h  FileRenameUtils.cpp
    Logger.h          Logger.cpp
    SubtitleUtils.h   SubtitleUtils.cpp
    FfmpegUtils.h     FfmpegUtils.cpp
    SettingsHelper.h  SettingsHelper.cpp
    ProcessRunner.h   ProcessRunner.cpp
    FfmpegRunner.h    FfmpegRunner.cpp
    GlobalConfig.h    GlobalConfig.cpp
)

target_include_directories(PluginCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(PluginCommon PUBLIC Qt6::Core)

cmake_minimum_required(VERSION 3.16)

project(ImageCropPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(ImageCropPlugin MODULE
    ImageCropPlugin.cpp
    ImageCropPlugin.h
    ImageCropController.cpp
    ImageCropController.h
    plugin.json
)

target_include_directories(ImageCropPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(ImageCropPlugin PRIVATE Qt6::Core Qt6::Gui PluginCommon)

set_target_properties(ImageCropPlugin PROPERTIES
    OUTPUT_NAME "ImageCrop"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/imagecrop"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/imagecrop"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/imagecrop")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/imagecrop/plugin.json
    COPYONLY
)
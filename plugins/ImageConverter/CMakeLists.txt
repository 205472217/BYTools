cmake_minimum_required(VERSION 3.16)

project(ImageConverterPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(ImageConverterPlugin MODULE
    ImageConverterPlugin.cpp
    ImageConverterPlugin.h
    ImageConverterController.cpp
    ImageConverterController.h
    ImageConverterSettings.cpp
    ImageConverterSettings.h
    plugin.json
)

target_include_directories(ImageConverterPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(ImageConverterPlugin PRIVATE Qt6::Core Qt6::Gui PluginCommon)

set_target_properties(ImageConverterPlugin PROPERTIES
    OUTPUT_NAME "ImageConverter"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/ImageConverter"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/ImageConverter"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/ImageConverter")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/ImageConverter/plugin.json
    COPYONLY
)

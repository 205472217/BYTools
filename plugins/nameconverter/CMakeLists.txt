cmake_minimum_required(VERSION 3.21)

project(NameConverterPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

set(PLUGIN_SOURCES
    NameConverterPlugin.cpp
    NameConverterPlugin.h
    BatchNameController.cpp
    BatchNameController.h
    NamePreviewModel.cpp
    NamePreviewModel.h
    NameService.cpp
    NameService.h
    TextConverter.cpp
    TextConverter.h
)

add_library(NameConverterPlugin MODULE
    ${PLUGIN_SOURCES}
)

target_include_directories(NameConverterPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(NameConverterPlugin
    PRIVATE Qt6::Core PluginCommon
)

set_target_properties(NameConverterPlugin PROPERTIES
    OUTPUT_NAME "NameConverter"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/nameconverter"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/nameconverter"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/nameconverter")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/nameconverter/plugin.json
    COPYONLY
)

if(WIN32)
    target_compile_definitions(NameConverterPlugin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
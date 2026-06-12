cmake_minimum_required(VERSION 3.16)

project(BatchRenamePlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(BatchRenamePlugin MODULE
    BatchRenamePlugin.cpp
    BatchRenamePlugin.h
    BatchRenameController.cpp
    BatchRenameController.h
    plugin.json
)

target_include_directories(BatchRenamePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
)

target_link_libraries(BatchRenamePlugin PRIVATE Qt6::Core PluginCommon)

set_target_properties(BatchRenamePlugin PROPERTIES
    OUTPUT_NAME "BatchRename"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/batchrename"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/batchrename"
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/batchrename")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${CMAKE_BINARY_DIR}/plugins/batchrename/plugin.json
    COPYONLY
)
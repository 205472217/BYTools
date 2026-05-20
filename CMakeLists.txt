cmake_minimum_required(VERSION 3.21)

project(BYTools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

set(BYTOOLS_PLATFORM_SOURCES)
if(WIN32)
    list(APPEND BYTOOLS_PLATFORM_SOURCES resources/app.rc)
endif()

set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/components/FeatureCard.qml PROPERTIES QT_RESOURCE_ALIAS components/FeatureCard.qml)
set_source_files_properties(qml/components/IconButton.qml PROPERTIES QT_RESOURCE_ALIAS components/IconButton.qml)
set_source_files_properties(qml/icons/arrow-left.svg PROPERTIES QT_RESOURCE_ALIAS icons/arrow-left.svg)
set_source_files_properties(qml/icons/app.svg PROPERTIES QT_RESOURCE_ALIAS icons/app.svg)
set_source_files_properties(qml/icons/folder-open.svg PROPERTIES QT_RESOURCE_ALIAS icons/folder-open.svg)
set_source_files_properties(qml/icons/languages.svg PROPERTIES QT_RESOURCE_ALIAS icons/languages.svg)
set_source_files_properties(qml/icons/play.svg PROPERTIES QT_RESOURCE_ALIAS icons/play.svg)
set_source_files_properties(qml/icons/spark.svg PROPERTIES QT_RESOURCE_ALIAS icons/spark.svg)
set_source_files_properties(qml/icons/trash.svg PROPERTIES QT_RESOURCE_ALIAS icons/trash.svg)
set_source_files_properties(qml/icons/undo.svg PROPERTIES QT_RESOURCE_ALIAS icons/undo.svg)
set_source_files_properties(qml/pages/HomePage.qml PROPERTIES QT_RESOURCE_ALIAS pages/HomePage.qml)
set_source_files_properties(qml/pages/RenameConverterPage.qml PROPERTIES QT_RESOURCE_ALIAS pages/RenameConverterPage.qml)

qt_add_executable(BYTools
    src/main.cpp
    src/app/AppController.cpp
    src/app/AppController.h
    src/core/FeatureInfo.h
    src/core/OperationResult.h
    src/features/renameconverter/BatchRenameController.cpp
    src/features/renameconverter/BatchRenameController.h
    src/features/renameconverter/RenamePreviewModel.cpp
    src/features/renameconverter/RenamePreviewModel.h
    src/features/renameconverter/RenameService.cpp
    src/features/renameconverter/RenameService.h
    src/features/renameconverter/TextConverter.cpp
    src/features/renameconverter/TextConverter.h
    ${BYTOOLS_PLATFORM_SOURCES}
)

qt_add_qml_module(BYTools
    URI BYTools
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/FeatureCard.qml
        qml/components/IconButton.qml
        qml/pages/HomePage.qml
        qml/pages/RenameConverterPage.qml
    RESOURCES
        qml/icons/arrow-left.svg
        qml/icons/app.svg
        qml/icons/folder-open.svg
        qml/icons/languages.svg
        qml/icons/play.svg
        qml/icons/spark.svg
        qml/icons/trash.svg
        qml/icons/undo.svg
)

target_link_libraries(BYTools
    PRIVATE Qt6::Quick
)

if(WIN32)
    set_target_properties(BYTools PROPERTIES WIN32_EXECUTABLE TRUE)
    target_compile_definitions(BYTools PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

cmake_minimum_required(VERSION 3.21)

project(BYTools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Core)

qt_standard_project_setup(REQUIRES 6.5)

set(BYTOOLS_PLATFORM_SOURCES)
if(WIN32)
    list(APPEND BYTOOLS_PLATFORM_SOURCES resources/app.rc)
endif()


qt_add_executable(BYTools
    src/main.cpp
    src/app/AppController.cpp
    src/app/AppController.h
    src/core/FeatureInfo.h
    src/core/OperationResult.h
    src/core/PluginInterface.h
    src/core/PluginManager.h
    src/core/PluginManager.cpp
    src/core/ThemeManager.h
    src/core/ThemeManager.cpp
    src/core/MpvPlayer.h
    src/core/MpvPlayer.cpp
    ${BYTOOLS_PLATFORM_SOURCES}
)

# In Debug build, show console window; in Release build, hide it (GUI mode)
if (WIN32)
    set_target_properties(BYTools PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

set(BYTOOLS_QML_FILES
    qml/Main.qml
    qml/components/FeatureCard.qml
    qml/components/IconButton.qml
    qml/components/TextFieldEx.qml
    qml/components/ComboBoxEx.qml
    qml/components/CheckBoxEx.qml
    qml/components/RadioButtonEx.qml
    qml/components/ImageViewer.qml
    qml/components/MpvViewer.qml
    qml/pages/HomePage.qml
    qml/pages/NameConverterPage.qml
    qml/pages/BatchRenamePage.qml
    qml/pages/ImageConverterPage.qml
    qml/pages/ImageCropPage.qml
    qml/pages/VideoSubtitlePage.qml
    qml/pages/VideoSubtitleSettingsPage.qml
    qml/pages/CustomSubtitlePage.qml
    qml/pages/SubtitleAdjustPage.qml
    qml/pages/FileViewPage.qml 
)

# Optional Qt6 Multimedia for video playback (subtitle adjust plugin)
find_package(Qt6 QUIET COMPONENTS Multimedia)
if(Qt6Multimedia_FOUND)
    message(STATUS "BYTools: Qt6 Multimedia found - video player component enabled")
    list(APPEND BYTOOLS_QML_FILES qml/components/MediaViewer.qml)
else()
    message(STATUS "BYTools: Qt6 Multimedia not found - video preview disabled")
endif()


qt_add_qml_module(BYTools
    URI BYTools
    VERSION 1.0
    QML_FILES ${BYTOOLS_QML_FILES}
    # 资源文件
    RESOURCES
        qml/resources.qrc
)

target_link_libraries(BYTools
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Qml Qt6::Core
)

if(WIN32)
    target_compile_definitions(BYTools PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_subdirectory(plugins/common)
add_subdirectory(plugins/nameconverter)
add_subdirectory(plugins/batchrename)
add_subdirectory(plugins/imageconverter)
add_subdirectory(plugins/imagecrop)
add_subdirectory(plugins/videosubtitle)
add_subdirectory(plugins/customsubtitle)
add_subdirectory(plugins/subtitleadjust)
add_subdirectory(plugins/fileview)
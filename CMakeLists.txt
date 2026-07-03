cmake_minimum_required(VERSION 3.21)

project(BYTools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Core)

qt_standard_project_setup(REQUIRES 6.5)

set(BYTOOLS_PLATFORM_SOURCES)
set(BYTOOLS_PLATFORM_QML_FILES)
if(WIN32)
    list(APPEND BYTOOLS_PLATFORM_SOURCES resources/app.rc src/core/MpvPlayer.h src/core/MpvPlayer.cpp)
    list(APPEND BYTOOLS_PLATFORM_QML_FILES qml/components/MpvViewer.qml)
endif()


qt_add_executable(BYTools
    src/main.cpp
    src/core/OperationResult.h
    src/core/PluginInterface.h
    src/core/PluginManager.h
    src/core/PluginManager.cpp
    src/core/ThemeManager.h
    src/core/ThemeManager.cpp
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
    qml/components/ConfirmDialog.qml
    qml/components/RadioButtonEx.qml
    qml/components/ViewToggleBtn.qml
    qml/components/ImageViewer.qml
    ${BYTOOLS_PLATFORM_QML_FILES}
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
add_subdirectory(plugins/NameConverter)
add_subdirectory(plugins/BatchRename)
add_subdirectory(plugins/ImageConverter)
add_subdirectory(plugins/ImageCrop)
add_subdirectory(plugins/VideoSubtitle)
add_subdirectory(plugins/CustomSubtitle)
add_subdirectory(plugins/SubtitleAdjust)
add_subdirectory(plugins/FileView)

# ── Copy third/ffmpeg/ to app directory (for global auto-detection) ──
set(FFMPEG_SRC_DIR "${CMAKE_SOURCE_DIR}/third/ffmpeg")
set(FFMPEG_DST_DIR "${CMAKE_BINARY_DIR}/third/ffmpeg")
if(EXISTS "${FFMPEG_SRC_DIR}")
    file(GLOB_RECURSE FFMPEG_FILES "${FFMPEG_SRC_DIR}/*")
    add_custom_command(
        OUTPUT  "${FFMPEG_DST_DIR}/.copy_stamp"
        DEPENDS ${FFMPEG_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${FFMPEG_SRC_DIR}" "${FFMPEG_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${FFMPEG_DST_DIR}/.copy_stamp"
        COMMENT "Copying ffmpeg to app directory"
    )
    add_custom_target(CopyFfmpeg ALL DEPENDS "${FFMPEG_DST_DIR}/.copy_stamp")
    add_dependencies(BYTools CopyFfmpeg)
endif()
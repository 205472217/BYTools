@echo off
setlocal

:: Define output directory (package folder)
set "OUT_DIR=%~dp0\Package\BYTools"
set "PLUGIN_OUT=%OUT_DIR%\plugins"

:: Clean previous package if exists
if exist "%OUT_DIR%" rd /s /q "%OUT_DIR%"

:: Create directory structure
mkdir "%OUT_DIR%"
mkdir "%PLUGIN_OUT%"

:: Copy the main executable and README
copy "build\BYTools.exe" "%OUT_DIR%"
copy "README.md" "%OUT_DIR%"

:: Copy third-party libraries
mkdir "%OUT_DIR%\third"
if exist "third\ffmpeg" (
    xcopy /E /I "third\ffmpeg" "%OUT_DIR%\third\ffmpeg"
)
if exist "third\whisper_amd" (
    xcopy /E /I "third\whisper_amd" "%OUT_DIR%\third\whisper_amd"
)
if exist "third\whisper_cpu" (
    xcopy /E /I "third\whisper_cpu" "%OUT_DIR%\third\whisper_cpu"
)
if exist "third\model" (
    xcopy /E /I "third\model" "%OUT_DIR%\third\model"
)

::==========================plugins===============================
:: Copy BatchRename plugin
mkdir "%PLUGIN_OUT%\BatchRename"
copy "build\plugins\BatchRename\libBatchRename.dll" "%PLUGIN_OUT%\BatchRename"

:: Copy ImageConverter plugin
mkdir "%PLUGIN_OUT%\ImageConverter"
copy "build\plugins\ImageConverter\libImageConverter.dll" "%PLUGIN_OUT%\ImageConverter"

:: Copy ImageCrop plugin
mkdir "%PLUGIN_OUT%\ImageCrop"
copy "build\plugins\ImageCrop\libImageCrop.dll" "%PLUGIN_OUT%\ImageCrop"

:: Copy NameConverter plugin
mkdir "%PLUGIN_OUT%\NameConverter"
copy "build\plugins\NameConverter\libNameConverter.dll" "%PLUGIN_OUT%\NameConverter"

:: Copy VideoSubtitle plugin
mkdir "%PLUGIN_OUT%\VideoSubtitle"
copy "build\plugins\VideoSubtitle\libVideoSubtitle.dll" "%PLUGIN_OUT%\VideoSubtitle"

:: Copy CustomSubtitle plugin
mkdir "%PLUGIN_OUT%\CustomSubtitle"
copy "build\plugins\CustomSubtitle\libCustomSubtitle.dll" "%PLUGIN_OUT%\CustomSubtitle"
if exist "plugins\CustomSubtitle\python" (
    xcopy /E /I "plugins\CustomSubtitle\python" "%PLUGIN_OUT%\CustomSubtitle\python"
)

:: Copy adjustsubtitle plugin
mkdir "%PLUGIN_OUT%\SubtitleAdjust"
copy "build\plugins\SubtitleAdjust\libSubtitleAdjust.dll" "%PLUGIN_OUT%\SubtitleAdjust"
if exist "third\mpv" (
    xcopy /E /I "third\mpv" "%PLUGIN_OUT%\SubtitleAdjust\mpv"
)

:: Copy FileView plugin
mkdir "%PLUGIN_OUT%\FileView"
copy "build\plugins\FileView\libFileView.dll" "%PLUGIN_OUT%\FileView"
if exist "third\mpv" (
    xcopy /E /I "third\mpv" "%PLUGIN_OUT%\FileView\mpv"
)
::==========================plugins===============================

:: Deploy Qt runtime dependencies for the exe (including QML modules)
windeployqt --dir "%OUT_DIR%" "%OUT_DIR%\BYTools.exe" --qmldir "%~dp0qml"

:: Deploy Qt runtime dependencies for each plugin DLL
for %%F in ("%PLUGIN_OUT%\*.dll") do (
    windeployqt --dir "%PLUGIN_OUT%" "%%F"
)

echo Packaging completed.

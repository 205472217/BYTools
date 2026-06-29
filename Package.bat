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
:: Copy batchrename plugin
mkdir "%PLUGIN_OUT%\card-batchrename"
copy "build\plugins\batchrename\libBatchRename.dll" "%PLUGIN_OUT%\card-batchrename"

:: Copy imageconverter plugin
mkdir "%PLUGIN_OUT%\card-imageconverter"
copy "build\plugins\imageconverter\libImageConverter.dll" "%PLUGIN_OUT%\card-imageconverter"

:: Copy imagecrop plugin
mkdir "%PLUGIN_OUT%\card-imagecrop"
copy "build\plugins\imagecrop\libImageCrop.dll" "%PLUGIN_OUT%\card-imagecrop"

:: Copy nameconverter plugin
mkdir "%PLUGIN_OUT%\card-nameconverter"
copy "build\plugins\nameconverter\libNameConverter.dll" "%PLUGIN_OUT%\card-nameconverter"

:: Copy videosubtitle plugin
mkdir "%PLUGIN_OUT%\card-videosubtitle"
copy "build\plugins\videosubtitle\libVideoSubtitle.dll" "%PLUGIN_OUT%\card-videosubtitle"

:: Copy customsubtitle plugin
mkdir "%PLUGIN_OUT%\card-customsubtitle"
copy "build\plugins\customsubtitle\libCustomSubtitle.dll" "%PLUGIN_OUT%\card-customsubtitle"
if exist "plugins\customsubtitle\python" (
    xcopy /E /I "plugins\customsubtitle\python" "%PLUGIN_OUT%\card-customsubtitle\python"
)

:: Copy adjustsubtitle plugin
mkdir "%PLUGIN_OUT%\card-subtitleadjust"
copy "build\plugins\subtitleadjust\libSubtitleAdjust.dll" "%PLUGIN_OUT%\card-subtitleadjust"
if exist "third\mpv" (
    xcopy /E /I "third\mpv" "%PLUGIN_OUT%\card-subtitleadjust\mpv"
)

:: Copy fileview plugin
mkdir "%PLUGIN_OUT%\card-fileview"
copy "build\plugins\fileview\libFileView.dll" "%PLUGIN_OUT%\card-fileview"
if exist "third\mpv" (
    xcopy /E /I "third\mpv" "%PLUGIN_OUT%\card-fileview\mpv"
)
::==========================plugins===============================

:: Deploy Qt runtime dependencies for the exe (including QML modules)
windeployqt --dir "%OUT_DIR%" "%OUT_DIR%\BYTools.exe" --qmldir "%~dp0qml"

:: Deploy Qt runtime dependencies for each plugin DLL
for %%F in ("%PLUGIN_OUT%\*.dll") do (
    windeployqt --dir "%PLUGIN_OUT%" "%%F"
)

echo Packaging completed.

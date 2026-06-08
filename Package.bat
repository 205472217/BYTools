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

:: Copy the main executable
copy "build\BYTools.exe" "%OUT_DIR%"

:: Copy plugin DLLs
copy "build\plugins\batchrename\libBatchRename.dll" "%PLUGIN_OUT%"
copy "build\plugins\imageconverter\libImageConverter.dll" "%PLUGIN_OUT%"
copy "build\plugins\imagecrop\libImageCrop.dll" "%PLUGIN_OUT%"
copy "build\plugins\nameconverter\libNameConverter.dll" "%PLUGIN_OUT%"
copy "build\plugins\videosubtitle\libVideoSubtitle.dll" "%PLUGIN_OUT%"

:: Deploy Qt runtime dependencies for the exe
windeployqt --dir "%OUT_DIR%" "%OUT_DIR%\BYTools.exe"

:: Deploy Qt runtime dependencies for each plugin DLL
for %%F in ("%PLUGIN_OUT%\*.dll") do (
    windeployqt --dir "%PLUGIN_OUT%" "%%F"
)

echo Packaging completed.

@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "DEST=tools\mingw32"
set "ARCHIVE=tools\winlibs-i686-msvcrt.7z"
set "URL=https://github.com/brechtsanders/winlibs_mingw/releases/download/12.4.0posix-12.0.0-msvcrt-r1/winlibs-i686-posix-dwarf-gcc-12.4.0-mingw-w64msvcrt-12.0.0-r1.7z"

if exist "%DEST%\bin\g++.exe" (
    echo [OK] Toolchain already present: %DEST%\bin
    "%DEST%\bin\g++.exe" -dumpmachine
    exit /b 0
)

where curl >nul 2>&1
if errorlevel 1 (
    echo [ERROR] curl not found. Install curl or download manually:
    echo %URL%
    exit /b 1
)

where 7z >nul 2>&1
if errorlevel 1 (
    if exist "%ProgramFiles%\7-Zip\7z.exe" (
        set "SEVENZ=%ProgramFiles%\7-Zip\7z.exe"
    ) else if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" (
        set "SEVENZ=%ProgramFiles(x86)%\7-Zip\7z.exe"
    ) else (
        echo [ERROR] 7-Zip ^(7z^) not found. Install from https://www.7-zip.org/
        exit /b 1
    )
) else (
    set "SEVENZ=7z"
)

if not exist tools mkdir tools

echo Downloading WinLibs i686 MSVCRT GCC 12.4 ^(~110 MB^)...
curl -L -o "%ARCHIVE%" "%URL%"
if errorlevel 1 (
    echo [ERROR] Download failed.
    exit /b 1
)

if exist "%DEST%" rmdir /s /q "%DEST%"

echo Extracting...
"%SEVENZ%" x -y -o"tools" "%ARCHIVE%"
if errorlevel 1 (
    echo [ERROR] Extraction failed.
    exit /b 1
)

if not exist "%DEST%\bin\g++.exe" (
    echo [ERROR] Expected %DEST%\bin\g++.exe after extraction.
    exit /b 1
)

echo [OK] Toolchain ready: %DEST%\bin
"%DEST%\bin\g++.exe" -dumpmachine
endlocal

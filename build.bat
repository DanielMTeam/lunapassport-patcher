@echo off
setlocal
cd /d "%~dp0"

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found. Install CMake: https://cmake.org/download/
    exit /b 1
)

where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found. Install MinGW-w64 i686 and add to PATH.
    echo Example: https://github.com/niXman/mingw-builds-binaries/releases
    exit /b 1
)

if not exist build mkdir build
cd build

cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 exit /b 1

cmake --build . --config Release
if errorlevel 1 exit /b 1

echo.
echo [OK] lunapassport-patcher.exe is in: %CD%
dir /b lunapassport-patcher.exe
endlocal

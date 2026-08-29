@echo off
setlocal EnableExtensions
cd /d "%~dp0"

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found. Install CMake: https://cmake.org/download/
    exit /b 1
)

rem Prefer bundled or user-specified 32-bit MinGW (required for Windows XP).
if defined MINGW32_BIN (
    set "PATH=%MINGW32_BIN%;%PATH%"
) else if exist "%~dp0tools\mingw32\bin\g++.exe" (
    set "PATH=%~dp0tools\mingw32\bin;%PATH%"
)

where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found.
    echo.
    echo Windows XP needs a 32-bit MinGW-w64 i686 toolchain with MSVCRT.
    echo Run setup-toolchain.bat or add tools\mingw32\bin to PATH.
    exit /b 1
)

for /f "delims=" %%T in ('g++ -dumpmachine 2^>nul') do set "TRIPLET=%%T"
echo %TRIPLET% | findstr /i "i686" >nul
if errorlevel 1 (
    echo [ERROR] Wrong compiler architecture: %TRIPLET%
    echo.
    echo lunapassport-patcher must be built as 32-bit i686 for Windows XP.
    echo Your g++ is 64-bit ^(x86_64^). A 64-bit exe shows "not a valid Win32 application" on XP.
    echo.
    echo Fix:
    echo   1. Run setup-toolchain.bat
    echo   2. Or install WinLibs i686 MSVCRT from https://winlibs.com/
    echo   3. Or set MINGW32_BIN=C:\path\to\i686\bin before build.bat
    exit /b 1
)

for /f "delims=" %%P in ('where g++ 2^>nul') do (
    set "GPP=%%P"
    goto :have_gpp
)
:have_gpp
echo %GPP% | findstr /i "ucrt" >nul
if not errorlevel 1 (
    echo [ERROR] UCRT toolchain detected: %GPP%
    echo.
    echo Windows XP does not support UCRT. Use MSVCRT instead.
    echo Run setup-toolchain.bat or pick "MSVCRT" on https://winlibs.com/
    exit /b 1
)

echo [OK] Using i686 toolchain: %TRIPLET%

if not exist build mkdir build
cd build

cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 exit /b 1

cmake --build . --config Release
if errorlevel 1 exit /b 1

echo.
echo [OK] lunapassport-patcher.exe is in: %CD%
for /f "delims=" %%F in ('where objdump 2^>nul') do (
    objdump -f lunapassport-patcher.exe | findstr /i "pei-i386 file format"
    goto :done_arch
)
:done_arch
dir /b lunapassport-patcher.exe
endlocal

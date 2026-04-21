@echo off
:: Build script for ShutdownRestoreService
:: Requires: CMake + MSVC (Visual Studio Build Tools)
echo ============================================
echo   Building ShutdownRestoreService
echo ============================================

if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo Trying Visual Studio 16 2019...
    cmake .. -G "Visual Studio 16 2019" -A x64
)
if errorlevel 1 (
    echo.
    echo ERROR: CMake configure failed. Make sure Visual Studio is installed.
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo   BUILD SUCCESSFUL!
echo   Executable: build\Release\ShutdownRestoreService.exe
echo ============================================
echo.
echo Next steps:
echo   1. Open a CMD/PowerShell as ADMINISTRATOR
echo   2. Run:  build\Release\ShutdownRestoreService.exe --install
echo.
pause

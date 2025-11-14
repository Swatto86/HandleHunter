@echo off
echo Testing for GCC compiler...
echo.
where gcc >nul 2>&1
if errorlevel 1 (
    echo ERROR: gcc not found in PATH
    echo.
    echo Please install MinGW-w64:
    echo 1. Download from: https://www.mingw-w64.org/downloads/
    echo 2. Or use winget: winget install mingw-w64
    echo 3. Add MinGW bin directory to PATH
    echo.
    pause
    exit /b 1
)

echo Found GCC:
gcc --version
echo.
echo GCC is available! You can now run build.bat
echo.
pause


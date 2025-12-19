@echo off
REM ============================================
REM build_msvc.bat - Build script using MSVC
REM ============================================
REM
REM This script compiles HandleHunter using Microsoft Visual C++ (MSVC)
REM from Visual Studio Build Tools instead of MinGW GCC.
REM
REM REQUIREMENTS:
REM - Visual Studio 2022 Build Tools with C++ support
REM - Windows SDK headers
REM
REM USAGE:
REM - build_msvc.bat         : Compile and link the executable
REM - build_msvc.bat clean   : Remove all build artifacts
REM
REM WHY MSVC INSTEAD OF GCC:
REM - Native Windows toolchain
REM - Better Windows SDK integration
REM - Excellent optimization
REM - Industry standard for Windows development
REM ============================================

REM Check for clean argument
if /i "%~1"=="clean" goto clean
if /i "%~1"=="/clean" goto clean
if /i "%~1"=="-clean" goto clean
goto build

REM ============================================
REM CLEAN TARGET
REM ============================================
:clean
echo Cleaning build artifacts...
if exist *.obj del /Q *.obj
if exist *.res del /Q *.res
if exist HandleHunter.exe del /Q HandleHunter.exe
echo Clean complete.
goto end

REM ============================================
REM BUILD TARGET
REM ============================================
:build
echo ========================================
echo Building HandleHunter with MSVC...
echo ========================================
echo.

REM ============================================
REM Initialize Visual Studio environment
REM ============================================
echo Initializing MSVC environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to initialize MSVC environment
    goto error
)

REM ============================================
REM Configuration Variables
REM ============================================

REM Resource compiler
set RC=rc.exe

REM Compiler flags for MSVC
REM /O2              : Optimize for speed
REM /Os              : Favor code size
REM /W4              : Warning level 4 (high)
REM /DUNICODE        : Define UNICODE macro
REM /D_UNICODE       : Define _UNICODE macro
REM /nologo          : Suppress copyright banner
REM /c               : Compile only, don't link
REM /Zi              : Generate debug info (optional, remove for smaller exe)
REM /GS-             : Disable security checks (reduces size, CRT-free compatible)
set CFLAGS=/O2 /Os /W4 /DUNICODE /D_UNICODE /nologo /c /GS-

REM Linker flags
REM /SUBSYSTEM:WINDOWS       : GUI application (not console)
REM /ENTRY:WinMainCRTStartup : Entry point
REM /NOLOGO                  : Suppress copyright banner
REM kernel32.lib             : Core Windows API
REM user32.lib               : User interface functions (wsprintfW is here)
REM gdi32.lib                : GDI functions (CreateFont, CreateSolidBrush, DeleteObject)
REM advapi32.lib             : Advanced API (Registry functions)
REM netapi32.lib             : Network API (NetFileEnum/NetFileClose)
REM comctl32.lib             : Common Controls (ListView)
REM dwmapi.lib               : Desktop Window Manager (dark mode)
REM uxtheme.lib              : UxTheme (SetWindowTheme)
set LDFLAGS=/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup /NOLOGO kernel32.lib user32.lib gdi32.lib advapi32.lib netapi32.lib comctl32.lib dwmapi.lib uxtheme.lib

REM Target executable
set TARGET=HandleHunter.exe

REM ============================================
REM STEP 1: Compile Resource File
REM ============================================
echo [1/7] Compiling resources...
%RC% /nologo /fo HandleHunter.res HandleHunter.rc
if errorlevel 1 (
    echo ERROR: Failed to compile resources
    goto error
)

REM ============================================
REM STEP 2: Compile main.c
REM ============================================
echo [2/7] Compiling main.c...
cl %CFLAGS% main.c
if errorlevel 1 (
    echo ERROR: Failed to compile main.c
    goto error
)

REM ============================================
REM STEP 3: Compile gui.c
REM ============================================
echo [3/7] Compiling gui.c...
cl %CFLAGS% gui.c
if errorlevel 1 (
    echo ERROR: Failed to compile gui.c
    goto error
)

REM ============================================
REM STEP 4: Compile netapi.c
REM ============================================
echo [4/7] Compiling netapi.c...
cl %CFLAGS% netapi.c
if errorlevel 1 (
    echo ERROR: Failed to compile netapi.c
    goto error
)

REM ============================================
REM STEP 5: Compile lockinfo.c
REM ============================================
echo [5/7] Compiling lockinfo.c...
cl %CFLAGS% lockinfo.c
if errorlevel 1 (
    echo ERROR: Failed to compile lockinfo.c
    goto error
)

REM ============================================
REM STEP 6: Compile modern_ui.c
REM ============================================
echo [6/7] Compiling modern_ui.c...
cl %CFLAGS% modern_ui.c
if errorlevel 1 (
    echo ERROR: Failed to compile modern_ui.c
    goto error
)

REM ============================================
REM STEP 7: Compile utilities.c
REM ============================================
echo [7/7] Compiling utilities.c...
cl %CFLAGS% utilities.c
if errorlevel 1 (
    echo ERROR: Failed to compile utilities.c
    goto error
)

REM ============================================
REM LINKING
REM ============================================
echo Linking %TARGET%...
link /OUT:%TARGET% main.obj gui.obj netapi.obj lockinfo.obj modern_ui.obj utilities.obj HandleHunter.res %LDFLAGS%
if errorlevel 1 (
    echo ERROR: Failed to link %TARGET%
    goto error
)

REM ============================================
REM SUCCESS
REM ============================================
echo.
echo ========================================
echo Build successful!
echo ========================================
if exist %TARGET% (
    echo Output: %TARGET%
    dir %TARGET% | find ".exe"
)
echo.
goto end

REM ============================================
REM ERROR
REM ============================================
:error
echo.
echo ========================================
echo Build FAILED!
echo ========================================
echo.
exit /b 1

REM ============================================
REM END
REM ============================================
:end

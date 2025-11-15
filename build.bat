@echo off
REM ============================================
REM build.bat - Build script for HandleHunter
REM ============================================
REM
REM This script compiles and links HandleHunter using MinGW-w64 (GCC for Windows).
REM 
REM REQUIREMENTS:
REM - MinGW-w64 with gcc and windres in PATH
REM - Windows SDK headers (included with MinGW-w64)
REM
REM USAGE:
REM - build.bat         : Compile and link the executable
REM - build.bat clean   : Remove all build artifacts
REM
REM WHY BATCH FILE INSTEAD OF MAKEFILE:
REM - Simpler for Windows users (no need for make utility)
REM - Easy to run from Command Prompt
REM - Self-contained (no dependencies except compiler)
REM
REM COMPILATION PROCESS:
REM 1. Compile resources (manifest, icon) into .res file
REM 2. Compile each .c file into .o object file
REM 3. Link all .o files + .res file into final .exe
REM ============================================

REM Check for clean argument (case-insensitive)
REM WHY MULTIPLE CHECKS: Support various command styles (clean, /clean, -clean)
if /i "%~1"=="clean" goto clean
if /i "%~1"=="/clean" goto clean
if /i "%~1"=="-clean" goto clean
goto build

REM ============================================
REM CLEAN TARGET - Remove build artifacts
REM ============================================
:clean
echo Cleaning build artifacts...
REM Delete object files (*.o) if they exist
if exist *.o del /Q *.o
REM Delete resource files (*.res) if they exist  
if exist *.res del /Q *.res
REM Delete executable if it exists
if exist HandleHunter.exe del /Q HandleHunter.exe
echo Clean complete.
goto end

REM ============================================
REM BUILD TARGET - Compile and link
REM ============================================
:build
echo ========================================
echo Building HandleHunter...
echo ========================================
echo.

REM ============================================
REM Configuration Variables
REM ============================================
REM WHY VARIABLES: Easy to modify compiler/linker settings in one place

REM Compiler executable (gcc from MinGW-w64)
set CC=gcc

REM Resource compiler executable (converts .rc to .res)
REM WHY NEEDED: Embeds icon and manifest into .exe
set WINDRES=windres

REM Compiler flags
REM -O2              : Optimize for speed (level 2)
REM -Wall            : Enable all common warnings
REM -DUNICODE        : Define UNICODE macro (use wide char APIs)
REM -D_UNICODE       : Define _UNICODE macro (for C runtime)
REM -D__USE_MINGW_ANSI_STDIO=1 : Use MinGW's printf (for %ls support)
REM WHY THESE FLAGS: 
REM   - Optimization reduces exe size and improves performance
REM   - Warnings catch potential bugs
REM   - UNICODE ensures we use UTF-16 APIs (required for non-English filenames)
set CFLAGS=-O2 -Wall -DUNICODE -D_UNICODE -D__USE_MINGW_ANSI_STDIO=1

REM Linker flags
REM -mwindows        : Build GUI application (not console)
REM -lnetapi32       : Link Network API library (for NetFileEnum/NetFileClose)
REM -lcomctl32       : Link Common Controls library (for ListView, StatusBar)
REM -ldwmapi         : Link Desktop Window Manager library (for dark mode)
REM -luxtheme        : Link UxTheme library (for SetWindowTheme)
REM -s               : Strip debug symbols (reduces exe size)
REM WHY THESE FLAGS:
REM   - mwindows prevents console window from appearing
REM   - Libraries provide APIs we use (file locks, controls, theming)
REM   - Stripping reduces file size significantly (~40%)
set LDFLAGS=-mwindows -lnetapi32 -lcomctl32 -ldwmapi -luxtheme -s

REM Target executable name
set TARGET=HandleHunter.exe

REM ============================================
REM STEP 1: Compile Resource File
REM ============================================
REM WHY FIRST: Resources (icon, manifest) are independent of C code
echo [1/6] Compiling resources...
REM windres converts .rc file to .res file (binary format)
REM -O coff : Output in COFF format (required for GCC linker)
%WINDRES% HandleHunter.rc -O coff -o HandleHunter.res
if errorlevel 1 (
    echo ERROR: Failed to compile resources
    goto error
)

REM ============================================
REM STEP 2: Compile main.c
REM ============================================
echo [2/6] Compiling main.c...
REM -c : Compile only (don't link), output object file
REM -o : Specify output filename
%CC% %CFLAGS% -c main.c -o main.o
if errorlevel 1 (
    echo ERROR: Failed to compile main.c
    goto error
)

REM ============================================
REM STEP 3: Compile gui.c
REM ============================================
echo [3/6] Compiling gui.c...
%CC% %CFLAGS% -c gui.c -o gui.o
if errorlevel 1 (
    echo ERROR: Failed to compile gui.c
    goto error
)

REM ============================================
REM STEP 4: Compile netapi.c
REM ============================================
echo [4/6] Compiling netapi.c...
%CC% %CFLAGS% -c netapi.c -o netapi.o
if errorlevel 1 (
    echo ERROR: Failed to compile netapi.c
    goto error
)

REM ============================================
REM STEP 5: Compile lockinfo.c
REM ============================================
echo [5/6] Compiling lockinfo.c...
%CC% %CFLAGS% -c lockinfo.c -o lockinfo.o
if errorlevel 1 (
    echo ERROR: Failed to compile lockinfo.c
    goto error
)

REM ============================================
REM STEP 6: Compile modern_ui.c
REM ============================================
echo [6/6] Compiling modern_ui.c...
%CC% %CFLAGS% -c modern_ui.c -o modern_ui.o
if errorlevel 1 (
    echo ERROR: Failed to compile modern_ui.c
    goto error
)

REM ============================================
REM LINKING: Combine all object files into executable
REM ============================================
echo Linking %TARGET%...
REM Link all .o files and .res file together
REM WHY THIS ORDER: Object files first, then resource file
REM %LDFLAGS% at end: Linker flags must come after source files for GCC
%CC% main.o gui.o netapi.o lockinfo.o modern_ui.o HandleHunter.res -o %TARGET% %LDFLAGS%
if errorlevel 1 (
    echo ERROR: Failed to link %TARGET%
    goto error
)

REM ============================================
REM SUCCESS: Show build result
REM ============================================
echo.
echo ========================================
echo Build successful!
echo ========================================
REM Show file info if executable exists
if exist %TARGET% (
    echo Output: %TARGET%
    REM Display file size and date
    dir %TARGET% | find ".exe"
)
echo.
goto end

REM ============================================
REM ERROR: Build failed
REM ============================================
:error
echo.
echo ========================================
echo Build FAILED!
echo ========================================
echo.
REM Exit with error code 1 (indicates failure to caller)
exit /b 1

REM ============================================
REM END: Script complete
REM ============================================
:end
REM Script ends here (success or after clean)

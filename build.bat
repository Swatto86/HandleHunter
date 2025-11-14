@echo off
REM ============================================
REM build.bat - Build script for HandleHunter
REM ============================================

REM Check for clean argument
if /i "%~1"=="clean" goto clean
if /i "%~1"=="/clean" goto clean
if /i "%~1"=="-clean" goto clean
goto build

:clean
echo Cleaning build artifacts...
if exist *.o del /Q *.o
if exist *.res del /Q *.res
if exist HandleHunter.exe del /Q HandleHunter.exe
echo Clean complete.
goto end

:build
echo ========================================
echo Building HandleHunter...
echo ========================================
echo.

REM Configuration
set CC=gcc
set WINDRES=windres
set CFLAGS=-O2 -Wall -DUNICODE -D_UNICODE -D__USE_MINGW_ANSI_STDIO=1
set LDFLAGS=-mwindows -lnetapi32 -lcomctl32 -ldwmapi -luxtheme -s
set TARGET=HandleHunter.exe

REM Compile resource file (includes manifest)
echo [1/6] Compiling resources...
%WINDRES% HandleHunter.rc -O coff -o HandleHunter.res
if errorlevel 1 (
    echo ERROR: Failed to compile resources
    goto error
)

REM Compile main.c
echo [2/6] Compiling main.c...
%CC% %CFLAGS% -c main.c -o main.o
if errorlevel 1 (
    echo ERROR: Failed to compile main.c
    goto error
)

REM Compile gui.c
echo [3/6] Compiling gui.c...
%CC% %CFLAGS% -c gui.c -o gui.o
if errorlevel 1 (
    echo ERROR: Failed to compile gui.c
    goto error
)

REM Compile netapi.c
echo [4/6] Compiling netapi.c...
%CC% %CFLAGS% -c netapi.c -o netapi.o
if errorlevel 1 (
    echo ERROR: Failed to compile netapi.c
    goto error
)

REM Compile lockinfo.c
echo [5/6] Compiling lockinfo.c...
%CC% %CFLAGS% -c lockinfo.c -o lockinfo.o
if errorlevel 1 (
    echo ERROR: Failed to compile lockinfo.c
    goto error
)

REM Compile modern_ui.c
echo [6/6] Compiling modern_ui.c...
%CC% %CFLAGS% -c modern_ui.c -o modern_ui.o
if errorlevel 1 (
    echo ERROR: Failed to compile modern_ui.c
    goto error
)

REM Link with resource file
echo Linking %TARGET%...
%CC% main.o gui.o netapi.o lockinfo.o modern_ui.o HandleHunter.res -o %TARGET% %LDFLAGS%
if errorlevel 1 (
    echo ERROR: Failed to link %TARGET%
    goto error
)

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

:error
echo.
echo ========================================
echo Build FAILED!
echo ========================================
echo.
exit /b 1

:end

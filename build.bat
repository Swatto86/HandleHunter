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
set CFLAGS=-O2 -Wall -DUNICODE -D_UNICODE
set LDFLAGS=-mwindows -lnetapi32 -lcomctl32 -s
set TARGET=HandleHunter.exe

REM Compile main.c
echo [1/4] Compiling main.c...
%CC% %CFLAGS% -c main.c -o main.o
if errorlevel 1 (
    echo ERROR: Failed to compile main.c
    goto error
)

REM Compile gui.c
echo [2/4] Compiling gui.c...
%CC% %CFLAGS% -c gui.c -o gui.o
if errorlevel 1 (
    echo ERROR: Failed to compile gui.c
    goto error
)

REM Compile netapi.c
echo [3/4] Compiling netapi.c...
%CC% %CFLAGS% -c netapi.c -o netapi.o
if errorlevel 1 (
    echo ERROR: Failed to compile netapi.c
    goto error
)

REM Compile lockinfo.c
echo [4/4] Compiling lockinfo.c...
%CC% %CFLAGS% -c lockinfo.c -o lockinfo.o
if errorlevel 1 (
    echo ERROR: Failed to compile lockinfo.c
    goto error
)

REM Link
echo Linking %TARGET%...
%CC% main.o gui.o netapi.o lockinfo.o -o %TARGET% %LDFLAGS%
if errorlevel 1 (
    echo ERROR: Failed to link %TARGET%
    goto error
)

REM Embed manifest (optional)
if exist manifest.xml (
    where mt.exe >nul 2>&1
    if not errorlevel 1 (
        echo Embedding manifest...
        mt.exe -nologo -manifest manifest.xml -outputresource:%TARGET%;1 2>nul
        if errorlevel 1 (
            echo WARNING: Failed to embed manifest
        ) else (
            echo Manifest embedded successfully
        )
    ) else (
        echo NOTE: mt.exe not found - manifest not embedded
        echo The app will still work, but UAC prompt may not appear
    )
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
echo Run as Administrator to use the program.
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

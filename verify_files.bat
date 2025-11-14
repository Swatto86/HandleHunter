@echo off
echo ========================================
echo HandleHunter - File Verification
echo ========================================
echo.

set MISSING=0

echo Checking required files...
echo.

if not exist main.c (
    echo [MISSING] main.c
    set MISSING=1
) else (
    echo [OK] main.c
)

if not exist gui.c (
    echo [MISSING] gui.c
    set MISSING=1
) else (
    echo [OK] gui.c
)

if not exist gui.h (
    echo [MISSING] gui.h
    set MISSING=1
) else (
    echo [OK] gui.h
)

if not exist netapi.c (
    echo [MISSING] netapi.c
    set MISSING=1
) else (
    echo [OK] netapi.c
)

if not exist netapi.h (
    echo [MISSING] netapi.h
    set MISSING=1
) else (
    echo [OK] netapi.h
)

if not exist lockinfo.c (
    echo [MISSING] lockinfo.c
    set MISSING=1
) else (
    echo [OK] lockinfo.c
)

if not exist lockinfo.h (
    echo [MISSING] lockinfo.h
    set MISSING=1
) else (
    echo [OK] lockinfo.h
)

if not exist resource.h (
    echo [MISSING] resource.h
    set MISSING=1
) else (
    echo [OK] resource.h
)

if not exist manifest.xml (
    echo [MISSING] manifest.xml
    set MISSING=1
) else (
    echo [OK] manifest.xml
)

if not exist build.bat (
    echo [MISSING] build.bat
    set MISSING=1
) else (
    echo [OK] build.bat
)

echo.
if %MISSING%==1 (
    echo ========================================
    echo ERROR: Some files are missing!
    echo ========================================
    pause
    exit /b 1
) else (
    echo ========================================
    echo SUCCESS: All files present!
    echo ========================================
    echo.
    echo You can now build by running:
    echo   build.bat
    echo.
)

pause


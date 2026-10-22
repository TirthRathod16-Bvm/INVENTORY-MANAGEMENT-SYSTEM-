@echo off
echo ============================================================
echo  Inventory Management System - Build Script
echo ============================================================

REM Check if g++ is in PATH, otherwise fallback to C:\MinGW\bin
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    if exist "C:\MinGW\bin\g++.exe" (
        set "PATH=C:\MinGW\bin;%PATH%"
    ) else (
        echo [ERROR] g++ not found. Please install MinGW or MSYS2.
        echo   Download: https://www.mingw-w64.org/downloads/
        pause
        exit /b 1
    )
)

echo [OK] g++ found.
echo [..] Compiling...

g++ -std=c++14 -Wall -Wextra -O2 ^
    src/main.cpp ^
    -o InventorySystem.exe ^
    2>build_errors.txt

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed! See build_errors.txt
    type build_errors.txt
    pause
    exit /b 1
)

echo [OK] Build successful! Output: InventorySystem.exe
echo.
echo Run with: InventorySystem.exe
echo ============================================================
pause

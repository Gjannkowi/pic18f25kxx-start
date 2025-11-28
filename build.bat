@echo off
setlocal

rem ==============================================
rem PIC18F25K42 + XC8 v3.10 – build.bat (działa zawsze)
rem hex tylko w out/, reszta w build/
rem ==============================================

set DEVICE=PIC18F25K42
set XC8="C:\Program Files\Microchip\xc8\v3.10\bin\xc8-cc.exe"
set DFP="C:\Program Files\Microchip\PIC18F-K_DFP\xc8"
set SOURCE=src\main.c
set BUILD_DIR=build
set OUT_DIR=out
set HEX_BUILD=%BUILD_DIR%\PIC18F25K42_Blink.hex
set HEX_OUT=%OUT_DIR%\PIC18F25K42_Blink.hex

echo.
echo === Kompilacja %DEVICE% ===
echo.

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %OUT_DIR%   mkdir %OUT_DIR%

%XC8% -mcpu=%DEVICE% -mdfp=%DFP% -O2 -o "%HEX_BUILD%" %SOURCE%

if %errorlevel% neq 0 (
    echo.
    echo !!! BLAD KOMPILACJI !!!
    echo.
    pause
    exit /b 1
)

echo.
echo === Kopiowanie czystego .hex do out/ ===
copy "%HEX_BUILD%" "%HEX_OUT%" >nul

echo.
echo ================================================
echo SUKCES! Czysty hex gotowy:
echo %HEX_OUT%
echo ================================================
echo.
echo Wgraj programatorem programatorem do mikrokontrolera.
echo.

pause
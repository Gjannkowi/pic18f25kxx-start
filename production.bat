@echo off
setlocal EnableDelayedExpansion

rem =====================================================
rem  PIC18F25K42 – PRODUCTION.BAT (wersja finalna, działa!)
rem  CLEAN + BUILD + PROGRAM przez NSDSP-2-X (HVP + 5.0V)
rem =====================================================

set "DEVICE=PIC18F25K42"
set "XC8=C:\Program Files\Microchip\xc8\v3.10\bin\xc8-cc.exe"
set "DFP=C:\Program Files\Microchip\PIC18F-K_DFP\xc8"
set "SOURCE=src\main.c"
set "BUILD_DIR=build"
set "OUT_DIR=out"
set "HEX_BUILD=%BUILD_DIR%\PIC18F25K42_Blink.hex"
set "FINAL_HEX=%OUT_DIR%\PIC18F25K42_Blink.hex"

echo.
echo =====================================================
echo   PIC18F25K42 - CLEAN + BUILD + PROGRAM
echo   NSDSP-2-X (High Voltage Programming + 5.0 V)
echo =====================================================
echo.

rem ================== 1. CZYSZCZENIE ==================
echo [1/3] Czyszczenie poprzednich plikow...
if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%" >nul 2>&1
if exist "%OUT_DIR%"   rmdir /S /Q "%OUT_DIR%"   >nul 2>&1
mkdir "%BUILD_DIR%" >nul 2>&1
mkdir "%OUT_DIR%"   >nul 2>&1

rem ================== 2. KOMPILACJA ===================
echo.
echo [2/3] Kompilacja %DEVICE%...
"%XC8%" -mcpu=%DEVICE% -mdfp="%DFP%" -O2 -o "%HEX_BUILD%" "%SOURCE%"

if %errorlevel% neq 0 (
    echo.
    echo !!! BLAD KOMPILACJI !!!
    echo.
    pause
    exit /b 1
)

echo.
echo Kopiowanie pliku .hex...
copy "%HEX_BUILD%" "%FINAL_HEX%" >nul

if not exist "%FINAL_HEX%" (
    echo.
    echo BLAD: Nie udalo sie skopiowac pliku .hex!
    pause
    exit /b 1
)

echo.
echo ================================================
echo   SUKCES! Skompilowano poprawnie
echo   %FINAL_HEX%
echo ================================================
echo.

rem ================== 3. PROGRAMOWANIE NSDSP ===============
echo [3/3] Wgrywanie na mikrokontroler...
echo.
if exist "nsprog.exe" (set "NSPROG=nsprog.exe") else (set "NSPROG=nsprog")

echo Uruchamiam programowanie (HVP + 5.0 V)...
echo.

"%NSPROG%" p -h -d %DEVICE% -i "%FINAL_HEX%" -t 5.0 -r 2000 >"%TEMP%\nsprog_out.txt" 2>&1
type "%TEMP%\nsprog_out.txt"

echo.

findstr /C:"Done" "%TEMP%\nsprog_out.txt" >nul 2>&1
if %errorlevel% equ 0 (
    echo.
    echo ================================================
    echo   GOTOWE! Mikrokontroler %DEVICE%
    echo   zostal poprawnie zaprogramowany!
    echo ================================================
) else (
    echo.
    echo ================================================
    echo   !!! BLAD PROGRAMOWANIA !!!
    echo   Mozliwe przyczyny:
    echo   - NSDSP nie podlaczony
    echo   - Zle podlaczenie ICSP
    echo   - Jumper na NSDSP nie na 5V
    echo ================================================
)

del "%TEMP%\nsprog_out.txt" >nul 2>&1
echo.
pause
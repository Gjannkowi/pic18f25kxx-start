@echo off
setlocal EnableDelayedExpansion

rem =====================================================
rem  Wgrywanie HEX-a na PIC18F25K42 przez NSDSP-2-X
rem  Tryb: HVP + 5.0 V – absolutnie niezawodne wykrywanie sukcesu
rem =====================================================

set "HEX=out\PIC18F25K42_Blink.hex"
set "DEVICE=PIC18F25K42"

echo.
echo ================================================
echo   Wgrywanie %DEVICE%
echo   przez NSDSP-2-X (HVP + 5.0 V)
echo   Plik: %HEX%
echo ================================================
echo.

if not exist "%HEX%" (
    echo.
    echo BLAD: Nie znaleziono pliku HEX!
    echo Najpierw uruchom build.bat lub make
    echo.
    pause
    exit /b 1
)

if exist "nsprog.exe" (set "NSPROG=nsprog.exe") else (set "NSPROG=nsprog")

echo Uruchamiam programowanie...
echo.

rem Przeżywamy cały output do tymczasowego pliku i jednocześnie wyświetlamy
%NSPROG% p -h -d %DEVICE% -i "%HEX%" -t 5.0 -r 2000 >"%TEMP%\nsprog_output.txt" 2>&1
type "%TEMP%\nsprog_output.txt"

echo.

rem Sprawdzamy czy w całym wyjściu jest słowo "Done"
findstr /C:"Done" "%TEMP%\nsprog_output.txt" >nul 2>&1
if %errorlevel% equ 0 (
    echo ================================================
    echo   SUKCES! Mikrokontroler %DEVICE%
    echo   zostal poprawnie zaprogramowany!
    echo ================================================
) else (
    echo ================================================
    echo   BLAD programowania!
    echo.
    echo   Mozliwe przyczyny:
    echo   - NSDSP odlaczony od USB
    echo   - Zle podlaczenie ICSP
    echo   - Za dlugi kabel ICSP
    echo   - Jumper na NSDSP nie na 5V
    echo ================================================
)

del "%TEMP%\nsprog_output.txt" 2>nul
echo.
pause
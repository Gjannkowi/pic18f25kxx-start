@echo off
echo.
echo ================================================
echo          CZYSZCZENIE PROJEKTU
echo ================================================
echo.

rem Usuwa tylko zawartość folderów – same foldery zostają puste
if exist build\* (
    del /Q build\* >nul 2>&1
    echo - Zawartosc folderu build/ wyczyszczona
) else (
    echo - Folder build/ juz pusty lub nie istnieje
)

if exist out\* (
    del /Q out\* >nul 2>&1
    echo - Zawartosc folderu out/ wyczyszczona
) else (
    echo - Folder out/ juz pusty lub nie istnieje
)

echo.
echo Gotowe – foldery build/ i out/ sa puste
echo.
pause
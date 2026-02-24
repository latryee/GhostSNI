@echo off
title GhostSNI Test 2
echo ADIM 1: Script basladi
echo.

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Admin degil, yukseltiliyor...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo ADIM 2: Admin yetkisi var
echo.

cd /d "%~dp0"
echo ADIM 3: Dizin: %cd%
echo.

if exist "build\GhostSNI.exe" (
    echo ADIM 4: EXE bulundu: build\GhostSNI.exe
) else (
    echo ADIM 4: EXE BULUNAMADI
)
echo.
echo ADIM 5: Pause oncesi
pause

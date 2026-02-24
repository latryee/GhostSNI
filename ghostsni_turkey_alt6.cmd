@echo off
title GhostSNI
cd /d "%~dp0"

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Yonetici yetkisi gerekli, yeniden baslatiliyor...
    powershell -Command "Start-Process cmd.exe -ArgumentList '/c cd /d %~dp0 && %~nx0' -Verb RunAs"
    exit /b
)

set "EXE="
if exist "%~dp0build\GhostSNI.exe" set "EXE=%~dp0build\GhostSNI.exe"
if exist "%~dp0bin\GhostSNI.exe" set "EXE=%~dp0bin\GhostSNI.exe"

if "%EXE%"=="" (
    echo.
    echo  [HATA] GhostSNI.exe bulunamadi!
    echo  Aranan: %~dp0build\GhostSNI.exe
    echo  Aranan: %~dp0bin\GhostSNI.exe
    echo.
    pause
    exit /b 1
)

echo.
"%EXE%" -f 2 -q -p -r -s -m
echo.
echo GhostSNI kapandi. (Hata kodu: %errorlevel%)
pause

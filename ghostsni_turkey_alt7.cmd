@echo off
title GhostSNI - Minimal - sadece frag + host tricks (en guvenli)

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

if exist "%~dp0bin\GhostSNI.exe" (
    start "" "%~dp0bin\GhostSNI.exe" -f 2 -r -s -m
    goto :started
)
if exist "%~dp0build\GhostSNI.exe" (
    start "" "%~dp0build\GhostSNI.exe" -f 2 -r -s -m
    goto :started
)
if exist "%~dp0GhostSNI.exe" (
    start "" "%~dp0GhostSNI.exe" -f 2 -r -s -m
    goto :started
)

echo.
echo  [HATA] GhostSNI.exe bulunamadi!
echo  Script dizini: %~dp0
echo  Aranan: %~dp0bin\GhostSNI.exe
echo  Aranan: %~dp0build\GhostSNI.exe
echo.
pause
exit /b 1

:started
echo.
echo  GhostSNI baslatiliyor...
echo  Profil: Minimal - sadece frag + host tricks (en guvenli)
echo.
timeout /t 2 /nobreak >nul
tasklist /fi "imagename eq GhostSNI.exe" 2>nul | find /i "GhostSNI.exe" >nul
if %errorlevel%==0 (
    echo  [OK] GhostSNI arka planda calisiyor.
    echo  Kapatmak icin: Tray ikonu ^> Durdur ve Cik
    timeout /t 3
) else (
    echo  [HATA] GhostSNI baslatilamadi!
    pause
)

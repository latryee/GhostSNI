@echo off
title GhostSNI - Cloudflare DNS (1.1.1.1) + tum ozellikler

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

if exist "%~dp0bin\GhostSNI.exe" (
    start "" "%~dp0bin\GhostSNI.exe" -f 2 -e -b -q -p -r -s -m --dns-addr 1.1.1.1 --dns-port 53
    goto :started
)
if exist "%~dp0build\GhostSNI.exe" (
    start "" "%~dp0build\GhostSNI.exe" -f 2 -e -b -q -p -r -s -m --dns-addr 1.1.1.1 --dns-port 53
    goto :started
)
if exist "%~dp0GhostSNI.exe" (
    start "" "%~dp0GhostSNI.exe" -f 2 -e -b -q -p -r -s -m --dns-addr 1.1.1.1 --dns-port 53
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
echo  Profil: Cloudflare DNS (1.1.1.1) + tum ozellikler
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

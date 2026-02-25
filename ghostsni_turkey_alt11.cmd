@echo off
title GhostSNI - Cloudflare DNS (1.1.1.1) + tum ozellikler

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

set "EXE="
if exist "bin\GhostSNI.exe" set "EXE=bin\GhostSNI.exe"
if exist "build\GhostSNI.exe" set "EXE=build\GhostSNI.exe"
if exist "GhostSNI.exe" set "EXE=GhostSNI.exe"

if "%EXE%"=="" (
    echo.
    echo  [HATA] GhostSNI.exe bulunamadi!
    echo  Aranan: bin\ , build\ , .\ 
    echo.
    pause
    exit /b 1
)

echo.
echo  GhostSNI baslatiliyor...
echo  Profil: Cloudflare DNS (1.1.1.1) + tum ozellikler
echo.

start "" "%EXE%" -f 2 -e -b -q -p -r -s -m --dns-addr 1.1.1.1 --dns-port 53

timeout /t 2 /nobreak >nul

tasklist /fi "imagename eq GhostSNI.exe" 2>nul | find /i "GhostSNI.exe" >nul
if %errorlevel%==0 (
    echo  [OK] GhostSNI arka planda calisiyor.
    echo  Kapatmak icin: Tray ikonu ^> Durdur ve Cik
    echo.
    timeout /t 3
) else (
    echo  [HATA] GhostSNI baslatilamadi!
    echo  Yonetici olarak calistirdiginizden emin olun.
    echo.
    pause
)

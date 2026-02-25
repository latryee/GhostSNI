@echo off
title GhostSNI - Servis Kurulumu

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

set "EXE="
if exist "bin\GhostSNI.exe" set "EXE=%~dp0bin\GhostSNI.exe"
if exist "build\GhostSNI.exe" set "EXE=%~dp0build\GhostSNI.exe"
if exist "GhostSNI.exe" set "EXE=%~dp0GhostSNI.exe"

if "%EXE%"=="" (
    echo.
    echo  [HATA] GhostSNI.exe bulunamadi!
    echo.
    pause
    exit /b 1
)

sc stop "GhostSNI" >nul 2>&1
sc delete "GhostSNI" >nul 2>&1
sc create "GhostSNI" binPath= "\"%EXE%\" -f 2 -e -b -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53" DisplayName= "GhostSNI DPI Bypass" start= auto type= own
sc description "GhostSNI" "GhostSNI DPI Bypass - Arka plan servisi"
sc start "GhostSNI"

echo.
echo  [+] Servis kuruldu ve baslatildi.
echo      EXE: %EXE%
echo      Kaldirmak icin: service_remove.cmd
echo.
pause

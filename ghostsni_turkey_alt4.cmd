@echo off
title GhostSNI - QUIC block kapali

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
    echo  Aranan konumlar: bin\ , build\ , .\ 
    echo.
    pause
    exit /b 1
)

echo  GhostSNI baslatiliyor...
echo  Profil: QUIC block kapali
echo  EXE: %EXE%
echo.

"%EXE%" -f 2 -e --ttl 3 -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53

echo.
echo  GhostSNI kapandi. (Kod: %errorlevel%)
pause

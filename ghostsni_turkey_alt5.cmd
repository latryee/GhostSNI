@echo off
title GhostSNI

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

if exist "build\GhostSNI.exe" (
    "build\GhostSNI.exe" -f 4 -e -b -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53
) else if exist "bin\GhostSNI.exe" (
    "bin\GhostSNI.exe" -f 4 -e -b -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53
) else (
    echo [HATA] GhostSNI.exe bulunamadi!
)

pause

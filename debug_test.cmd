@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)
cd /d "%~dp0"
if exist "build\GhostSNI.exe" (
    start "" "build\GhostSNI.exe" -f 2 -e -b -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53 -v
) else (
    start "" "GhostSNI.exe" -f 2 -e -b -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53 -v
)

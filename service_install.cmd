@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)
cd /d "%~dp0"

if not exist "bin\GhostSNI.exe" (
    echo [HATA] bin\GhostSNI.exe bulunamadi!
    pause
    exit /b 1
)

set "GHOST_PATH=%~dp0bin\GhostSNI.exe"

sc stop "GhostSNI" >nul 2>&1
sc delete "GhostSNI" >nul 2>&1
sc create "GhostSNI" binPath= "\"%GHOST_PATH%\" -f 2 -e -b --wrong-chksum --reverse-frag -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53" DisplayName= "GhostSNI DPI Bypass" start= auto type= own
sc description "GhostSNI" "GhostSNI DPI Bypass - Arka plan servisi"
sc start "GhostSNI"

echo.
echo [+] Servis kuruldu ve baslatildi.
echo     Kaldirmak icin: service_remove.cmd
echo.
pause

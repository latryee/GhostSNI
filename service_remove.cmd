@echo off
title GhostSNI - Servis Kaldirma

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

sc stop "GhostSNI" >nul 2>&1
sc delete "GhostSNI" >nul 2>&1

echo.
echo  [+] GhostSNI servisi durduruldu ve kaldirildi.
echo.
pause

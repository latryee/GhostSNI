@echo off

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo =============================================================
echo   GhostSNI - Servis Kaldirma
echo =============================================================
echo.

echo [*] GhostSNI servisi durduruluyor...
net stop GhostSNI >nul 2>&1

timeout /t 2 /nobreak >nul

echo [*] GhostSNI servisi kaldiriliyor...
sc delete GhostSNI

if %errorlevel% neq 0 (
    echo [HATA] Servis kaldirilamadi!
    pause
    exit /b 1
)

echo.
echo [+] GhostSNI servisi basariyla kaldirildi!
echo.

pause

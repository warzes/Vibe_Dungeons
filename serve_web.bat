@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================
echo  Dungeon Crawlers - Web Server
echo ============================================
echo.
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "WEB_DIR=%ROOT%\bin\web"

if not exist "%WEB_DIR%\index.html" (
    echo [ERROR] Web build not found. Run build_web.bat first.
    echo   Expected: %WEB_DIR%\index.html
    pause
    exit /b 1
)

echo Starting HTTP server on http://localhost:8080
echo Open http://localhost:8080 in your browser.
echo Press Ctrl+C to stop.
echo.

cd /D "%WEB_DIR%"

:: Try Python 3 first
python --version >nul 2>nul
if %ERRORLEVEL% equ 0 (
    python -m http.server 8080
    goto :EOF
)

:: Fallback: Node.js
where npx >nul 2>nul
if %ERRORLEVEL% equ 0 (
    npx http-server -p 8080 --cors
    goto :EOF
)

echo [ERROR] Neither Python nor Node.js found.
pause
exit /b 1

endlocal

@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================
echo  Dungeon Crawlers - Web (Emscripten) Build
echo ============================================

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
if exist "D:\develop\emsdk\emsdk_env.bat" (
    set "EMSDK=D:\develop\emsdk"
) else (
    set "EMSDK=C:\develop\emsdk"
)

call "%EMSDK%\emsdk_env.bat" >nul 2>&1

set "CXX=em++"
set "OUT_DIR=%ROOT%\bin\web"
set "OUT_HTML=%OUT_DIR%\index.html"

set "CXXFLAGS=-std=c++26 -Wall -Wextra -Wpedantic -Wno-uninitialized-const-pointer -Wno-unused-private-field -Wno-experimental -Wno-ignored-gch -O2 -g"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src\3rdparty"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src\3rdparty\imgui"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src\3rdparty\glm"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src\3rdparty\nlohmann"

set "CXXFLAGS=%CXXFLAGS% -sUSE_SDL=3"
set "CXXFLAGS=%CXXFLAGS% -sUSE_WEBGL2=1"
set "CXXFLAGS=%CXXFLAGS% -sFULL_ES3=1"
set "CXXFLAGS=%CXXFLAGS% -sMAX_WEBGL_VERSION=2"
set "CXXFLAGS=%CXXFLAGS% -sMIN_WEBGL_VERSION=2"
set "CXXFLAGS=%CXXFLAGS% -sALLOW_MEMORY_GROWTH=1"
set "CXXFLAGS=%CXXFLAGS% -sINITIAL_MEMORY=64MB"
set "CXXFLAGS=%CXXFLAGS% -sMAXIMUM_MEMORY=512MB"
set "CXXFLAGS=%CXXFLAGS% -sEXIT_RUNTIME=1"
set "CXXFLAGS=%CXXFLAGS% -sFORCE_FILESYSTEM=1"
set "CXXFLAGS=%CXXFLAGS% -sASSERTIONS"
set "CXXFLAGS=%CXXFLAGS% -sGL_ASSERTIONS=1"
set "CXXFLAGS=%CXXFLAGS% -sERROR_ON_UNDEFINED_SYMBOLS=0"
set "CXXFLAGS=%CXXFLAGS% -DIMGUI_IMPL_OPENGL_ES3"
set "CXXFLAGS=%CXXFLAGS% -include src/stdafx.h"

set "SOURCES="

set "SOURCES=!SOURCES! "%ROOT%\src\stdafx.cpp""

for /R "%ROOT%\src\core" %%F in (*.cpp) do set "SOURCES=!SOURCES! "%%F""
for /R "%ROOT%\src\engine" %%F in (*.cpp) do set "SOURCES=!SOURCES! "%%F""
for /R "%ROOT%\src\game" %%F in (*.cpp) do set "SOURCES=!SOURCES! "%%F""

for %%F in ("%ROOT%\src\3rdparty\imgui\imgui.cpp" "%ROOT%\src\3rdparty\imgui\imgui_draw.cpp" "%ROOT%\src\3rdparty\imgui\imgui_tables.cpp" "%ROOT%\src\3rdparty\imgui\imgui_widgets.cpp" "%ROOT%\src\3rdparty\imgui\imgui_impl_sdl3.cpp" "%ROOT%\src\3rdparty\imgui\imgui_impl_opengl3.cpp") do set "SOURCES=!SOURCES! "%%F""

set "SOURCES=!SOURCES! "%ROOT%\src\3rdparty\stb\stb.cpp""
set "SOURCES=!SOURCES! "%ROOT%\src\3rdparty\cgltf\cgltf.cpp""
set "SOURCES=!SOURCES! "%ROOT%\src\3rdparty\tiny_obj_loader\tiny_obj_loader.cpp""

set "SOURCES=!SOURCES! "%ROOT%\src\main.cpp""

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo Compiling for Web...
cd /D "%ROOT%"
%CXX% %CXXFLAGS% %SOURCES% -o "%OUT_HTML%"

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Web build completed!
    echo Output: %OUT_HTML%
) else (
    echo.
    echo [FAILED] Web build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo Done.
endlocal

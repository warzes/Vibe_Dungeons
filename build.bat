@echo off
chcp 65001 >nul

setlocal enabledelayedexpansion

echo ============================================
echo  Dungeon Crawlers - Build Script
echo ============================================

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"

set "SDL_INC=%ROOT%\src\3rdparty\SDL3\x86_64-w64-mingw32\include"
set "SDL_LIB=%ROOT%\src\3rdparty\SDL3\x86_64-w64-mingw32\lib"
set "SDL_DLL=%ROOT%\src\3rdparty\SDL3\x86_64-w64-mingw32\bin\SDL3.dll"
set "GLAD_INC=%ROOT%\src\3rdparty"
set "IMGUI_DIR=%ROOT%\src\3rdparty\imgui"
set "GLM_DIR=%ROOT%\src\3rdparty\glm"
set "JSON_DIR=%ROOT%\src\3rdparty\nlohmann"
set "CORE_INC=%ROOT%\src\core"
set "ENGINE_INC=%ROOT%\src\engine"
set "GAME_INC=%ROOT%\src\game"
set "OUT_DIR=%ROOT%\bin"
set "OUT_EXE=%OUT_DIR%\dungeon_crawlers.exe"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

set "CXX=g++"
set "CXXFLAGS=-std=c++26 -Wall -Wextra -Wpedantic -Werror -O2 -g -Wno-unused-function"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src"
set "CXXFLAGS=%CXXFLAGS% -I%ROOT%\src\3rdparty"
set "CXXFLAGS=%CXXFLAGS% -I%SDL_INC%"
set "CXXFLAGS=%CXXFLAGS% -I%GLAD_INC%"
set "CXXFLAGS=%CXXFLAGS% -I%IMGUI_DIR%"
set "CXXFLAGS=%CXXFLAGS% -I%JSON_DIR%"
set "CXXFLAGS=%CXXFLAGS% -I%CORE_INC%"
set "CXXFLAGS=%CXXFLAGS% -I%ENGINE_INC%"
set "CXXFLAGS=%CXXFLAGS% -I%GAME_INC%"

set "LDFLAGS=-L%SDL_LIB% -lSDL3 -lopengl32"

:: Precompiled header (GCC .gch)
set "PCH_FILE=%ROOT%\src\stdafx.h"
set "PCH_GCH=%ROOT%\src\stdafx.h.gch"
%CXX% %CXXFLAGS% -x c++-header "%PCH_FILE%" -o "%PCH_GCH%"
if %ERRORLEVEL% neq 0 (
    echo [FAILED] PCH compilation failed.
    exit /b %ERRORLEVEL%
)

set "CXXFLAGS=%CXXFLAGS% -include src/stdafx.h"

:: Object directory
set "OBJ_DIR=%ROOT%\_obj\gcc"
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

:: Collect all source files into a temp file (one per line, quoted)
set "ALL_SRCS=%TEMP%\dc_all_srcs.txt"
(
    echo "%ROOT%\src\stdafx.cpp"
    for /R "%ROOT%\src\core" %%F in (*.cpp) do echo "%%F"
    for /R "%ROOT%\src\engine" %%F in (*.cpp) do echo "%%F"
    for /R "%ROOT%\src\game" %%F in (*.cpp) do echo "%%F"
    for %%F in ("%IMGUI_DIR%\imgui.cpp" "%IMGUI_DIR%\imgui_draw.cpp" "%IMGUI_DIR%\imgui_tables.cpp" "%IMGUI_DIR%\imgui_widgets.cpp" "%IMGUI_DIR%\imgui_impl_sdl3.cpp" "%IMGUI_DIR%\imgui_impl_opengl3.cpp") do echo "%%F"
    echo "%ROOT%\src\3rdparty\glad\gl.c"
    echo "%ROOT%\src\3rdparty\stb\stb.cpp"
    echo "%ROOT%\src\3rdparty\cgltf\cgltf.cpp"
    echo "%ROOT%\src\3rdparty\tiny_obj_loader\tiny_obj_loader.cpp"
    echo "%ROOT%\src\main.cpp"
) > "%ALL_SRCS%"

:: Use PowerShell to find changed files (locale-independent UTC comparison)
set "CHG_LIST=%TEMP%\dc_changed.txt"
powershell -NoProfile -Command "$r='%ROOT%';$od='%OBJ_DIR%';$c=@();Get-Content '%ALL_SRCS%'|ForEach-Object{$s=$_.Trim('""');$rel=$s.Substring($r.Length+1)-replace'\\','.';$o=Join-Path $od ($rel+'.o');if(-not(Test-Path $o)){$c+=$s;return};$st=[System.IO.File]::GetLastWriteTimeUtc($s);$ot=[System.IO.File]::GetLastWriteTimeUtc($o);if($st-gt$ot){$c+=$s}};($c -join [Environment]::NewLine)|Out-File -FilePath '%CHG_LIST%' -Encoding ASCII"

:: Job tracking directory
set "JOB_DIR=%TEMP%\dc_build_jobs"
if exist "%JOB_DIR%" rmdir /s /q "%JOB_DIR%"
mkdir "%JOB_DIR%"

:: Phase 1: compile changed files in parallel
set JOB_COUNT=0
for /f "usebackq delims=" %%F in ("%CHG_LIST%") do call :compile_file "%%F"
goto :after_compile

:compile_file
set "SRC=%~1"
set "REL=!SRC:%ROOT%\=!"
set "REL=!REL:\=.!"
set "OBJ=%OBJ_DIR%\!REL!.o"
set /a JOB_COUNT+=1
set "WRAPPER=%JOB_DIR%\job_!JOB_COUNT!.bat"
set "DONE_FLAG=%JOB_DIR%\job_!JOB_COUNT!.done"
(
    echo @"%CXX%" %CXXFLAGS% -c "!SRC!" -o "!OBJ!" ^> "!OBJ!.log" 2^>^&1
    echo @type nul ^> "!DONE_FLAG!"
) > "!WRAPPER!"
start /b "" cmd /c "!WRAPPER!"
goto :eof

:after_compile
if %JOB_COUNT% gtr 0 (
    echo Compiling %JOB_COUNT% file(s^) in parallel...
) else (
    echo No files to compile.
    goto :link
)

:: Wait for all jobs to finish
:wait_loop
set DONE=0
for %%F in ("%JOB_DIR%\*.done") do set /a DONE+=1
if %DONE% lss %JOB_COUNT% (
    timeout /t 0 /nobreak >nul 2>&1
    goto :wait_loop
)

:: Check for failures in .log files
set BUILD_FAILED=0
for %%F in ("%OBJ_DIR%\*.log") do (
    findstr /i ": error:" "%%F" >nul 2>&1
    if not errorlevel 1 set BUILD_FAILED=1
)

if %BUILD_FAILED% neq 0 (
    echo.
    echo [FAILED] Compilation errors:
    for %%F in ("%OBJ_DIR%\*.log") do (
        findstr /i ": error:" "%%F" >nul 2>&1
        if not errorlevel 1 (
            echo -- %%~nF --
            type "%%F"
        )
    )
    rmdir /s /q "%JOB_DIR%" 2>nul
    exit /b 1
)

:link
echo.
echo Linking...
set "OBJ_FILES="
for /f "usebackq delims=" %%F in ("%ALL_SRCS%") do (
	set "SRC=%%~F"
	set "REL=!SRC:%ROOT%\=!"
	set "REL=!REL:\=.!"
	set "OBJ=%OBJ_DIR%\!REL!.o"
	if exist "!OBJ!" set "OBJ_FILES=!OBJ_FILES! "!OBJ!""
)

%CXX% !OBJ_FILES! %LDFLAGS% -o "%OUT_EXE%"
if %ERRORLEVEL% neq 0 (
    echo [FAILED] Link failed with error code %ERRORLEVEL%
    rmdir /s /q "%JOB_DIR%" 2>nul
    exit /b %ERRORLEVEL%
)

rmdir /s /q "%JOB_DIR%" 2>nul

echo.
echo [SUCCESS] Build completed!
echo Output: %OUT_EXE%

if exist "%SDL_DLL%" (
    copy /Y "%SDL_DLL%" "%OUT_DIR%" >nul
    echo SDL3.dll copied to output directory.
) else (
    echo WARNING: SDL3.dll not found at "%SDL_DLL%"
)

echo.
echo Done.
endlocal
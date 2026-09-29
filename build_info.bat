@echo off
setlocal
cd /d "%~dp0"

rem Usage: build_info.bat [cpp]
rem   no arg - build with gcc (C)
rem   cpp    - build with g++ (C++)

set CC=gcc
if /i "%~1"=="cpp" set CC=g++

where %CC% >nul 2>nul
if errorlevel 1 (
    echo [ERROR] %CC% not found in PATH
    exit /b 1
)

%CC% -O2 -Wall -o test_info.exe test_info.c info.c -lpsapi -lversion -ladvapi32 -luser32 -lgdi32 -lsetupapi
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo Build OK: test_info.exe (%CC%)
endlocal

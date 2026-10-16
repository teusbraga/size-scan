@echo off
setlocal
echo ========================================================
echo  Compilando SIZE-SCAN PLUS (Win32 Nativo / Ultra-Leve)
echo ========================================================

rem Procurar vcvars64.bat em caminhos comuns do Visual Studio
set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :found_vs

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :found_vs

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :found_vs

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :found_vs

set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :found_vs

echo [ERRO] vcvars64.bat nao encontrado! Verifique a instalacao do MSVC.
pause
exit /b 1

:found_vs
echo Utilizando ambiente: %VS_PATH%
call "%VS_PATH%" >nul 2>&1

echo Compilando recursos e icone...
rc.exe /nologo /i src /fo src\resource.res src\resource.rc

echo Compilando fontes C++ e C...
cl.exe /nologo /O2 /W3 /std:c++17 /EHsc /utf-8 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE ^
    /I.. /Isrc ^
    /Fe:sizescan_plus.exe ^
    src\main.cpp src\native_ui.cpp src\model.cpp src\theme.cpp src\scanner_fast.cpp src\crash_handler.cpp src\i18n.cpp ^
    src\resource.res ^
    user32.lib gdi32.lib gdiplus.lib shell32.lib comctl32.lib uxtheme.lib dwmapi.lib ole32.lib shlwapi.lib ^
    /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /MANIFEST:EMBED /MANIFESTINPUT:sizescan_plus.exe.manifest

if %errorlevel% neq 0 (
    echo.
    echo [ERRO] Falha na compilacao!
    exit /b %errorlevel%
)

del *.obj src\resource.res >nul 2>&1
echo.
echo ========================================================
echo  COMPILACAO CONCLUIDA COM SUCESSO!
echo  Executavel gerado: size-scan-plus\sizescan_plus.exe
echo ========================================================
echo.

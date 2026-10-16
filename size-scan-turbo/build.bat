@echo off
setlocal
echo ========================================================
echo  Compilando SIZE-SCAN TURBO (MFT Direct + x64 ASM + SIMD)
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

echo 1. Compilando modulo em Assembly x64 (ml64.exe)...
ml64.exe /nologo /c /Fosrc\fast_mft.obj src\fast_mft.asm
if %errorlevel% neq 0 (
    echo [ERRO] Falha na montagem do Assembly!
    exit /b %errorlevel%
)

echo 2. Compilando recursos e icone...
rc.exe /nologo /i src /fo src\resource.res src\resource.rc
if %errorlevel% neq 0 (
    echo [ERRO] Falha na compilacao dos recursos!
    exit /b %errorlevel%
)

echo 3. Compilando fontes C++ com AVX2 e otimizacao maxima...
cl.exe /nologo /O2 /arch:AVX2 /W3 /std:c++17 /EHsc /utf-8 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE ^
    /I.. /Isrc ^
    /Fe:sizescan_turbo.exe ^
    src\main.cpp src\native_ui.cpp src\model.cpp src\theme.cpp src\scanner_fast.cpp src\crash_handler.cpp src\i18n.cpp ^
    src\fast_mft.cpp src\mft_reader.cpp ^
    src\fast_mft.obj src\resource.res ^
    user32.lib gdi32.lib gdiplus.lib shell32.lib comctl32.lib uxtheme.lib dwmapi.lib ole32.lib shlwapi.lib advapi32.lib ^
    /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /MANIFEST:EMBED /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"

if %errorlevel% neq 0 (
    echo.
    echo [ERRO] Falha na compilacao C++!
    exit /b %errorlevel%
)

del *.obj src\*.obj src\resource.res >nul 2>&1
echo.
echo ========================================================
echo  COMPILACAO CONCLUIDA COM SUCESSO!
echo  Executavel gerado: size-scan-turbo\sizescan_turbo.exe
echo ========================================================
echo.

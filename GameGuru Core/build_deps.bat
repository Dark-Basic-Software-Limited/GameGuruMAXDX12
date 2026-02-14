@echo off
REM Build all missing third-party library dependencies
REM Uses same MSVC environment as build.bat

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
IF ERRORLEVEL 1 (
    echo ERROR: Could not initialize Visual Studio environment.
    exit /b 1
)
echo MSVC environment ready.

SET OUTDIR=D:\max\GameGuruMAXDX12\GameGuru Core\Dark Basic Public Shared\Lib64\Debug
SET PROJROOT=D:\max\GameGuruMAXDX12\GameGuru Core

REM ============================================================
REM 1. Generate steam_api64.lib from DLL
REM ============================================================
echo.
echo === Building steam_api64.lib ===
dumpbin /exports "%PROJROOT%\SDK\Steamworks SDK\redistributable_bin\win64\steam_api64.dll" > "%TEMP%\steam_exports.txt" 2>&1
if errorlevel 1 (
    echo FAILED: dumpbin steam_api64.dll
) else (
    echo LIBRARY steam_api64 > "%TEMP%\steam_api64.def"
    echo EXPORTS >> "%TEMP%\steam_api64.def"
    REM Parse export names from dumpbin output (column 4 after ordinal, hint, RVA)
    for /f "skip=19 tokens=4" %%a in (%TEMP%\steam_exports.txt) do (
        if not "%%a"=="" (
            echo %%a >> "%TEMP%\steam_api64.def"
        )
    )
    lib /def:"%TEMP%\steam_api64.def" /out:"%OUTDIR%\steam_api64.lib" /machine:x64 >nul 2>&1
    if exist "%OUTDIR%\steam_api64.lib" (
        echo OK: steam_api64.lib created
    ) else (
        echo FAILED: lib command for steam_api64
    )
)

REM ============================================================
REM 2. Generate OptickCore.lib from DLL
REM ============================================================
echo.
echo === Building OptickCore.lib ===
dumpbin /exports "%PROJROOT%\SDK\OPTICK\lib\x64\debug\OptickCore.dll" > "%TEMP%\optick_exports.txt" 2>&1
if errorlevel 1 (
    echo FAILED: dumpbin OptickCore.dll
) else (
    echo LIBRARY OptickCore > "%TEMP%\OptickCore.def"
    echo EXPORTS >> "%TEMP%\OptickCore.def"
    for /f "skip=19 tokens=4" %%a in (%TEMP%\optick_exports.txt) do (
        if not "%%a"=="" (
            echo %%a >> "%TEMP%\OptickCore.def"
        )
    )
    lib /def:"%TEMP%\OptickCore.def" /out:"%OUTDIR%\OptickCore.lib" /machine:x64 >nul 2>&1
    if exist "%OUTDIR%\OptickCore.lib" (
        echo OK: OptickCore.lib created
    ) else (
        echo FAILED: lib command for OptickCore
    )
)

REM ============================================================
REM 3. Generate assimp.lib from DLL
REM ============================================================
echo.
echo === Building assimp.lib ===
dumpbin /exports "%PROJROOT%\SDK\OPTICK\samples\WindowsVulkan\dll\assimp-vc140-mt.dll" > "%TEMP%\assimp_exports.txt" 2>&1
if errorlevel 1 (
    echo FAILED: dumpbin assimp DLL
) else (
    echo LIBRARY assimp-vc140-mt > "%TEMP%\assimp.def"
    echo EXPORTS >> "%TEMP%\assimp.def"
    for /f "skip=19 tokens=4" %%a in (%TEMP%\assimp_exports.txt) do (
        if not "%%a"=="" (
            echo %%a >> "%TEMP%\assimp.def"
        )
    )
    lib /def:"%TEMP%\assimp.def" /out:"%OUTDIR%\assimp.lib" /machine:x64 >nul 2>&1
    if exist "%OUTDIR%\assimp.lib" (
        echo OK: assimp.lib created
    ) else (
        echo FAILED: lib command for assimp
    )
)

REM ============================================================
REM 4. Build DirectXTex
REM ============================================================
echo.
echo === Building DirectXTex ===
msbuild "%PROJROOT%\SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj" /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal 2>&1
if errorlevel 1 (
    echo NOTE: DirectXTex build had errors - trying without PCH
    msbuild "%PROJROOT%\SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj" /p:Configuration=Debug /p:Platform=x64 /p:UsePrecompiledHeader=NotUsing /m /verbosity:minimal 2>&1
)

REM ============================================================
REM 5. Build libogg
REM ============================================================
echo.
echo === Building libogg ===
msbuild "%PROJROOT%\SDK\OGG\libogg\win32\VS2022\libogg.vcxproj" /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal 2>&1

REM ============================================================
REM 6. Build libvorbis_static
REM ============================================================
echo.
echo === Building libvorbis_static ===
msbuild "%PROJROOT%\SDK\OGG\libvorbis\win32\VS2022\libvorbis\libvorbis_static.vcxproj" /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal 2>&1

REM ============================================================
REM 7. Build libvorbisfile_static
REM ============================================================
echo.
echo === Building libvorbisfile_static ===
msbuild "%PROJROOT%\SDK\OGG\libvorbis\win32\VS2022\libvorbisfile\libvorbisfile_static.vcxproj" /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal 2>&1

REM ============================================================
REM 8. Create empty stub libs for Photon SDK (no source available)
REM ============================================================
echo.
echo === Creating Photon stub libs ===
echo. > "%TEMP%\empty.c"
cl /c "%TEMP%\empty.c" /Fo"%TEMP%\empty.obj" >nul 2>&1
lib /out:"%OUTDIR%\Common-cpp_vc14_release_windows_mt_x64.lib" "%TEMP%\empty.obj" /machine:x64 >nul 2>&1
lib /out:"%OUTDIR%\LoadBalancing-cpp_vc14_release_windows_mt_x64.lib" "%TEMP%\empty.obj" /machine:x64 >nul 2>&1
lib /out:"%OUTDIR%\Photon-cpp_vc14_release_windows_mt_x64.lib" "%TEMP%\empty.obj" /machine:x64 >nul 2>&1
echo OK: Photon stub libs created (will need real libs for runtime)

REM ============================================================
REM 9. Create empty stub lib for strmbase (no source available)
REM ============================================================
echo.
echo === Creating strmbase stub lib ===
lib /out:"%PROJROOT%\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\BaseClasses\strmbase.lib" "%TEMP%\empty.obj" /machine:x64 >nul 2>&1
echo OK: strmbase.lib stub created (will need real lib for runtime)

REM ============================================================
REM 10. Download D3DX11.lib from NuGet package
REM ============================================================
echo.
echo === D3DX11.lib ===
echo NOTE: D3DX11.lib requires the legacy DirectX SDK or Microsoft.DXSDK.D3DX NuGet package
echo Creating stub lib for now...
lib /out:"%OUTDIR%\D3DX11.lib" "%TEMP%\empty.obj" /machine:x64 >nul 2>&1
echo OK: D3DX11.lib stub created

echo.
echo === Summary ===
echo Check output above for results. Some libs are stubs that will need
echo real implementations for full runtime functionality.

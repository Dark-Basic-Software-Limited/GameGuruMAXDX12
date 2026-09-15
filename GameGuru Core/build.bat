@echo off
REM =============================================================================
REM build.bat - Build GameGuruWickedMAX from command line using MSVC
REM Place this in your project root (next to GameGuruWickedMAX.sln)
REM
REM Usage:
REM   build.bat                  - Build Debug x64
REM   build.bat Release          - Build Release x64
REM   build.bat Debug             - Build Debug x64
REM   build.bat Release rebuild   - Clean rebuild Release x64
REM   build.bat Debug rebuild     - Clean rebuild Debug x64
REM =============================================================================

SET SOLUTION=GameGuruWickedMAX.sln
SET PLATFORM=x64
SET CONFIG=%1
SET ACTION=%2

IF "%CONFIG%"=="" SET CONFIG=Debug
IF /I "%ACTION%"=="rebuild" (SET TARGET=Rebuild) ELSE (SET TARGET=Build)

REM --- Initialize VS Developer Environment ---
REM GGMAX 3.38: pick the install that EXISTS on this machine rather than hardcoding one - the
REM desktop has VS 2026 (18\Community) and the laptop only has VS 2022 Community.
REM WARNING: do NOT pin this to 2022\Community. On a box that also has a 2022 BuildTools install,
REM both provide toolset v143 at the SAME version, VsDevCmd sets INCLUDE from one while MSBuild
REM resolves the toolset to the other, and the CRT headers are then seen twice:
REM   excpt.h(22,14): error C2011: '_EXCEPTION_DISPOSITION': 'enum' type redefinition
REM That failure is LATENT - it only appears once a file in the affected sub-projects actually
REM recompiles, so an incremental build can look green for weeks.
SET "GG_VSDEV="
IF EXIST "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" SET "GG_VSDEV=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
IF NOT DEFINED GG_VSDEV IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" SET "GG_VSDEV=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
IF NOT DEFINED GG_VSDEV (
    ECHO ERROR: no supported Visual Studio install found ^(looked for 18\Community then 2022\Community^).
    EXIT /B 9
)
call "%GG_VSDEV%" -arch=amd64 >nul 2>&1

REM Capture ATL/MFC paths from VS 2026 before MSBuild overrides VCToolsInstallDir
SET GAMEGURU_ATLMFC_INCLUDE=%VCToolsInstallDir%ATLMFC\include
SET GAMEGURU_ATLMFC_LIB=%VCToolsInstallDir%ATLMFC\lib\x64

IF ERRORLEVEL 1 (
    echo ERROR: Could not initialize Visual Studio environment.
    echo Make sure Visual Studio 2026 Community is installed.
    exit /b 1
)

REM Copy WickedEngine lib to where the linker can find it
SET WICKED_SRC=D:\max\WickedEngineDX12\BUILD\x64\%CONFIG%\WickedEngine_Windows.lib
SET WICKED_DST=Dark Basic Public Shared\Lib64\%CONFIG%\WickedEngine_Windows.lib
if exist "%WICKED_SRC%" (
    copy /Y "%WICKED_SRC%" "%WICKED_DST%" >nul 2>&1
)

echo =============================================
echo  Building %SOLUTION%
echo  Configuration: %CONFIG%
echo  Platform:      %PLATFORM%
echo  Action:        %TARGET%
echo =============================================
echo.

msbuild %SOLUTION% /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /t:%TARGET% /m /verbosity:minimal

IF ERRORLEVEL 1 (
    echo.
    echo =============================================
    echo  BUILD FAILED
    echo =============================================
    exit /b 1
) ELSE (
    echo.
    echo =============================================
    echo  BUILD SUCCEEDED
    echo =============================================
    if exist "D:\max\WickedEngineDX12\refresh_shaders.ps1" (
        echo Refreshing stale engine shader .cso ...
        powershell -NoProfile -ExecutionPolicy Bypass -File "D:\max\WickedEngineDX12\refresh_shaders.ps1"
    )
    exit /b 0
)

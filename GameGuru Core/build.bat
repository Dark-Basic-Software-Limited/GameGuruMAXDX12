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

REM --- Initialize VS 2026 Developer Environment ---
REM Using v143 (VS 2022) toolset as installed
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1

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
    exit /b 0
)

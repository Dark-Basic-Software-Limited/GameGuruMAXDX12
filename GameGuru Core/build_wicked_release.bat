@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
IF ERRORLEVEL 1 (
    echo ERROR: Could not initialize Visual Studio environment.
    exit /b 1
)
echo MSVC environment ready.
echo Building WickedEngine_Windows.lib (Release x64)...
SET SOLDIR=D:\max\WickedEngineDX12\
msbuild "D:\max\WickedEngineDX12\WickedEngine\WickedEngine_Windows.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=%SOLDIR% /m /verbosity:minimal
echo.
echo Exit code: %ERRORLEVEL%
if exist "D:\max\WickedEngineDX12\BUILD\x64\Release\WickedEngine_Windows.lib" (
    echo SUCCESS: WickedEngine_Windows.lib (Release) found
) else (
    echo FAILED: WickedEngine_Windows.lib (Release) not found
)

@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
IF ERRORLEVEL 1 (
    echo ERROR: Could not initialize Visual Studio environment.
    exit /b 1
)
echo MSVC environment ready.
echo Cleaning WickedEngine (Debug x64)...
SET SOLDIR=D:\max\WickedEngineDX12\
msbuild "D:\max\WickedEngineDX12\WickedEngine\WickedEngine_Windows.vcxproj" /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir=%SOLDIR% /t:Clean /m /verbosity:quiet
echo Building WickedEngine_Windows.lib (Debug x64) with /MTd...
SET MTPROPS=D:\max\WickedEngineDX12\wicked_mt_override.props
msbuild "D:\max\WickedEngineDX12\WickedEngine\WickedEngine_Windows.vcxproj" /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir=%SOLDIR% /p:ForceImportAfterCppTargets=%MTPROPS% /m /verbosity:minimal
echo.
echo Exit code: %ERRORLEVEL%
if exist "D:\max\WickedEngineDX12\BUILD\x64\Debug\WickedEngine_Windows.lib" (
    echo SUCCESS: WickedEngine_Windows.lib found
) else (
    echo CHECKING alternate locations...
    dir /s /b "D:\max\WickedEngineDX12\*.lib" 2>nul | findstr /i "WickedEngine"
    echo If no results above, the build did not produce a .lib file.
)

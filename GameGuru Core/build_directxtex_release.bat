@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
echo Building DirectXTex (Release x64)...
msbuild "D:\max\GameGuruMAXDX12\GameGuru Core\SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /verbosity:minimal
echo Exit code: %ERRORLEVEL%
SET LIBPATH=D:\max\GameGuruMAXDX12\GameGuru Core\SDK\DirectXTex\DirectXTex\Bin\Desktop_2022\x64\Release
if exist "%LIBPATH%\DirectXTex.lib" (
    echo SUCCESS: DirectXTex.lib (Release) found
    dir "%LIBPATH%\DirectXTex.lib"
) else (
    echo FAILED: DirectXTex.lib not found at %LIBPATH%
)

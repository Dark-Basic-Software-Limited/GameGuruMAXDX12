@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
SET GAMEGURU_ATLMFC_INCLUDE=%VCToolsInstallDir%ATLMFC\include
SET GAMEGURU_ATLMFC_LIB=%VCToolsInstallDir%ATLMFC\lib\x64
cd /d "D:\max\GameGuruMAXDX12\GameGuru Core"
echo Starting build...
SET WICKED_SRC=D:\max\WickedEngineDX12\BUILD\x64\Release\WickedEngine_Windows.lib
SET WICKED_DST=Dark Basic Public Shared\Lib64\Release\WickedEngine_Windows.lib
if exist "%WICKED_SRC%" (copy /Y "%WICKED_SRC%" "%WICKED_DST%" >nul 2>&1)
msbuild GameGuruWickedMAX.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /verbosity:minimal
echo Exit code: %ERRORLEVEL%

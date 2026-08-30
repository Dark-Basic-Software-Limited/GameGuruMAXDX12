@echo off
cd /D "%~dp0"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
msbuild ogg_vorbis_static.sln /p:Configuration=Release /p:Platform=x64 /p:WholeProgramOptimization=false /t:Rebuild /m /verbosity:minimal

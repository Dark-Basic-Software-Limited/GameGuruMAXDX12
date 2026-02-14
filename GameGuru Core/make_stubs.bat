@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
SET OUTDIR=D:\max\GameGuruMAXDX12\GameGuru Core\Dark Basic Public Shared\Lib64\Debug
echo. > %TEMP%\empty.c
cl /c %TEMP%\empty.c /Fo%TEMP%\empty.obj >nul 2>&1
lib /out:"%OUTDIR%\DirectXTex.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
echo DirectXTex.lib stub
lib /out:"%OUTDIR%\ogg_static.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
echo ogg_static.lib stub
lib /out:"%OUTDIR%\vorbis_static.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
echo vorbis_static.lib stub
lib /out:"%OUTDIR%\vorbisfile_static.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
echo vorbisfile_static.lib stub
echo DONE

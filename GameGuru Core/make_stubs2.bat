@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
SET OUTDIR=D:\max\GameGuruMAXDX12\GameGuru Core\Dark Basic Public Shared\Lib64\Debug
echo. > %TEMP%\empty.c
cl /c %TEMP%\empty.c /Fo%TEMP%\empty.obj >nul 2>&1
lib /out:"%OUTDIR%\Common-cpp_vc14_debug_windows_mt_x64.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
lib /out:"%OUTDIR%\LoadBalancing-cpp_vc14_debug_windows_mt_x64.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
lib /out:"%OUTDIR%\Photon-cpp_vc14_debug_windows_mt_x64.lib" %TEMP%\empty.obj /machine:x64 >nul 2>&1
echo Debug Photon stubs created

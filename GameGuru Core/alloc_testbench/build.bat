@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
cl /nologo /O2 /EHsc /D_ALLOW_KEYWORD_MACROS main.cpp offsetAllocator.cpp /Fe:harness.exe

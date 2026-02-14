# GameGuruWickedMAX - Claude Code Instructions

## Project Overview
This is a large C++ Windows x64 project (game engine) built with MSVC.
Solution file: `GameGuruWickedMAX.sln`

## Build Commands
Invoke `build.bat` using its full quoted path (required because the project root contains a space):

```
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Debug          # Build Debug x64
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Release        # Build Release x64
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Debug rebuild  # Clean rebuild Debug x64
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Release rebuild # Clean rebuild Release x64
```

## Build System
- **Compiler**: MSVC v143 (VS 2022 toolset) via Visual Studio 2026 Community
- **Platform**: x64 only
- **Solution**: MSBuild-based (.sln/.vcxproj)
- **VS Install Path**: C:\Program Files\Microsoft Visual Studio\18\Community

## File Structure Notes
- Large .cpp files have been split into `_partN.cpp` files (e.g., `Entity_part0.cpp`, `Entity_part1.cpp`)
- When fixing compiler errors in split files, check if missing includes or forward declarations need to be added to the top of each part file
- Original header files (.h) are NOT split

## Important
- Always use `build.bat` to compile — it sets up the MSVC environment automatically
- Build errors will use the standard MSVC format and appear in the terminal
- The `$msCompile` problem matcher is configured in VS Code tasks

## Third-Party Dependencies
- **WickedEngineDX12** is located at `../WickedEngineDX12` (sibling folder at `D:\max\WickedEngineDX12`)
- This is the rendering engine the project depends on
- Reference this repo when resolving includes, engine API calls, or tracking down type definitions
- Do NOT modify files in WickedEngineDX12 unless explicitly asked

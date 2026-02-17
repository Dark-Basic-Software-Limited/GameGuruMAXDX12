// GGEngineGlobals.hlsli - Trampoline to include new engine globals.
//
// Uses angle-bracket include (<>) to bypass DXC's "directory stack" behavior.
// With quoted includes (""), DXC searches all parent includer directories before
// AdditionalIncludeDirectories. This means #include "globals.hlsli" would find
// PBR/globals.hlsli when this file is included from PBR/ShaderInterop_Renderer.h.
// Angle brackets skip the directory stack and go directly to include paths,
// where the engine's shaders directory comes first.
//
// The engine's globals.hlsli provides:
//   - ShaderInterop.h (CBUFFER, CONSTANTBUFFER macros)
//   - ShaderInterop_Renderer.h (FrameCB, CameraCB, ShaderEntity, ShaderScene, etc.)
//   - GetFrame(), GetCamera(), GetScene(), GetWeather() accessor macros
//   - load_entity(), load_entitymatrix(), load_entitytile() bindless accessors
//   - lights(), probes(), decals() etc. ShaderEntityIterator factories
//   - Sampler declarations (sampler_linear_clamp, etc.)
//   - Bindless resource array declarations
//   - Utility functions (pack/unpack, blue_noise, tonemap, etc.)

#ifndef GG_ENGINE_GLOBALS_HLSLI
#define GG_ENGINE_GLOBALS_HLSLI

// Disable half precision for GG shaders - our code uses float everywhere
#define DISABLE_HALF_PRECISION

// Use angle brackets to force resolution via AdditionalIncludeDirectories only,
// bypassing DXC's directory stack (which would find PBR/globals.hlsli).
#include <globals.hlsli>

#endif // GG_ENGINE_GLOBALS_HLSLI

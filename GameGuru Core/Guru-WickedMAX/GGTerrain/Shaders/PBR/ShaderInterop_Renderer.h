// PBR/ShaderInterop_Renderer.h - Redirect to new engine + GG compatibility.
//
// If the engine's ShaderInterop_Renderer.h was already included (via globals.hlsli),
// the redirect section is skipped. GG-specific additions are under separate guards.

#ifndef WI_SHADERINTEROP_RENDERER_H
// Pull in the new engine's ShaderInterop_Renderer.h which provides:
//   FrameCB, CameraCB, ShaderEntity, ShaderScene, ShaderWeather, ShaderCamera,
//   ShaderEntityIterator, OPTION_BIT_* enum, ENTITY_TYPE_* enum, etc.
#include "GGEngineGlobals.hlsli"
#endif

// ============================================================================
// GG-specific OPTION_BIT values not in the new engine's enum.
// The C++ side must set these bits in FrameCB.options for them to have effect.
// ============================================================================
#ifndef GG_OPTION_BITS_COMPAT
#define GG_OPTION_BITS_COMPAT

// OPTION_BIT_SIMPLE_SKY: bit 5 is unused in the new engine
static const uint OPTION_BIT_SIMPLE_SKY = 1 << 5;

// OPTION_BIT_TRANSPARENTSHADOWS_ENABLED: bit 1 is commented out in the new engine
static const uint OPTION_BIT_TRANSPARENTSHADOWS_ENABLED = 1 << 1;

// OPTION_BIT_WATER_ENABLED: pick unused bit 22 (old was 11 but new uses 11 for DISABLE_ALBEDO_MAPS)
static const uint OPTION_BIT_WATER_ENABLED = 1 << 22;

// VoxelGI reflections - not in new engine (voxel system removed)
static const uint OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED = 1 << 21;

// Precipitation bits - not in new engine, pick unused bits 23-25
static const uint OPTION_BIT_SNOW_ENABLED = 1 << 23;
static const uint OPTION_BIT_DUST_ENABLED = 1 << 24;
static const uint OPTION_BIT_RAIN_ENABLED = 1 << 25;

#endif // GG_OPTION_BITS_COMPAT

// ============================================================================
// GG-specific constant buffers that remain unchanged.
// These are "on demand" CBs at higher slots (b5-b10) still bound by GG C++ code.
// ============================================================================
#ifndef GG_ONDEMAND_CB_COMPAT
#define GG_ONDEMAND_CB_COMPAT

// MaterialCB and related CBs - used by GG object shaders.
// ShaderMaterial in the new engine is a completely different struct (bindless, packed half).
// For GG custom shaders that reference g_xMaterial, we provide a minimal compat struct.
// NOTE: Most GG terrain/tree/grass shaders do NOT use MaterialCB - they use custom CBs.

// ForwardEntityMaskCB - used by env probe shaders via GGForwardLighting
#ifndef CBSLOT_RENDERER_FORWARD_LIGHTMASK
// Already defined by engine's ShaderInterop.h (=2), but just in case:
#define CBSLOT_RENDERER_FORWARD_LIGHTMASK 2
#endif

// CubemapRenderCB slot define for backward compat (even though CB itself is removed)
#ifndef CBSLOT_RENDERER_CUBEMAPRENDER
#define CBSLOT_RENDERER_CUBEMAPRENDER 8
#endif

#endif // GG_ONDEMAND_CB_COMPAT

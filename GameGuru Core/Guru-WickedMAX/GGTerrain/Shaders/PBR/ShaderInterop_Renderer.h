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

// OPTION_BIT_TRANSPARENTSHADOWS_ENABLED: REMOVED in GGMAX 3.34.
//
// 3.33 un-commented this bit in the engine enum (WickedEngine/shaders/ShaderInterop_Renderer.h)
// so the transparent-shadow fetch could be gated at runtime instead of only at compile time.
// This shim redefined the same name as a static const uint, and any GGTerrain shader that pulls
// in both headers then fails to compile: "redefinition of ... as different kind of symbol".
//
// The engine enum is now the single definition. Nothing here needs to declare it.
//
// ⚠ This did not show up when 3.33 landed, which is worth knowing: an engine-header change can
// break the GGTerrain shader tree while every engine .cso still builds clean, and a sweep run on
// stale GGTerrain .cso files will pass anyway. Prove an engine shader-header edit with a GAME
// build before trusting a sweep that follows it.

// OPTION_BIT_WATER_ENABLED REMOVED (was 1<<22): the old GG underwater fog it gated is retired —
// Wicked's underwaterCS owns underwater now. Bit 22 aliases the engine's OPTION_BIT_DEBUG_NORMAL_VIS,
// so keeping this define let the 'I' debug key accidentally trigger old GG water fog. Do NOT re-add
// a water bit here; if a GG-owned frame flag is ever needed, use ggCustomFrame.ggOptions (b4), not
// the engine FrameCB.options. See GGCommonFunctions.hlsli (ApplyFogCustom).

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

// PBR/ConstantBufferMapping.h - Redirect to new engine CB slot defines.
//
// The new engine's ShaderInterop.h (included via globals.hlsli) already defines:
//   CBSLOT_RENDERER_FRAME = 0
//   CBSLOT_RENDERER_CAMERA = 1
//   CBSLOT_RENDERER_FORWARD_LIGHTMASK = 2
//
// This file is kept for backward compatibility with GG shaders that
// include it directly. Only provides the CUBEMAPRENDER slot which
// no longer exists in the new engine.

#ifndef WI_CONSTANTBUFFER_MAPPING_H
#define WI_CONSTANTBUFFER_MAPPING_H

// Pull in engine defines if not already available
#ifndef CBSLOT_RENDERER_FRAME
#include "GGEngineGlobals.hlsli"
#endif

// GG-specific: CubemapRenderCB slot (used by env probe shaders for backward compat)
#ifndef CBSLOT_RENDERER_CUBEMAPRENDER
#define CBSLOT_RENDERER_CUBEMAPRENDER 8
#endif

#endif // WI_CONSTANTBUFFER_MAPPING_H

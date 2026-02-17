// PBR/ShaderInterop.h - Redirect to new engine + GG slot-based resource macros.
//
// If the new engine's ShaderInterop.h was already included (via globals.hlsli),
// the redirect is skipped. The GG resource macros are under a separate guard
// so they're always available.

#ifndef WI_SHADERINTEROP_H
// Pull in the new engine's ShaderInterop.h via the trampoline/include path.
// This provides CBUFFER, CONSTANTBUFFER, PUSHCONSTANT, CBSLOT_* defines.
#include "GGEngineGlobals.hlsli"
#endif

// GG-specific slot-based resource declaration macros.
// The new engine doesn't need these (100% bindless), but GG custom shaders
// use explicit SRV slots for textures at t50-58.
#ifndef GG_RESOURCE_MACROS
#define GG_RESOURCE_MACROS

// Token-pasting helpers for DXC
#define _GG_REG_B(slot) register(b ## slot)
#define _GG_REG_T(slot) register(t ## slot)
#define _GG_REG_U(slot) register(u ## slot)
#define _GG_REG_S(slot) register(s ## slot)

#define TEXTURE2D(name, type, slot) Texture2D< type > name : _GG_REG_T(slot)
#define TEXTURE2DMS(name, type, slot) Texture2DMS< type > name : _GG_REG_T(slot)
#define TEXTURE2DARRAY(name, type, slot) Texture2DArray< type > name : _GG_REG_T(slot)
#define TEXTURECUBE(name, type, slot) TextureCube< type > name : _GG_REG_T(slot)
#define TEXTURECUBEARRAY(name, type, slot) TextureCubeArray< type > name : _GG_REG_T(slot)
#define TEXTURE3D(name, type, slot) Texture3D< type > name : _GG_REG_T(slot)
#define TEXTURE1D(name, type, slot) Texture1D< type > name : _GG_REG_T(slot)
#define TEXTURE1DARRAY(name, type, slot) Texture1DArray< type > name : _GG_REG_T(slot)
#define RWTEXTURE2D(name, type, slot) RWTexture2D< type > name : _GG_REG_U(slot)
#define RWTEXTURE2DARRAY(name, type, slot) RWTexture2DArray< type > name : _GG_REG_U(slot)
#define RWTEXTURE3D(name, type, slot) RWTexture3D< type > name : _GG_REG_U(slot)
#define RWTEXTURE1D(name, type, slot) RWTexture1D< type > name : _GG_REG_U(slot)
#define ROVTEXTURE2D(name, type, slot) RasterizerOrderedTexture2D< type > name : _GG_REG_U(slot)
#define ROVTEXTURE3D(name, type, slot) RasterizerOrderedTexture3D< type > name : _GG_REG_U(slot)

#define RAWBUFFER(name,slot) ByteAddressBuffer name : _GG_REG_T(slot)
#define RWRAWBUFFER(name,slot) RWByteAddressBuffer name : _GG_REG_U(slot)
#define TYPEDBUFFER(name, type, slot) Buffer< type > name : _GG_REG_T(slot)
#define RWTYPEDBUFFER(name, type, slot) RWBuffer< type > name : _GG_REG_U(slot)
#define STRUCTUREDBUFFER(name, type, slot) StructuredBuffer< type > name : _GG_REG_T(slot)
#define RWSTRUCTUREDBUFFER(name, type, slot) RWStructuredBuffer< type > name : _GG_REG_U(slot)
#define ROVSTRUCTUREDBUFFER(name, type, slot) RasterizerOrderedStructuredBuffer< type > name : _GG_REG_U(slot)

#define SAMPLERSTATE(name, slot) SamplerState name : _GG_REG_S(slot)
#define SAMPLERCOMPARISONSTATE(name, slot) SamplerComparisonState name : _GG_REG_S(slot)

#endif // GG_RESOURCE_MACROS

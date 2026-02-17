// GGRootSignature.hlsli - DX12 Root Signature for GameGuru custom shaders
//
// This defines GAMEGURU_ROOTSIGNATURE, which is identical to the Wicked Engine
// default root signature (WICKED_ENGINE_DEFAULT_ROOTSIGNATURE in globals.hlsli)
// but with SRV numDescriptors = 64 (instead of the original 16) to support
// the custom terrain/grass/tree texture slots at t50-t57.
//
// Usage in custom shaders:
//   #include "GGRootSignature.hlsli"
//   [RootSignature(GAMEGURU_ROOTSIGNATURE)]
//   float4 main(...) : SV_Target { ... }
//
// The DXC compiler embeds this root signature in the .cso bytecode, which
// the DX12 backend extracts via D3D12CreateVersionedRootSignatureDeserializer.
//
// IMPORTANT: DESCRIPTORBINDER_SRV_COUNT in wiGraphicsDevice.h must also be 64
// for the C++ BindResource() calls to work with slots >= 16.

#ifndef GG_ROOTSIGNATURE_HLSLI
#define GG_ROOTSIGNATURE_HLSLI

#define GAMEGURU_ROOTSIGNATURE \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=12, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"CBV(b2), " \
	"DescriptorTable( " \
		"CBV(b3, numDescriptors = 11, flags = DATA_STATIC_WHILE_SET_AT_EXECUTE)," \
		"SRV(t0, numDescriptors = 64, flags = DESCRIPTORS_VOLATILE | DATA_STATIC_WHILE_SET_AT_EXECUTE)," \
		"UAV(u0, numDescriptors = 16, flags = DESCRIPTORS_VOLATILE | DATA_STATIC_WHILE_SET_AT_EXECUTE)" \
	")," \
	"DescriptorTable( " \
		"Sampler(s0, offset = 0, numDescriptors = 16, flags = DESCRIPTORS_VOLATILE)" \
	")," \
	"DescriptorTable( " \
		"Sampler(s0, space = 1, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE)" \
	")," \
	"DescriptorTable( " \
		"SRV(t0, space = 2, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 3, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 4, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 5, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 6, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 7, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 8, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 9, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 10, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 11, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 12, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 13, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 14, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 15, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 16, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 17, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 18, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 19, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 20, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 21, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 22, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 23, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 24, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 25, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 26, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 27, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 28, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 29, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 30, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 100, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 101, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 102, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 103, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 104, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 105, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 106, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 107, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 108, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 109, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 110, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 111, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 112, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 113, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 114, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 115, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 200, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 201, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 202, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 203, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 204, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 205, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 206, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 207, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 208, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)" \
	"), " \
	"StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s101, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s102, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s103, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s104, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s105, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s106, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
	"StaticSampler(s107, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
	"StaticSampler(s108, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
	"StaticSampler(s109, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"

// For shaders that include globals.hlsli (which defines WICKED_ENGINE_DEFAULT_ROOTSIGNATURE),
// override it so the DXC -rootsig-define mechanism picks up our expanded version:
#ifdef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE
#undef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE
#endif
#define WICKED_ENGINE_DEFAULT_ROOTSIGNATURE GAMEGURU_ROOTSIGNATURE

#endif // GG_ROOTSIGNATURE_HLSLI

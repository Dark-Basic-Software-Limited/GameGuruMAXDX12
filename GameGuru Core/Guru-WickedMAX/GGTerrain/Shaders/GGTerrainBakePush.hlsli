// GGTerrainBakePush.hlsli - GGMAX 3.115: the Terrain Bake draw's per-chunk push constants.
//
// 48 bytes = the GG root signature's 12 root constants (RootConstants b999, GGRootSignature.hlsli).
// ★ Must match struct BakePush in GGTerrainBake.cpp byte for byte (static_assert there).
// Pushed after BindPipelineState for every chunk, prepass included (its PS simply ignores b999).
//
// Every descriptor here is derived from a handle the bake OWNS (the chunk's BC1 texture, the retained
// blendmap) or from this frame's transient layer table, so none can outlive what it points at; -1
// switches a feature off, never 0 (0 is a real bindless slot).

#ifndef GG_TERRAINBAKEPUSH_HLSLI
#define GG_TERRAINBAKEPUSH_HLSLI

struct GGTerrainBakePush
{
	int   bakedTex;          // the chunk's baked BC1_UNORM_SRGB texture (bindless SRV)
	int   blendmap;          // the chunk's retained blendmap Texture2DArray (bindless SRV); -1 = no near detail
	int   layerTable;        // this frame's layer table, a ByteAddressBuffer (bindless SRV); -1 = none
	uint  layerTableOffset;  // byte offset of entry 0 in that buffer
	uint  layerCount;        // blendmap array size = layers to visit
	float bakeRes;           // the chunk's baked texture resolution, for the bake's own LOD
	float fadeLodEnd;        // near detail is gone at bake LOD >= fadeLodEnd ...
	float fadeRcpWidth;      // ... and full at fadeLodEnd - 1/fadeRcpWidth
	uint  flags;             // GG_BAKEPUSH_*
	uint  pad0, pad1, pad2;
};

#define GG_BAKEPUSH_SPLAT      0x01u   // wi::terrain::gg_splat_blend: sum(w*c)/sum(w), else the decal stack
#define GG_BAKEPUSH_FORCEFULL  0x02u   // debug: detail weight 1 everywhere it can run (the sharpness ceiling)
#define GG_BAKEPUSH_FORCENONE  0x04u   // debug: detail weight 0 (the bake alone, through the same code path)

// Layer table entry (32 bytes), one per blendmap layer = terrain->materialEntities[i]:
//   uint4  { colourSRV (sRGB view) or -1, normalSRV or -1, asuint(repeats per chunk), valid }
//   float4 baseColor (the MaterialComponent's, multiplies the colour map as the bake did)
#define GG_BAKE_LAYER_STRIDE 32

#endif // GG_TERRAINBAKEPUSH_HLSLI

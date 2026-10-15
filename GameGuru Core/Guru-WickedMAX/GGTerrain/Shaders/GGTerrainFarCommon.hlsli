// =============================================================================================
// GGMAX 3.114 - FAR TERRAIN (the DX11 horizon). Shared by GGTerrainFarVS / PrepassPS / PS.
//
// The live Wicked terrain is a ring of chunks around the camera that ends ~63,360 units out;
// DX11 drew terrain to its 500,000-unit far plane. The far field draws the OUTER levels of the
// legacy GGTerrain clipmap - DX11's own terrain geometry, which DX12 still generates every frame -
// everywhere the live (or baked) chunks are not. The coverage mask below is the set of chunks that
// actually exist (not an analytic square: the ring lingers after travel, has holes while it fills,
// and a bake's chunk set does not follow the camera).
//
// ★ The interior test MUST be identical in the prepass and the colour pass (both include this
//   file; the VS is one compiled shader shared by both PSOs) - a coverage difference between the
//   two passes writes depth without colour, the BLACK-terrain failure of 3.04-3.06.
// =============================================================================================

#ifndef GG_TERRAINFAR_COMMON_HLSLI
#define GG_TERRAINFAR_COMMON_HLSLI

cbuffer GGTerrainFarCB : register( b5 )
{
	uint4  far_mask[ 32 ];        // 64x64 bits, cell (i,j) = chunk (origin.x + i, origin.y + j); bit = j*64 + i
	int2   far_maskOrigin;
	float  far_chunkSize;         // live chunk stride, 5280
	float  far_segBase;           // legacy clipmap segment size at LOD 0 (vertex spacing doubles per level)
	float  far_sinkMul;           // border-cell sink, in multiples of the vertex's own segment size
	uint   far_flags;             // bits 8..11 = debug view
	float  far_texScale;          // world units -> material UV (the live terrain's 32 repeats per chunk)
	float  far_pad0;
	float4 far_layerStart[ 2 ];   // GG height layers 0..4 (x,y,z,w then x)
	float4 far_layerRcp[ 2 ];     // 1 / (end - start), 0 = unused layer
	float4 far_slope;             // start0, rcp0, start1, rcp1  (on 1 - |N.y|)
	uint4  far_slots[ 2 ];        // [0] base, low, layer1, high   [1].x slope   (texColorArray slices)
};

#define FAR_DEBUG_VIEW ( ( far_flags >> 8 ) & 0xF )

bool FarCovered( int2 c )
{
	const int2 m = c - far_maskOrigin;
	if ( any( m < 0 ) || any( m >= 64 ) ) return false;
	const uint bit = (uint)( m.y * 64 + m.x );
	const uint4 v = far_mask[ bit >> 7 ];
	const uint u = v[ ( bit >> 5 ) & 3 ];
	return ( ( u >> ( bit & 31 ) ) & 1 ) != 0;
}

// wiTerrain convention: chunk c is CENTRED at c * stride.
int2 FarCell( float2 xz )
{
	return (int2)floor( xz / far_chunkSize + 0.5 );
}

// Interior = this cell and all 8 neighbours are live chunks: the far field is discarded there.
// Border cells (covered, but next to an uncovered one) keep the far field, sunk under the live
// chunk by the VS, as a skirt that closes any crack where the two meshes approximate the same
// height function differently.
bool FarInterior( float2 xz )
{
	const int2 c = FarCell( xz );
	[unroll] for ( int dz = -1; dz <= 1; dz++ )
	{
		[unroll] for ( int dx = -1; dx <= 1; dx++ )
		{
			if ( !FarCovered( c + int2( dx, dz ) ) ) return false;
		}
	}
	return true;
}

#endif // GG_TERRAINFAR_COMMON_HLSLI

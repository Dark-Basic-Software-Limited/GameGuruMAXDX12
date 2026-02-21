#include "GGRootSignature.hlsli"
Texture2D texColorAndMetalness   : register( t50 );
Texture2D texNormalRoughnessAO   : register( t51 );
							     
Texture2DArray<float> texPageTableArray : register( t53 );
Texture2D<float> texPageTableFinal      : register( t54 );
Texture2D<float> texMaterialMap          : register( t55 );
Texture2D<float> texGrassMap             : register( t56 );
Texture2D<float> texTreeMap              : register( t57 );

SamplerState sampler1            : register( s1 );

#include "GGTerrainConstants.hlsli"

#include "../GGTerrainPageSettings.h"

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"

#include "GGCommonFunctions.hlsli"

// EntityTiles now accessed via load_entitytile() from engine globals

// Enable envmap ambient output variant for terrain PBR
#define GGLIGHTING_ENVMAP_AMBIENT
#include "GGLighting.hlsli"

static const float4 mipColors[16] = {
	float4( 1.0, 0.0, 0.0, 1.0 ),
	float4( 0.0, 1.0, 0.0, 1.0 ),
	float4( 0.0, 0.0, 1.0, 1.0 ),
	float4( 1.0, 1.0, 0.0, 1.0 ),
	float4( 0.0, 1.0, 1.0, 1.0 ),
	float4( 1.0, 0.0, 1.0, 1.0 ),
	float4( 1.0, 1.0, 1.0, 1.0 ),
	float4( 0.5, 0.0, 0.0, 1.0 ),
	float4( 0.0, 0.5, 0.0, 1.0 ),
	float4( 0.0, 0.0, 0.5, 1.0 ),
	float4( 0.5, 0.5, 0.0, 1.0 ),
	float4( 0.0, 0.5, 0.5, 1.0 ),
	float4( 0.5, 0.0, 0.5, 1.0 ),
	float4( 0.5, 0.5, 0.5, 1.0 ),
	float4( 0.25, 0.25, 0.25, 1.0 ),
	float4( 0.0, 0.0, 0.0, 1.0 ),
};

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD1;
	float lodLevel: TEXCOORD2;
	float3 normal : TEXCOORD3;
	float clip : SV_ClipDistance0;
	float2 uv : TEXCOORD4;
};

struct GBuffer
{
	float4 g0 : SV_TARGET0;	/*FORMAT_R11G11B10_FLOAT*/
	float4 g1 : SV_TARGET1;	/*FORMAT_R8G8B8A8_FLOAT*/
};

// Reference color palette: 32 distinctive flat colors for material visualization
static const float3 referenceColors[32] = {
	float3( 0.18, 0.55, 0.15 ),  //  0: dark green (base/grass)
	float3( 0.95, 0.85, 0.25 ),  //  1: yellow (sand)
	float3( 0.76, 0.70, 0.50 ),  //  2: tan/khaki (sandy rock)
	float3( 0.10, 0.40, 0.10 ),  //  3: forest green (dense vegetation)
	float3( 0.55, 0.55, 0.55 ),  //  4: grey (rock/cliff)
	float3( 0.90, 0.30, 0.10 ),  //  5: red-orange
	float3( 0.20, 0.60, 0.85 ),  //  6: sky blue
	float3( 0.80, 0.20, 0.60 ),  //  7: magenta
	float3( 0.95, 0.60, 0.10 ),  //  8: orange
	float3( 0.30, 0.80, 0.70 ),  //  9: teal
	float3( 0.70, 0.10, 0.10 ),  // 10: dark red
	float3( 0.50, 0.85, 0.20 ),  // 11: lime green
	float3( 0.35, 0.20, 0.70 ),  // 12: purple
	float3( 0.90, 0.75, 0.55 ),  // 13: peach
	float3( 0.10, 0.35, 0.55 ),  // 14: navy
	float3( 0.85, 0.85, 0.10 ),  // 15: bright yellow
	float3( 0.60, 0.30, 0.15 ),  // 16: brown
	float3( 0.40, 0.80, 0.40 ),  // 17: medium green
	float3( 0.75, 0.45, 0.75 ),  // 18: orchid
	float3( 0.25, 0.65, 0.45 ),  // 19: sea green
	float3( 0.90, 0.45, 0.45 ),  // 20: salmon
	float3( 0.45, 0.45, 0.80 ),  // 21: slate blue
	float3( 0.80, 0.65, 0.20 ),  // 22: goldenrod
	float3( 0.55, 0.85, 0.85 ),  // 23: light cyan
	float3( 0.85, 0.35, 0.35 ),  // 24: indian red
	float3( 0.35, 0.70, 0.20 ),  // 25: olive green
	float3( 0.65, 0.20, 0.45 ),  // 26: dark magenta
	float3( 0.95, 0.70, 0.40 ),  // 27: sandy brown
	float3( 0.20, 0.50, 0.70 ),  // 28: steel blue
	float3( 0.70, 0.70, 0.30 ),  // 29: dark khaki
	float3( 0.50, 0.30, 0.50 ),  // 30: plum
	float3( 0.80, 0.80, 0.80 ),  // 31: light grey
};

// virtual texture variables
static const float2 virtToPageSize = float2( pageSize / physTexSizeX, pageSize / physTexSizeY );
static const float2 texelOffset = float2( pagePaddingLeft / physTexSizeX, pagePaddingLeft / physTexSizeY );

GBuffer main( PixelIn IN )
{
	GBuffer output;

	// Reference color mode: bypass virtual texture, show flat material colors
	if ( terrain_flags & GGTERRAIN_SHADER_FLAG2_REFERENCE_COLOR )
	{
		// Sample material map using world position
		float2 matUV = IN.worldPos.xz / terrain_mapEditSize * 0.5 + 0.5;
		float matSample = texMaterialMap.SampleLevel( sampler1, matUV, 0 );
		uint matIndex = (uint)(matSample * 255.0);

		// If unpainted (0), determine material from height/slope layer rules
		if ( matIndex == 0 )
		{
			float height = IN.worldPos.y;
			matIndex = terrain_baseLayerMaterial & 0xFF;

			// Height layers (same logic as page gen shader)
			for ( int i = 0; i < 5; i++ )
			{
				float t = clamp( (height - terrain_layers[i].start) * terrain_layers[i].transition, 0.0, 1.0 );
				if ( t >= 1 ) matIndex = terrain_layers[i].material & 0xFF;
			}

			// Slope layers
			float normaly = 1 - abs(IN.normal.y);
			for ( int j = 0; j < 2; j++ )
			{
				float t = clamp( (normaly - terrain_slopes[j].start) * terrain_slopes[j].transition, 0.0, 1.0 );
				if ( t >= 1 ) matIndex = terrain_slopes[j].material & 0xFF;
			}
		}
		else
		{
			matIndex = matIndex - 1; // convert from 1-based to 0-based
		}

		// Look up flat color from palette
		float3 refColor = referenceColors[matIndex & 31];

		// Apply simple ambient so terrain shape is visible
		float ambient = saturate( dot(IN.normal, float3(0, 1, 0)) * 0.5 + 0.5 );
		refColor *= ambient;

		// Grass map overlay
		if ( terrain_flags & GGTERRAIN_SHADER_FLAG2_SHOW_GRASS_MAP )
		{
			float2 vegUV = IN.worldPos.xz / terrain_mapEditSize * 0.5 + 0.5;
			float grassSample = texGrassMap.SampleLevel( sampler1, vegUV, 0 );
			uint grassType = (uint)(grassSample * 255.0);
			if ( grassType > 0 )
			{
				float3 grassColor = referenceColors[grassType & 31] * float3(0.5, 1.0, 0.5);
				grassColor *= ambient;
				refColor = lerp( refColor, grassColor, 0.7 );
			}
		}

		// Tree map overlay
		if ( terrain_flags & GGTERRAIN_SHADER_FLAG2_SHOW_TREE_MAP )
		{
			float2 vegUV = IN.worldPos.xz / terrain_mapEditSize * 0.5 + 0.5;
			float treeSample = texTreeMap.SampleLevel( sampler1, vegUV, 0 );
			uint treeType = (uint)(treeSample * 255.0);
			if ( treeType > 0 )
			{
				float3 treeColor = referenceColors[treeType & 31] * ambient;
				refColor = treeColor;
			}
		}

		float4 color = float4( refColor, 1 );

		// Editor overlays (brush circle, map border) still apply
		if ( terrain_flags & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE )
		{
			float3 editColor = float3( 244.0/255.0, 239.0/255.0, 38.0/255.0 );
			float editX = (IN.worldPos.x / terrain_mapEditSize) * 0.5 + 0.5;
			float editZ = (IN.worldPos.z / terrain_mapEditSize) * 0.5 + 0.5;

			if ( editX >= 0 && editX <= 1 && editZ >= 0 && editZ <= 1 )
			{
				float fade = editX + editZ;
				fade = sin( fade * 150 + g_xFrame_Time*2 ) * 0.5 + 0.5;
				fade = step( fade, 0.5 );
				if ( editX > 0.004 && editX < 0.996 && editZ > 0.004 && editZ < 0.996 ) fade = 0;
				color.rgb = lerp( color.rgb, editColor, fade );
			}
		}

		if ( (terrain_flags & GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE) && !any(g_xCamera_ClipPlane) )
		{
			float3 brushColor = float3( 37.0/255.0, 245.0/255.0, 43.0/255.0 );
			float2 brushPos = IN.worldPos.xz - terrain_mouseHit.xy;
			float sqrDist = dot( brushPos, brushPos );
			float border = terrain_brushSize * 0.02;
			if ( border > 20 ) border = 20;
			if ( border < 1 ) border = 1;
			float minDist = terrain_brushSize - border;
			float maxDist = terrain_brushSize + border;

			if ( sqrDist < maxDist*maxDist && sqrDist > minDist*minDist )
			{
				color.rgb = brushColor;
			}
		}

		color = max( 0, color );
		output.g0 = float4( color.rgb, 1 );
		output.g1 = float4( IN.normal * 0.5f + 0.5f, 1 );
		return output;
	}

	// page table look up
	int maxLevel = terrain_numLODLevels - 1; 
	int detailLevel = texPageTableFinal.CalculateLevelOfDetailUnclamped( sampler1, IN.uv );
	detailLevel = max( detailLevel, IN.lodLevel );
	int origLOD = detailLevel;
	detailLevel = min( detailLevel, maxLevel + GGTERRAIN_MAX_PAGE_TABLE_MIP - 1 );
	float pageEntry = 0;
	float2 levelUV = float2(0,0);
	
	// go through the page table array until we find a suitable page
	while( pageEntry == 0 && detailLevel < maxLevel )
	{
		levelUV = IN.worldPos.xz - float2( terrain_LOD[ detailLevel ].x, terrain_LOD[ detailLevel ].z );
		levelUV *= terrain_LOD[ detailLevel ].size;
		levelUV.y = 1 - levelUV.y;
		
		int2 loadUV = levelUV * 256.0; // assumes texPageTableArray is 256 x 256 pixels
		pageEntry = texPageTableArray.Load( int4(loadUV, detailLevel, 0) ); // uv, arrayLevel, mipLevel
		
		detailLevel++;
	}

	uint mipLevel = max( detailLevel - maxLevel, 0 );

	// if still no page then go through the final page table mipmaps
	if ( pageEntry == 0 )
	{
		levelUV = IN.worldPos.xz - float2( terrain_LOD[ maxLevel ].x, terrain_LOD[ maxLevel ].z );
		levelUV *= terrain_LOD[ maxLevel ].size;
		levelUV.y = 1 - levelUV.y;

		uint mipSize = 256 >> mipLevel; // assumes texPageTableFinal is 256 x 256 pixels
		int2 loadUV = levelUV * mipSize; 
		
		while( pageEntry == 0 && mipLevel < GGTERRAIN_MAX_PAGE_TABLE_MIP ) // not a full mip stack
		{
			pageEntry = texPageTableFinal.Load( int3(loadUV, mipLevel) ); // uv, mipLevel
			loadUV >>= 1;
			mipLevel++;
			detailLevel++;
		}
		mipLevel--;
	}
	
	uint iPageEntry = (uint) (pageEntry * 65535);

	uint mipSize;
	if ( iPageEntry == 0 )
	{
		detailLevel = maxLevel + GGTERRAIN_MAX_PAGE_TABLE_MIP - 1;
		mipSize = 1;
	}
	else
	{
		detailLevel--;
		mipSize = 256 >> mipLevel;
	}
	
	// calculate UV for physical (page cache) texture
	uint iOffsetX = iPageEntry & 0xFF;
	uint iOffsetY = iPageEntry >> 8;
	float2 pageOffset = float2(iOffsetX / physPagesX, iOffsetY / physPagesY);

	float2 pageUV = frac( levelUV * mipSize ) * virtToPageSize + texelOffset;
	
	pageUV += pageOffset;

	float uvScale = 256.0 / (1 << detailLevel);
	float2 dx = ddx( IN.uv ) * (uvScale / physTexSizeX);
	float2 dy = ddy( IN.uv ) * (uvScale / physTexSizeY);

	// physical texture sample
	float4 colorMetalness = texColorAndMetalness.SampleGrad( sampler1, pageUV, dx, dy );
	float4 normalRoughnessAO = texNormalRoughnessAO.SampleGrad( sampler1, pageUV, dx, dy );
	
	/*
	output.g0 = float4( colorMetalness.rgb, 1 );
	output.g1 = float4( 0, 1, 0, 1 ); // RGB=normal, A=roughness
	return output;
	*/

	// expand normal from 2 channels to 3 channels
	float3 normal;
	normal.rg = normalRoughnessAO.rg * 2 - 1;
	normal.b = 1 - (normal.r*normal.r + normal.g*normal.g);
	if ( normal.b > 0 ) normal.b = sqrt( normal.b );

	// calculate TBN matrix
	float3 tangent = normalize( cross( IN.normal, float3(0,0,1) ) );
	float3 binormal = normalize( cross( IN.normal, tangent ) );
	float3x3 TBN = float3x3( tangent, binormal, IN.normal );

	// transform normal
	normal = mul( normal, TBN );
	normal = lerp( IN.normal, normal, terrain_bumpiness );
	normal = normalize( normal );

	// WickedEngine PBR
	Surface surface;

	surface.N = normal;
	surface.P = IN.worldPos;
	surface.V = g_xCamera_CamPos - surface.P;
	float dist = length( surface.V );
	surface.V /= dist;

	// de-gamma is now done automatically by hardware due to sRGB texture
	float3 baseColor = colorMetalness.rgb;
		
	float metalness = colorMetalness.a;
	float roughness = normalRoughnessAO.b;
	float occlusion = normalRoughnessAO.a;

	surface.createMetalness( metalness, roughness, occlusion, terrain_reflectance, baseColor, true );
	
	const float2 pixel = IN.position.xy;
	const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;
	
	surface.update();

	float3 ambient = GetAmbient(surface.N);
	//ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

	/*
	float3 ambient = g_xFrame_Ambient.rgb;
	if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
	{
		float3 sampleDir;
		sampleDir.x = 0;
		sampleDir.y = saturate( 1 - IN.normal.y ) * 0.7 + 0.15;
		sampleDir.z = sqrt( 1 - sampleDir.y*sampleDir.y );
		ambient += GetDynamicSkyColor( sampleDir, false, false, false, true )*3;
		
		//ambient += lerp(
		//	GetDynamicSkyColor(-surface.V, false, false, false, true),
		//	GetDynamicSkyColor(float3(0, 1, 0), false, false, false, true),
		//	max(surface.N.y * 0.5 + 0.5, 0) );
	}
	else
	{
		float3 sampleDir = -surface.V;
		sampleDir.y = abs(sampleDir.y);
		ambient += texture_globalenvmap.SampleLevel(sampler_linear_clamp, IN.normal, 8).rgb * 2;
	}
	*/

	

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);
	
	//ForwardLighting(surface, lighting);
	float3 envAmbient = 0;
	GGTiledLightingWithAmbient(surface, lighting, envAmbient);

	lighting.indirect.diffuse += envAmbient;

	float4 color = float4(0,0,0,0);
	GGApplyLighting(surface, lighting, color);

	//ApplyFog(dist, color);
	if ( (terrain_flags & GGTERRAIN_SHADER_FLAG2_USE_FOG) ) 
	{
		color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );
	}
	
	if ( terrain_flags & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE )
	{
		float3 editColor = float3( 244.0/255.0, 239.0/255.0, 38.0/255.0 );
		float editX = (IN.worldPos.x / terrain_mapEditSize) * 0.5 + 0.5;
		float editZ = (IN.worldPos.z / terrain_mapEditSize) * 0.5 + 0.5;
		
		if ( editX >= 0 && editX <= 1 && editZ >= 0 && editZ <= 1 )
		{
			float fade = editX + editZ;
			fade = sin( fade * 150 + g_xFrame_Time*2 ) * 0.5 + 0.5;
			fade = step( fade, 0.5 );
			if ( editX > 0.004 && editX < 0.996 && editZ > 0.004 && editZ < 0.996 ) fade = 0;
			color.rgb = lerp( color.rgb, editColor, fade );
		}
	}

	if ( (terrain_flags & GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE) && !any(g_xCamera_ClipPlane) )
	{
		float3 brushColor = float3( 37.0/255.0, 245.0/255.0, 43.0/255.0 );
		float2 brushPos = IN.worldPos.xz - terrain_mouseHit.xy;
		float sqrDist = dot( brushPos, brushPos );
		float border = terrain_brushSize * 0.02;
		if ( border > 20 ) border = 20;
		if ( border < 1 ) border = 1;
		float minDist = terrain_brushSize - border;
		float maxDist = terrain_brushSize + border;

		if ( sqrDist < maxDist*maxDist && sqrDist > minDist*minDist )
		{
			color.rgb = brushColor;
		}
	}
		
	color = max( 0, color );
	output.g0 = float4( color.rgb, 1 );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness ); // RGB=normal, A=roughness
	return output;
}
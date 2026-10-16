#include "GGRootSignature.hlsli"
// brdf + lightingHF before GGCommonFunctions: ApplyFogCustom in there reaches GetDynamicSkyColor,
// which lives behind the engine lighting headers. Same include order GGGrassPS.hlsl uses.
#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"
#include "GGCommonFunctions.hlsli"

// GGMAX 3.25 - WATER BAKE, pixel shader.
//
// ★ GGMAX 3.119: TWO MODES now. DEFAULT (waterbakedepth=1) = DX11's water fog - opaque with depth and distance
// from the scene depth behind the plane, the colour lit by sun + ambient, a Schlick darkening (block below).
// waterbakedepth=0 = the 3.25/3.35f flat plane this header describes: authored colour and alpha + the distance
// ramp. Both stay off the planar reflection pass, which is where Water Bake's saving comes from.
//
// FLAT AUTHORED COLOUR AND NOTHING ELSE. No reflection, no refraction, no normal map, no FFT
// simulation, and - since Lee's 2026-08-26 feedback - no Fresnel either. The point of the switch
// is that the planar reflection pass (which redraws the whole scene from a mirrored camera,
// measured at 4.37 ms of a 10.06 ms GPU frame on TESTPRO1) stops being recorded; anything put
// back here works against that.
//
// ⚠ The ripples and refraction Lee reported in Water Bake mode were NOT this shader - they were
// the real ocean, still being drawn because the switch applied its visuals one frame before the
// flag it reads was written (see GGWaterBake.cpp). This shader never had either. The Fresnel
// lift it DID have is gone all the same: he asked for a plain colour with semi-transparency so
// the ground below the water line shows through, and that is exactly what this now is.
//
// Fog is kept, and that is a deliberate exception to "nothing else": the plane spans 250,000
// units, so without fog its far edge is a hard band of flat colour against a hazy sky. It is one
// lerp, not an effect.

struct PixelIn
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float3 worldPos : TEXCOORD0;
	float2 fres     : TEXCOORD1;  // GGMAX 3.35f: (strength, power)
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
float4 main( PixelIn IN ) : SV_TARGET
{
	const float3 viewVec = g_xCamera_CamPos - IN.worldPos;
	const float  dist    = length( viewVec );
	const float3 V       = viewVec / max( dist, 0.0001 );

	// ---- DEBUG BISECT ------------------------------------------------------------------------
	// Exact magenta arriving in the vertex colour means SET_WATERBAKEDEBUG is on, and this returns
	// it UNTOUCHED - no fog, no blend-relevant alpha, nothing.
	// The first bisect only forced the VERTEX colour and still ran ApplyFogCustom below, so at
	// several thousand units from the camera the fog could quietly absorb the whole test and the
	// result read as "the draw produced nothing" when it may have produced a fogged nothing.
	// A solid-colour bisect has to bypass the shader's own maths, or it is not solid.
	if ( IN.color.r > 0.99 && IN.color.g < 0.01 && IN.color.b > 0.99 )
		return float4( 1, 0, 1, 1 );
	// -------------------------------------------------------------------------------------------

	// ★★★ GGMAX 3.119: DX11 WATER FOG - opaque with DEPTH and DISTANCE, as DX11's "cheap water" (its FFT ocean
	// with reflections off: D:/max/WickedRepo shaders/oceanSurfacePS.hlsl:54-93, GG's water fog). DX11 mixed a
	// refraction copy of the scene behind with the lit water colour by how much water the ray crosses; with a
	// flat plane the "refraction" is just the pixel behind, so the same maths becomes this alpha - no copy, no
	// pass. The depth behind is the engine's linear-depth copy (rtLinearDepth: the prepass, so the lake bed
	// whether the terrain is live or baked, never the water) - ONE 4-byte load per water pixel.
	//   t    = view-Z gap from the surface to the bed (sky = z_far: open water is opaque), boosted by the
	//          camera height as DX11 did (t * (1 + V.y)) - with a flat bed t grows as 1/V.y: the DISTANCE term
	//   fade = exp(-4 (t - WaterFogMin) / (WaterFogMax - WaterFogMin)), capped at 1 - WaterFogMinAmount = how
	//          much of the bed shows (the level's own DX11 Water Fog values)
	//   keep = 1 - 0.5 F (Schlick, f0 0.02): DX11 lerped toward a BLACK reflection with reflections off
	// The authored Water Base Color A is a MINIMUM OPACITY here (it caps fade with WaterFogMinAmount): 0 - every
	// DX11-authored level - is exactly DX11, which ignored A. Off, or no depth copy (defensive: a render path
	// without rtLinearDepth - the plane is only drawn by the main view's transparent pass): the 3.35f plane below.
	// Under water: DX11's branch (t = distance, unlit, no fog). ⚠ With Terrain Bake on, an ENTITY under the bed
	// keeps its PrimitiveID in the resolve (the bake's prepass masks RT0), so its silhouette reads as deeper
	// water - the engine fix is depth = max(depth, customDepth) in visibility_resolveCS (not taken: a cost every
	// frame); with the live terrain the terrain's own ID wins and it is exact.
	const uint gg_opt = ggCustomFrame.ggOptions;
	[branch]
	if ( ( gg_opt & GG_OPT_WATERBAKE_DEPTH ) && GetCamera().texture_lineardepth_index >= 0 )
	{
		const float waterY = IN.worldPos.y;
		const bool  under  = g_xCamera_CamPos.y < waterY;
		const float surfZ  = IN.position.w;
		float t;
		[branch]
		if ( under ) t = surfZ;
		else
		{
			const float bedZ = texture_lineardepth[ uint2( IN.position.xy ) ] * GetCamera().z_far;
			t = max( 0.0, bedZ - surfZ );
			t += t * ( g_xCamera_CamPos.y - waterY ) / max( dist, 0.0001 );
		}
		const float fogMin = g_xFrame_WaterFogMin;
		float fade = exp( -4.0 * max( 0.0, t - fogMin ) / max( g_xFrame_WaterFogMax - fogMin, 0.1 ) );
		fade = min( saturate( 1.0 - max( g_xFrame_WaterFogMinAmount, IN.color.a ) ), fade );   // + the authored A as a floor
		float3 Cw   = IN.color.rgb;
		float  keep = 1.0;
		if ( !under )
		{
			if ( gg_opt & GG_OPT_WATERBAKE_LIT )   // DX11 oceanSurfacePS:35-43,89 - sun diffuse (1/PI) + half the ambient
				Cw *= (float3)GetSunColor() * saturate( (float)GetSunDirection().y ) * ( 1.0 / PI ) + 0.5 * (float3)GetAmbientColor();
			const float m = 1.0 - saturate( abs( V.y ) + 1e-5 );
			keep = 1.0 - 0.5 * ( 0.02 + 0.948 * ( m * m * m * m * m ) );
		}
		const float gg_a = 1.0 - fade * keep;
		float3 rgbD = Cw * ( 1.0 - fade ) * keep / max( gg_a, 1e-4 );   // straight alpha: rgb*a + bed*(1-a) = DX11
		if ( !under ) rgbD = ApplyFogCustom( IN.worldPos, dist, rgbD, V );
		if ( gg_opt & GG_OPT_WATERBAKE_VIEW_ALPHA ) return float4( gg_a.xxx, 1 );
		if ( gg_opt & GG_OPT_WATERBAKE_VIEW_THICK ) return float4( saturate( t / max( g_xFrame_WaterFogMax, 1.0 ) ).xxx, 1 );
		return float4( max( 0, rgbD ), gg_a );
	}

	// the authored water colour, unmodified
	float3 rgb = IN.color.rgb;
	rgb = ApplyFogCustom( IN.worldPos, dist, rgb, V );

	// ★★★ GGMAX 3.35f: distance opacity ramp - the water closes up as it recedes.
	//
	// The flat plane draws one alpha everywhere, so you could see the lake bed at the horizon just
	// as clearly as underfoot, which is what made it read as a sheet of coloured glass rather than
	// water. Real water goes opaque at grazing angles because it reflects instead of transmitting.
	//
	// ★ This is a genuine Schlick term on the plane normal, NOT a lerp on `dist`, and the
	// difference is not pedantry: for a horizontal plane the normal is (0,1,0), so N.V is just V.y,
	// and V.y already encodes distance AND camera height together. A raw distance ramp tuned from
	// the shoreline would be wrong the moment you climbed a cliff; this one is not.
	//
	// abs() so it still behaves when the camera is under the water looking up.
	//
	// ⚠ Cost is one saturate, one pow and one lerp on a single quad - it does not touch the
	// planar reflection pass, which is where Water Bake's 4.37 ms saving actually comes from.
	const float gg_ndotv = saturate( abs( V.y ) );
	const float gg_fres  = pow( 1.0 - gg_ndotv, max( IN.fres.y, 0.1 ) );
	const float gg_alpha = lerp( IN.color.a, 1.0, saturate( gg_fres * IN.fres.x ) );

	return float4( max( 0, rgb ), gg_alpha );
}

// GGMAX 3.72: the ENGINE's exponential fog curve, reproduced exactly.
//
// Terrain, entities and near trees are drawn by the engine's object shader, which fogs through
// fogHF.hlsli GetFogAmount(). Billboard trees, grass, baked terrain and baked water are GG custom
// draws that fog through ApplyFogCustom() below. Two formulas over one scene can never agree, and
// they did not: testpro2desertlevel2 showed distant billboard trees painted SOLID fog colour while
// the terrain they stand on was barely tinted.
//
// ★ The cause was the compat shim, not the curve. GG's C++ stores the user's Fog Range as an engine
// DENSITY (M-GridEditB_part3.cpp Wicked_ApplyFogModel: density = 4 / (far - near)), and
// GGFrameCompat.hlsli reconstructed the DX11 "fog end" from it as start + 1/density. But that 4 is
// exactly what ApplyFogCustom's own exp(dist * 4 / (fogMin - fogMax)) divides back out, so the
// reconstruction had to be start + 4/density. It was not - so every GG custom draw ran at FOUR TIMES
// the engine's optical depth. At Lee's settings (Fog Range 0-100 -> near 0, far 50000) a tree 10 km
// out sat at 96% fog against terrain at 55%.
//
// Fixing only the reconstruction would leave two curves that agree solely when fogStart is 0, so
// this mirrors the engine formula outright. ★ Keep it in step with WickedEngine/shaders/fogHF.hlsli.
//   O : ray origin (the camera)   V : NORMALISED camera -> point direction (i.e. -surface.V)
float GGEngineFogAmount( float distance, float3 O, float3 V )
{
	const float fogStart   = GetWeather().fog.start;
	const float fogDensity = GetWeather().fog.density;

	// The engine's soft entry into the fog. At fogStart 0 this divides by zero and saturates to 1,
	// which is deliberate - fog then simply starts at the camera. Mirrored, not "corrected".
	const float startDistanceFalloff = saturate( (distance - fogStart) / fogStart );

	if ( g_xFrame_Options & OPTION_BIT_HEIGHT_FOG )
	{
		const float fogHeightStart = GetWeather().fog.height_start;
		const float fogFalloffScale = rcp( max( 0.01, GetWeather().fog.height_end - fogHeightStart ) );
		const float fogFalloff = 6.907755 * fogFalloffScale;   // solve e^(-h * x) = 0.001

		const float originHeight = O.y;
		const float Z = V.y;
		const float effectiveZ = max( abs(Z), 0.001 );

		const float endLineHeight = mad( distance, Z, originHeight );
		const float minLineHeight = min( originHeight, endLineHeight );
		const float heightLineFalloff = max( minLineHeight - fogHeightStart, 0 );

		const float baseHeightFogDistance = clamp( (fogHeightStart - minLineHeight) / effectiveZ, 0, distance );
		const float exponentialFogDistance = distance - baseHeightFogDistance;
		const float exponentialHeightLineIntegral = exp( -heightLineFalloff * fogFalloff ) * (1.0 - exp( -exponentialFogDistance * effectiveZ * fogFalloff )) / (effectiveZ * fogFalloff);

		const float opticalDepth = fogDensity * startDistanceFalloff * (baseHeightFogDistance + exponentialHeightLineIntegral);
		return 1.0 - exp( -opticalDepth );
	}

	return 1.0 - exp( -fogDensity * startDistanceFalloff * distance );
}

float3 ApplyFogCustom( float3 pos, float dist, float3 color, float3 viewDir )
{
	// Captured BEFORE the colour block below, which flips viewDir in place on the non-realistic-sky
	// path. The fog amount needs the camera -> point ray, so it cannot read viewDir afterwards.
	const float3 fogRayDir = -viewDir;

	float3 fogColor = g_xFrame_WaterColor.rgb;
	float fogMinAmount = 1;
	
	// Old GG underwater fog RETIRED — Wicked's underwaterCS post-process now owns the underwater
	// look (fog + colour tint + depth extinction). This branch's gate read OPTION_BIT_WATER_ENABLED
	// (bit 22), which aliases the engine FrameCB's DEBUG_NORMAL_VIS, so pressing the 'I' debug key
	// used to accidentally flip old GG underwater fog on alongside the normal-visualisation. Forced
	// false; the else path (normal interior / realistic-sky distance fog) is the only live branch now.
	if ( false )
	{
		const float fogMin = g_xFrame_WaterFogMin;   // the dead branch keeps its own locals now
		const float fogMax = g_xFrame_WaterFogMax;
		fogMinAmount = saturate( 1 - g_xFrame_WaterFogMinAmount );

		float fogFade = viewDir.y * 0.5 + 0.5;
		fogFade = 1.0 - (fogFade * fogFade);
		fogColor *= fogFade;

		float dist2 = max( 0, g_xFrame_WaterHeight - pos.y );
		dist += dist2;
		/*
		float colorFade = max( 0, g_xFrame_WaterHeight - pos.y );
		colorFade = saturate( colorFade * 0.003 );
		colorFade = 1.0 - colorFade;
		color *= colorFade;
		*/
	}
	else
	{
		//LB: Restore fog control for interior scenes
		const float3 PassedInFogColor = GetFogColor();
		const float PassedInFogOpacity = clamp(GetFogOpacity(), 0.0, 1.0);

		if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
		{
			float3 horizonDir = -viewDir;
			float invLen = rsqrt( horizonDir.x*horizonDir.x + horizonDir.z*horizonDir.z );
			invLen *= 0.995;
			horizonDir.x *= invLen;
			horizonDir.z *= invLen;
			fogColor = GetDynamicSkyColor( float3(horizonDir.x, 0.1, horizonDir.z), false, false, false, true );
		}
		else
		{
			viewDir = -viewDir;
			viewDir.y = abs( viewDir.y );
			//PE: interior scenes , skybox looks strange, so disable for now.
			//fogColor = texture_globalenvmap.SampleLevel(sampler_linear_clamp, viewDir, 8).rgb;
			fogColor = color.rgb;
			//PE: Remove env map by opacity.
			//fogColor = lerp(color.rgb, fogColor, PassedInFogOpacity);
		}

		fogColor = lerp(fogColor, PassedInFogColor, PassedInFogOpacity);
	}

	// ★ The distance term is now the ENGINE's, so a GG custom draw hazes at exactly the rate the
	// terrain behind it does. No early-out is needed: below fogStart the falloff saturates to 0 and
	// the amount is 0, which is what the old "if ( dist <= fogMin ) return color;" did by hand.
	float transmittance = 1.0 - GGEngineFogAmount( dist, g_xCamera_CamPos, fogRayDir );
	transmittance = min( fogMinAmount, transmittance );
	return lerp( fogColor, color, transmittance );
}
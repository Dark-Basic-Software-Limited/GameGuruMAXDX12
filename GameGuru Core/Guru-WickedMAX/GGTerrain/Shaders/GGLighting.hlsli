// GGLighting.hlsli - Shared tiled lighting and light application functions
// for all GameGuru custom shaders (terrain, grass, trees, spheres).
//
// This header factors out the duplicated TiledLighting() function that was
// copy-pasted across ~11 pixel shaders with minor variations. The variations
// are controlled by preprocessor defines set BEFORE including this header:
//
//   GGLIGHTING_ENVMAP_AMBIENT     - Output envmapAmbient from env probe loop (terrain PBR)
//   GGLIGHTING_SIMPLE_SHADOWS     - Accept runtime 'simple' flag for DirectionalLight (grass)
//   GGLIGHTING_ALWAYS_SIMPLE      - Always pass simple=true to DirectionalLight (low-LOD trees)
//   GGLIGHTING_VOXELGI            - Include VoxelGI call between envmaps and lights (trees high)
//   GGLIGHTING_SKIP_ENVMAPS       - Skip the entire env map accumulation section (low-LOD trees)
//
// Prerequisites:
//   - PBR/brdf.hlsli and PBR/lightingHF.hlsli must be included before this header
//   - Engine globals.hlsli must be included (provides load_entity/load_entitymatrix/
//     load_entitytile macros and bindless_cubemaps_half4 for env probes)
//   - The g_xFrame_* compatibility macros from GGFrameCompat.hlsli must be available
//
// Public functions:
//
//   GGTiledLighting(surface, lighting)
//     Basic variant for most shaders. Env maps + lights.
//
//   GGTiledLightingWithAmbient(surface, lighting, envmapAmbient)
//     Terrain PBR variant. Requires GGLIGHTING_ENVMAP_AMBIENT.
//     Outputs per-probe ambient for indirect diffuse.
//
//   GGTiledLightingSimple(surface, lighting, simple)
//     Grass variant. Requires GGLIGHTING_SIMPLE_SHADOWS.
//     Passes runtime 'simple' flag to DirectionalLight.
//
//   GGForwardLighting(surface, lighting)
//     Env probe forward pass variant. Uses xForwardLightMask instead of EntityTiles.
//     Always calls DirectionalLight with simple=true.
//     Requires ForwardEntityMaskCB (xForwardLightMask) to be declared.
//
//   GGApplyLighting(surface, lighting, color)
//     Standard light application: albedo * diffuse + specular.
//
//   GGApplyLightingRefraction(surface, lighting, color)
//     Light application with refraction blend (terrain sphere).

#ifndef GG_LIGHTING_HLSLI
#define GG_LIGHTING_HLSLI

// ============================================================================
// Internal: Environment map accumulation
// ============================================================================
#ifndef GGLIGHTING_SKIP_ENVMAPS
inline float4 GG_AccumulateEnvMaps(inout Surface surface, uint flatTileIndex
#ifdef GGLIGHTING_ENVMAP_AMBIENT
	, out float3 envmapAmbient
#endif
)
{
	float4 envmapAccumulation = 0;
#ifdef GGLIGHTING_ENVMAP_AMBIENT
	envmapAmbient = 0;
#endif

#ifndef DISABLE_ENVMAPS
#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (g_xFrame_EnvProbeArrayCount > 0)
	{
		const uint first_item = g_xFrame_EnvProbeArrayOffset;
		const uint last_item = first_item + g_xFrame_EnvProbeArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item && envmapAccumulation.a < 1)
				{
					ShaderEntity probe = load_entity(entity_index);

					float4x4 probeProjection = load_entitymatrix(probe.GetMatrixIndex());
					// Extract cubemap texture descriptor stashed in matrix row 3
					const int probeTexture = asint(probeProjection[3][0]);
					probeProjection[3] = float4(0, 0, 0, 1); // restore matrix to proper form
					TextureCube<half4> cubemap = bindless_cubemaps_half4[descriptor_index(probeTexture)];

					const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
					const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
					[branch]
					if (is_saturated(uvw))
					{
#ifdef GGLIGHTING_ENVMAP_AMBIENT
						uint2 probeDim;
						uint probeMipCount;
						cubemap.GetDimensions(0, probeDim.x, probeDim.y, probeMipCount);
						float3 probeAmbient = cubemap.SampleLevel(sampler_linear_clamp, surface.N, float(probeMipCount) - 1).rgb;
#endif
						const float4 envmapColor = EnvironmentReflection_Local(cubemap, surface, probe, probeProjection, clipSpacePos);
						// Manual blending of probes (sorted top-to-bottom, blended bottom-to-top)
						envmapAccumulation.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * envmapColor.rgb) + envmapAccumulation.rgb;
#ifdef GGLIGHTING_ENVMAP_AMBIENT
						envmapAmbient.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * probeAmbient.rgb) + envmapAmbient.rgb;
#endif
						envmapAccumulation.a = envmapColor.a + (1 - envmapColor.a) * envmapAccumulation.a;
						[branch]
						if (envmapAccumulation.a >= 1.0)
						{
							bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
							break;
						}
					}
				}
				else if (entity_index > last_item)
				{
					bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
					break;
				}
			}
		}
	}
#endif // DISABLE_LOCALENVPMAPS

	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99)
	{
#ifdef GGLIGHTING_ENVMAP_AMBIENT
		TextureCube<half4> globalCubemap = bindless_cubemaps_half4[descriptor_index(GetScene().globalprobe)];
		uint2 globalDim;
		uint globalMipCount;
		globalCubemap.GetDimensions(0, globalDim.x, globalDim.y, globalMipCount);
		float3 globalAmbient = globalCubemap.SampleLevel(sampler_linear_clamp, surface.N, float(globalMipCount) - 1).rgb;
		envmapAmbient = lerp( globalAmbient, envmapAmbient, envmapAccumulation.a );
#endif
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface), envmapAccumulation.rgb, envmapAccumulation.a);
	}
#endif // DISABLE_ENVMAPS

	return envmapAccumulation;
}
#endif // !GGLIGHTING_SKIP_ENVMAPS

// ============================================================================
// Internal: Light iteration
// ============================================================================
inline void GG_IterateLights(inout Surface surface, inout Lighting lighting, uint flatTileIndex
#ifdef GGLIGHTING_SIMPLE_SHADOWS
	, bool simple
#endif
)
{
	[branch]
	if (g_xFrame_LightArrayCount > 0)
	{
		const uint first_item = g_xFrame_LightArrayOffset;
		const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
					ShaderEntity light = load_entity(entity_index);

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue;
					}

					switch (light.GetType())
					{
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
#if defined(GGLIGHTING_SIMPLE_SHADOWS)
						DirectionalLight(light, surface, lighting, 1, simple);
#elif defined(GGLIGHTING_ALWAYS_SIMPLE)
						DirectionalLight(light, surface, lighting, 1, true);
#else
						DirectionalLight(light, surface, lighting);
#endif
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
						PointLight(light, surface, lighting);
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						SpotLight(light, surface, lighting);
					}
					break;
					}
				}
				else if (entity_index > last_item)
				{
					bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
					break;
				}
			}
		}
	}
}

// ============================================================================
// Public: GGTiledLighting - standard variant
//   Used by: GGTreesHighPS, GGTreeBranchesHighPS, GGTerrainSpherePS,
//            GGTreesPS (with GGLIGHTING_SKIP_ENVMAPS + GGLIGHTING_ALWAYS_SIMPLE)
// ============================================================================
inline void GGTiledLighting(inout Surface surface, inout Lighting lighting)
{
	const uint2 tileIndex = uint2(floor(surface.pixel / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

#ifndef GGLIGHTING_SKIP_ENVMAPS
#ifdef GGLIGHTING_ENVMAP_AMBIENT
	float3 _unused_ambient;
	float4 envmapAccumulation = GG_AccumulateEnvMaps(surface, flatTileIndex, _unused_ambient);
#else
	float4 envmapAccumulation = GG_AccumulateEnvMaps(surface, flatTileIndex);
#endif

#ifndef DISABLE_ENVMAPS
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif
#endif // !GGLIGHTING_SKIP_ENVMAPS

#ifndef DISABLE_VOXELGI
#ifdef GGLIGHTING_VOXELGI
	VoxelGI(surface, lighting);
#endif
#endif

	GG_IterateLights(surface, lighting, flatTileIndex
#ifdef GGLIGHTING_SIMPLE_SHADOWS
		, false
#endif
	);
}

#ifdef GGLIGHTING_ENVMAP_AMBIENT
// ============================================================================
// Public: GGTiledLightingWithAmbient - terrain PBR variant
//   Used by: GGTerrainVirtualPBR_PS
//   Outputs envmapAmbient for indirect diffuse contribution
// ============================================================================
inline void GGTiledLightingWithAmbient(inout Surface surface, inout Lighting lighting, out float3 envmapAmbient)
{
	const uint2 tileIndex = uint2(floor(surface.pixel / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	float4 envmapAccumulation = GG_AccumulateEnvMaps(surface, flatTileIndex, envmapAmbient);

#ifndef DISABLE_ENVMAPS
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif

	GG_IterateLights(surface, lighting, flatTileIndex
#ifdef GGLIGHTING_SIMPLE_SHADOWS
		, false
#endif
	);
}
#endif // GGLIGHTING_ENVMAP_AMBIENT

#ifdef GGLIGHTING_SIMPLE_SHADOWS
// ============================================================================
// Public: GGTiledLightingSimple - grass variant
//   Used by: GGGrassPS
//   Accepts runtime 'simple' flag for DirectionalLight shadow simplification
// ============================================================================
inline void GGTiledLightingSimple(inout Surface surface, inout Lighting lighting, bool simple)
{
	const uint2 tileIndex = uint2(floor(surface.pixel / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

#ifndef GGLIGHTING_SKIP_ENVMAPS
#ifdef GGLIGHTING_ENVMAP_AMBIENT
	float3 _unused_ambient2;
	float4 envmapAccumulation = GG_AccumulateEnvMaps(surface, flatTileIndex, _unused_ambient2);
#else
	float4 envmapAccumulation = GG_AccumulateEnvMaps(surface, flatTileIndex);
#endif

#ifndef DISABLE_ENVMAPS
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif
#endif // !GGLIGHTING_SKIP_ENVMAPS

	GG_IterateLights(surface, lighting, flatTileIndex, simple);
}
#endif // GGLIGHTING_SIMPLE_SHADOWS

// ============================================================================
// Public: GGForwardLighting - env probe forward pass variant
//   Used by: GGTerrainEnvProbePS, GGTreesHighEnvProbePS, GGTreeBranchesHighEnvProbePS
//   Uses xForwardLightMask from ForwardEntityMaskCB instead of EntityTiles.
//   Always calls DirectionalLight with simple=true.
//   Requires ForwardEntityMaskCB (xForwardLightMask) to be declared.
// ============================================================================
inline void GGForwardLighting(inout Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	float4 envmapAccumulation = 0;

	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface), envmapAccumulation.rgb, envmapAccumulation.a);
	}
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

	[branch]
	if (any(xForwardLightMask))
	{
		// Loop through light buckets for the draw call:
		const uint first_item = 0;
		const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, 1); // only 2 buckets max (uint2) for forward pass!
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = xForwardLightMask[bucket];

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				ShaderEntity light = load_entity(g_xFrame_LightArrayOffset + entity_index);
				if ((light.layerMask & surface.layerMask) == 0)
					continue;

				if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
				{
					continue;
				}

				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					DirectionalLight(light, surface, lighting, 1, true);
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					PointLight(light, surface, lighting);
				}
				break;
				}
			}
		}
	}
}

// ============================================================================
// Public: GGApplyLighting - standard light combination
//   Used by: most shaders
// ============================================================================
inline void GGApplyLighting(in Surface surface, in Lighting lighting, inout float4 color)
{
	LightingPart combined_lighting = CombineLighting(surface, lighting);
	color.rgb = surface.albedo * combined_lighting.diffuse + combined_lighting.specular;
}

// ============================================================================
// Public: GGApplyLightingRefraction - with refraction blend
//   Used by: GGTerrainSpherePS
// ============================================================================
inline void GGApplyLightingRefraction(in Surface surface, in Lighting lighting, inout float4 color)
{
	LightingPart combined_lighting = CombineLighting(surface, lighting);
	color.rgb = lerp(surface.albedo * combined_lighting.diffuse, surface.refraction.rgb, surface.refraction.a) + combined_lighting.specular;
}

#endif // GG_LIGHTING_HLSLI

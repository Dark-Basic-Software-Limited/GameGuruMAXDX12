#ifndef WI_LIGHTING_HF
#define WI_LIGHTING_HF
#include "globals.hlsli"
#include "brdf.hlsli"
#include "voxelConeTracingHF.hlsli"
#include "skyHF.hlsli"
#include "shadowHF.hlsli"

struct LightingPart
{
	float3 diffuse;
	float3 specular;
};
struct Lighting
{
	LightingPart direct;
	LightingPart indirect;

	inline void create(
		in float3 diffuse_direct,
		in float3 specular_direct,
		in float3 diffuse_indirect,
		in float3 specular_indirect
	)
	{
		direct.diffuse = diffuse_direct;
		direct.specular = specular_direct;
		indirect.diffuse = diffuse_indirect;
		indirect.specular = specular_indirect;
	}
};

// Combine the direct and indirect lighting into final contribution
inline LightingPart CombineLighting(in Surface surface, in Lighting lighting)
{
	LightingPart result;
	result.diffuse = lighting.direct.diffuse + lighting.indirect.diffuse * surface.occlusion;
	result.specular = lighting.direct.specular + lighting.indirect.specular * surface.occlusion;

	return result;
}

// Shadow sampling now provided by engine's shadowHF.hlsli (shadow_2D, shadow_cube)

inline void DirectionalLight(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1, in bool simple = false)
{
	float3 L = light.GetDirection();

	SurfaceToLight surfaceToLight;
	surfaceToLight.create(surface, L);

#ifdef SUBSURFACESCATTERING
	//PE: Crysis subsurface scattering
	//https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-16-vegetation-procedural-animation-and-shading-crysis
    const float3 lightcolor = light.GetColor().rgb;
	// New engine bakes energy into GetColor() at upload time, so GetEnergy() is removed.
	const float lightenergy = 1.0;
	const float dotsNL = max(0, dot(-surface.N, L));
	const float dotEL = max(0, dot(L, -surface.V));
	const float DotELNL = pow(dotEL, 8) * dotsNL;
#endif

	[branch]
	if (any(surfaceToLight.NdotL_sss))
	{
		float3 shadow = shadow_mask;

		[branch]
        if (light.IsCastingShadow() && surface.IsReceiveShadow())
		{
#ifndef RTAPI
			[branch]
			if ((g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS) == 0)
#endif // RTAPI
			{
				if ( L.y <= 0 )
				{
					shadow = float3(0,0,0);
				}
				else
				{
					// Loop through cascades from closest (smallest) to furthest (largest)
					[loop]
					for (uint cascade = (simple ? 2 : 0); cascade < light.GetShadowCascadeCount(); ++cascade)
					{
						// Project into shadow map space (no need to divide by .w because ortho projection!):
						float3 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(surface.P, 1)).xyz;
						float3 shadow_uv = clipspace_to_uv(shadow_pos);

						// Determine if pixel is inside current cascade bounds and compute shadow if it is:
						[branch]
						if (is_saturated(shadow_uv))
						{
							const half3 shadow_main = shadow_2D(light, shadow_pos.z, shadow_uv.xy, cascade);
							const half3 shadow_box = half3(shadow_pos.xy, shadow_pos.z * 2 - 1);
							const half3 cascade_edgefactor = saturate(saturate(abs(shadow_box)) - 0.8) * 5.0;
							const half cascade_fade = max(cascade_edgefactor.x, max(cascade_edgefactor.y, cascade_edgefactor.z));

							// If we are on cascade edge threshold and not the last cascade, then fallback to a larger cascade:
							[branch]
							if (cascade_fade > 0 && cascade < light.GetShadowCascadeCount() - 1)
							{
								// Project into next shadow cascade (no need to divide by .w because ortho projection!):
								cascade += 1;
								shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(surface.P, 1)).xyz;
								shadow_uv = clipspace_to_uv(shadow_pos);
								const half3 shadow_fallback = shadow_2D(light, shadow_pos.z, shadow_uv.xy, cascade);

								shadow *= lerp(shadow_main, shadow_fallback, cascade_fade);
							}
							else
							{
								shadow *= shadow_main;
							}
							break;
						}
					}
				}
			}
		}

		[branch]
		if ( any(shadow))
		{
			float3 atmosphereTransmittance = 1;
			if ( !simple && (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY) )
			{
				atmosphereTransmittance = GetAtmosphericLightTransmittance(g_xFrame_Atmosphere, surface.P, L, texture_transmittancelut);
			}

            //shadow = lerp(shadow, 1.0, 0.5);
#ifdef SUBSURFACESCATTERING
			float3 lightColor = lightcolor * lightenergy * shadow * atmosphereTransmittance;
#else
			float3 lightColor = light.GetColor().rgb * shadow * atmosphereTransmittance;
#endif

            lighting.direct.diffuse += max(0, lightColor * surfaceToLight.NdotL_sss * BRDF_GetDiffuse(surface, surfaceToLight));

			if ( !simple )
			{
				lighting.direct.specular += max(0, lightColor * surfaceToLight.NdotL * BRDF_GetSpecular(surface, surfaceToLight));
			}

#ifndef WATER
            // Caustic UV scale is driven by the water component's "Caustic Size" slider
            // (GGCustomFrameCB.causticScale = base/size, computed C++-side). Bigger size ->
            // smaller scale -> larger, more natural caustic cells for GGMAX's inch world.
            float2 ocean_uv = (surface.P.xz + (surface.P.yy * 0.25)) * g_xFrame_CausticScale;
            float water_height = g_xFrame_WaterHeight;
            if ((g_xFrame_Options & OPTION_BIT_WATER_ENABLED) && surface.P.y < water_height)
            {
                float3 caustic = caustic_pattern(ocean_uv, g_xFrame_Time * 0.65);
                caustic *= sqr(saturate((water_height - surface.P.y) * 0.0025)); // fade out at shoreline
                caustic *= lightColor;
                lighting.indirect.diffuse += caustic;
            }
#endif

		}

    }

#ifdef SUBSURFACESCATTERING
	//PE: Crysis subsurface scattering
#ifdef GRASSPS
	float3 lightColor = lightcolor * lightenergy; //* shadow * atmosphereTransmittance
	lighting.direct.diffuse += max(0, lightColor * (dotEL*0.2) * g_xFrame_TreeSubSurfaceScattering);
#else
	float3 lightColor = lightcolor * lightenergy; //* shadow * atmosphereTransmittance
	lighting.direct.diffuse += max(0, lightColor * DotELNL * g_xFrame_TreeSubSurfaceScattering);
#endif
#endif

}

inline void PointLight(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range2 = light.GetRange() * light.GetRange();

	[branch]
	if (dist2 < range2)
	{
		const float3 Lunnormalized = L;
		const float dist = sqrt(dist2);
		L /= dist;

		SurfaceToLight surfaceToLight;
		surfaceToLight.create(surface, L);

		[branch]
		if (any(surfaceToLight.NdotL_sss))
		{
			float3 shadow = shadow_mask;

			[branch]
			if (light.IsCastingShadow() && surface.IsReceiveShadow())
			{
#ifndef RTAPI
				[branch]
				if ((g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS) == 0)
#endif // RTAPI
				{
					shadow *= shadow_cube(light, Lunnormalized);
				}
			}

			[branch]
			if (any(shadow))
			{
				float3 lightColor = light.GetColor().rgb * shadow;

				const float att = saturate(1 - (dist2 / range2));
				const float attenuation = att * att;
				lightColor *= attenuation;

				lighting.direct.diffuse +=
					max(0, lightColor * surfaceToLight.NdotL_sss * BRDF_GetDiffuse(surface, surfaceToLight));

				lighting.direct.specular +=
					max(0, lightColor * surfaceToLight.NdotL * BRDF_GetSpecular(surface, surfaceToLight));
			}
		}
	}
}
inline void SpotLight(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range2 = light.GetRange() * light.GetRange();

	[branch]
	if (dist2 < range2)
	{
		const float dist = sqrt(dist2);
		L /= dist;

		SurfaceToLight surfaceToLight;
		surfaceToLight.create(surface, L);

		[branch]
		if (any(surfaceToLight.NdotL_sss))
		{
			const float SpotFactor = dot(L, light.GetDirection());
			const float spotCutOff = light.GetConeAngleCos();

			[branch]
			if (SpotFactor > spotCutOff)
			{
				float3 shadow = shadow_mask;

				[branch]
				if (light.IsCastingShadow() && surface.IsReceiveShadow())
				{
#ifndef RTAPI
					[branch]
					if ((g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS) == 0)
#endif // RTAPI
					{
						float4 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(surface.P, 1));
						shadow_pos.xyz /= shadow_pos.w;
						float2 shadow_uv = clipspace_to_uv(shadow_pos.xy);
						[branch]
						if (is_saturated(shadow_uv))
						{
							shadow *= shadow_2D(light, shadow_pos.z, shadow_uv.xy, 0);
						}
					}
				}

				[branch]
				if (any(shadow))
				{
					float3 lightColor = light.GetColor().rgb * shadow;

					const float att = saturate(1 - (dist2 / range2));
					float attenuation = att * att;
					attenuation *= saturate((1 - (1 - SpotFactor) * 1 / (1 - spotCutOff)));
					lightColor *= attenuation;

					lighting.direct.diffuse +=
						max(0, lightColor * surfaceToLight.NdotL_sss * BRDF_GetDiffuse(surface, surfaceToLight));

					lighting.direct.specular +=
						max(0, lightColor * surfaceToLight.NdotL * BRDF_GetSpecular(surface, surfaceToLight));
				}
			}
		}
	}
}


// ENVIRONMENT MAPS


inline float3 GetAmbient(in float3 N)
{
	float3 ambient = 0;

#ifdef ENVMAPRENDERING

	// Set realistic_sky_stationary to true so we capture ambient at float3(0.0, 0.0, 0.0), similar to the standard sky to avoid flickering and weird behavior
	//ambient = lerp(
	//	GetDynamicSkyColor(float3(0, -1, 0), false, false, false, true),
	//	GetDynamicSkyColor(float3(0, 1, 0), false, false, false, true),
	//	saturate(N.y * 0.5 + 0.5));

#else

	//ambient = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(N, g_xFrame_GlobalEnvProbeIndex), g_xFrame_EnvProbeMipCount-2).rgb;

#endif // ENVMAPRENDERING

	// This is not entirely correct if we have probes, because it shouldn't be added twice.
	//	However, it is not correct if we leave it out from probes, because if we render a scene
	//	with dark sky but ambient, we still want some visible result.
	ambient += GetAmbientColor();

	return ambient;
}

// surface:				surface descriptor
// MIP:					mip level to sample
// return:				color of the environment color (rgb)
inline float3 EnvironmentReflection_Global(in Surface surface)
{
	float3 envColor;

#ifdef ENVMAPRENDERING
	return float3(0,0,0);
#else

	// New engine: global env probe is a bindless cubemap
	TextureCube<half4> cubemap = bindless_cubemaps_half4[descriptor_index(GetScene().globalprobe)];
	uint2 cubeDim;
	uint cubeMipCount;
	cubemap.GetDimensions(0, cubeDim.x, cubeDim.y, cubeMipCount);

	float MIP = surface.roughness * float(cubeMipCount);
	envColor = cubemap.SampleLevel(sampler_linear_clamp, surface.R, MIP).rgb * surface.F;

#ifdef BRDF_SHEEN
	envColor *= surface.sheen.albedoScaling;
	MIP = surface.sheen.roughness * float(cubeMipCount);
	envColor += cubemap.SampleLevel(sampler_linear_clamp, surface.R, MIP).rgb * surface.sheen.color * surface.sheen.DFG;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	envColor *= 1 - surface.clearcoat.F;
	MIP = surface.clearcoat.roughness * float(cubeMipCount);
	envColor += cubemap.SampleLevel(sampler_linear_clamp, surface.clearcoat.R, MIP).rgb * surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

#endif // ENVMAPRENDERING

	envColor *= (0.5 * surface.metalness + 0.5);
	return envColor;
}

// surface:				surface descriptor
// probe :				the shader entity holding properties
// probeProjection:		the inverse OBB transform matrix
// clipSpacePos:		world space pixel position transformed into OBB space by probeProjection matrix
// MIP:					mip level to sample
// return:				color of the environment map (rgb), blend factor of the environment map (a)
inline float4 EnvironmentReflection_Local(in TextureCube<half4> cubemap, in Surface surface, in ShaderEntity probe, in float4x4 probeProjection, in float3 clipSpacePos)
{
	if ((probe.layerMask & surface.layerMask) == 0)
		return 0; // early exit: layer mismatch

	// Perform parallax correction of reflection ray (R) into OBB:
	float3 RayLS = mul((float3x3)probeProjection, surface.R);
	float3 FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	float3 R_parallaxCorrected = surface.P - probe.position + surface.R * Distance;

	// Get mip count from cubemap dimensions
	uint2 cubeDim;
	uint cubeMipCount;
	cubemap.GetDimensions(0, cubeDim.x, cubeDim.y, cubeMipCount);

	// Sample cubemap texture:
	float MIP = surface.roughness * float(cubeMipCount);
	float3 envColor = cubemap.SampleLevel(sampler_linear_clamp, R_parallaxCorrected, MIP).rgb * surface.F;

#ifdef BRDF_SHEEN
	envColor *= surface.sheen.albedoScaling;
	MIP = surface.sheen.roughness * float(cubeMipCount);
	envColor += cubemap.SampleLevel(sampler_linear_clamp, R_parallaxCorrected, MIP).rgb * surface.sheen.color * surface.sheen.DFG;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	RayLS = mul((float3x3)probeProjection, surface.clearcoat.R);
	FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	R_parallaxCorrected = surface.P - probe.position + surface.clearcoat.R * Distance;

	envColor *= 1 - surface.clearcoat.F;
	MIP = surface.clearcoat.roughness * float(cubeMipCount);
	envColor += cubemap.SampleLevel(sampler_linear_clamp, R_parallaxCorrected, MIP).rgb * surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

	// blend out if close to any cube edge:
	float edgeBlend = 1 - pow(saturate(max(abs(clipSpacePos.x), max(abs(clipSpacePos.y), abs(clipSpacePos.z)))), 8);

	envColor *= (0.5 * surface.metalness + 0.5);
	return float4(envColor, edgeBlend);
}



// VOXEL RADIANCE

inline void VoxelGI(in Surface surface, inout Lighting lighting)
{
	[branch] if (g_xFrame_VoxelRadianceDataRes != 0)
	{
		// determine blending factor (we will blend out voxel GI on grid edges):
		float3 voxelSpacePos = surface.P - g_xFrame_VoxelRadianceDataCenter;
		voxelSpacePos *= g_xFrame_VoxelRadianceDataSize_rcp;
		voxelSpacePos *= g_xFrame_VoxelRadianceDataRes_rcp;
		voxelSpacePos = saturate(abs(voxelSpacePos));
		float blend = 1 - pow(max(voxelSpacePos.x, max(voxelSpacePos.y, voxelSpacePos.z)), 4);

		// diffuse:
		{
			float4 trace = ConeTraceDiffuse(texture_voxelradiance, surface.P, surface.N);
			lighting.indirect.diffuse = lerp(lighting.indirect.diffuse, trace.rgb, trace.a * blend);
		}

		// specular:
		[branch]
		if (g_xFrame_Options & OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED)
		{
			float4 trace = ConeTraceSpecular(texture_voxelradiance, surface.P, surface.N, surface.V, surface.roughness);
			lighting.indirect.specular = lerp(lighting.indirect.specular, trace.rgb * surface.F, trace.a * blend);
		}
	}
}

#endif // WI_LIGHTING_HF

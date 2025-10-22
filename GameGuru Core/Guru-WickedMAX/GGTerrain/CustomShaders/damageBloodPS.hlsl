// damageBloodPS.hlsl - Procedural Damage Blood Shader


#define OBJECTSHADER_LAYOUT_COMMON
#define BLOODDAMAGE
#define OBJECTSHADER_USE_ROTATIONAXIS
#define OBJECTSHADER_USE_POSITION3D
#define OBJECTSHADER_USE_COLOR

#include "objectHF.hlsli"

#include "globals.hlsli"
#include "brdf.hlsli"
#include "lightingHF.hlsli"


float hash21(float2 p)
{
    p = frac(p * float2(233.34, 851.73));
    p += dot(p, p + 23.45);
    return frac(p.x * p.y);
}

float noiseMav(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));
    
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

// Fractal noise for complex splatter patterns
float fbm(float2 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noiseMav(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}


// Generates a static, irregular blood splatter mask using a 2D position input.
float getStaticBloodMask(float2 pos, float coverageAmount, float octaves)
{
    // Use a large, non-zero offset to ensure the pattern is static and consistent across objects
    float splatterNoise = fbm(pos + 100.0, octaves);
    
    // Calculate the threshold based on the desired coverage amount (0.0 to 1.0)
    float bloodMask = saturate(splatterNoise - (1.0 - coverageAmount));
    
    // Soften the edges of the blood mask
    bloodMask = pow(bloodMask, 0.5);
    
    return bloodMask;
}

// Defines the blood color gradient
float3 getBloodColor(float coverageValue, float brightness)
{
    float3 darkRed = float3(0.3 * brightness, 0.0, 0.0); // Dry/old blood
    float3 brightRed = float3(0.6 * brightness, 0.05 * brightness, 0.05 * brightness); // Fresh blood
    
    return lerp(darkRed, brightRed, pow(coverageValue, 2.0));
}


// 3D hash — returns a single float based on a 3D position
float hash31(float3 p)
{
    p = frac(p * float3(443.8975, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return frac((p.x + p.y) * p.z);
}

// Smoothed 3D value noise
float noiseMav3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    // 8 corner hashes
    float n000 = hash31(i + float3(0.0, 0.0, 0.0));
    float n100 = hash31(i + float3(1.0, 0.0, 0.0));
    float n010 = hash31(i + float3(0.0, 1.0, 0.0));
    float n110 = hash31(i + float3(1.0, 1.0, 0.0));
    float n001 = hash31(i + float3(0.0, 0.0, 1.0));
    float n101 = hash31(i + float3(1.0, 0.0, 1.0));
    float n011 = hash31(i + float3(0.0, 1.0, 1.0));
    float n111 = hash31(i + float3(1.0, 1.0, 1.0));

    // Trilinear interpolation
    float nx00 = lerp(n000, n100, f.x);
    float nx10 = lerp(n010, n110, f.x);
    float nx01 = lerp(n001, n101, f.x);
    float nx11 = lerp(n011, n111, f.x);

    float nxy0 = lerp(nx00, nx10, f.y);
    float nxy1 = lerp(nx01, nx11, f.y);

    return lerp(nxy0, nxy1, f.z);
}

// Fractal Brownian Motion using 3D noise
float fbm3D(float3 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noiseMav3D(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

// Static blood mask using 3D world position
float getStaticBloodMask3D(float3 pos, float coverageAmount)
{
    // Offset to keep pattern consistent
    float splatterNoise = fbm3D(pos + 100.0, 3);
    
    // Threshold based on coverage amount
    float bloodMask = saturate(splatterNoise - (1.0 - coverageAmount));
    
    // Soften edges
    bloodMask = pow(bloodMask, 0.5);
    
    return bloodMask;
}



// --- Tiled Lighting Function (UNTOUCHED for brevity) ---
inline void TiledLighting(inout Surface surface, inout Lighting lighting)
{
    const uint2 tileIndex = uint2(floor(surface.pixel / TILED_CULLING_BLOCKSIZE));
    const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;
    
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
            uint bucket_bits = EntityTiles[flatTileIndex + bucket];
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
                    ShaderEntity light = EntityArray[entity_index];
                    if ((light.layerMask & surface.layerMask) == 0)
                        continue;
                    if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
                        continue;

                    switch (light.GetType())
                    {
                        case ENTITY_TYPE_DIRECTIONALLIGHT:
                            DirectionalLight(light, surface, lighting);
                            break;
                        case ENTITY_TYPE_POINTLIGHT:
                            PointLight(light, surface, lighting);
                            break;
                        case ENTITY_TYPE_SPOTLIGHT:
                            SpotLight(light, surface, lighting);
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


// --- Main Pixel Shader Entry Point ---
float4 main(PixelInput input, in bool is_frontface : SV_IsFrontFace) : SV_TARGET
{
    const float depth = input.pos.z;
    const float lineardepth = input.pos.w;
    const float2 pixel = input.pos.xy;
    const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;

    // Initialize surface
    Surface surface;
    surface.init();

    // Handle front/back faces
    if (is_frontface == false)
    {
        input.nor = -input.nor;
    }
    surface.N = normalize(input.nor);
    surface.P = input.pos3D; // World Position
    surface.V = g_xCamera_CamPos - surface.P;
    float dist = length(surface.V);
    surface.V /= dist;

    float4 tangent = input.tan;
    tangent.xyz = normalize(tangent.xyz);
    float3 binormal = normalize(cross(tangent.xyz, surface.N) * tangent.w);
    float3x3 TBN = float3x3(tangent.xyz, binormal, surface.N);

    float bloodCoverage = GetMaterial().customShaderParam1;
    float splatterScale = GetMaterial().customShaderParam2;
    float wetness = GetMaterial().customShaderParam3;
    float edgeFade = GetMaterial().customShaderParam4;
    float maxBlood = GetMaterial().customShaderParam5;
    float brightness = GetMaterial().customShaderParam6;

    float2 positionNoiseInput = input.uvsets.zw;
    float2 noiseInput = positionNoiseInput / splatterScale;
    
    float2 uv = input.uvsets.xy;
    float3 worldPos = input.pos3D;
    float3 worldNormal = normalize(input.nor);

    float splatterScale2 = splatterScale * 50;
    splatterScale *= 50;
    float2 uvNoise = uv * splatterScale2;
    float2 uvNoise2 = uv * splatterScale2 * 2.0;
    float uvBase = fbm(uvNoise + 100.0, 4);
    float uvBase2 = fbm(uvNoise2 + 100.0, 2);
    float mixedNoise = lerp(uvBase, uvBase2, 0.5);
    
    
    bloodCoverage *= (maxBlood * 0.05);
    bloodCoverage = pow(bloodCoverage,0.15);
    float bloodMask = saturate(mixedNoise - (1.0 - bloodCoverage));

    bloodMask = pow(bloodMask, edgeFade);
    
    float3 bumpColor = 0;
    if (bloodMask < 0.001)
    {
        // Re-use original UVs for texture sampling
        float4 color = input.color;
        float4 baseColorMap = texture_basecolormap.Sample(sampler_objectshader, input.uvsets.xy);
        color.rgb = DEGAMMA(baseColorMap.rgb);

        NormalMapping(input.uvsets, surface.N, TBN, bumpColor);

        float4 surfaceMap = texture_surfacemap.Sample(sampler_objectshader, input.uvsets.xy);
        surface.create(GetMaterial(), color, surfaceMap);
        surface.update();

        float3 ambient = GetAmbient(surface.N);
        Lighting lighting;
        lighting.create(0, 0, ambient, 0);
        //TiledLighting(surface, lighting);
        //PE: envMap was missing.
        float3 envAmbient = 0;
        TiledLighting(surface, lighting, envAmbient);
        lighting.indirect.diffuse += envAmbient;

        
        ApplyLighting(surface, lighting, color);
        ApplyFog(dist, g_xCamera_CamPos, surface.V, color);
        
        return max(0, color);
    }

    float4 baseColorMap = texture_basecolormap.Sample(sampler_objectshader, input.uvsets.xy);
    float3 originalColor = DEGAMMA(baseColorMap.rgb);
    float3 bloodColor = getBloodColor(bloodMask,brightness);
    float3 finalColor_rgb = lerp(originalColor, bloodColor, bloodMask);
    float4 color = float4(finalColor_rgb, baseColorMap.a);

    NormalMapping(input.uvsets, surface.N, TBN, bumpColor);
    
    //float3 originalNormal = texture_normalmap.Sample(sampler_objectshader, input.uvsets.xy).rgb * 2.0 - 1.0;
    //float noiseBump = fbm(noiseInput * 4.0, 2) * 0.5 - 0.25;
    //float3 bloodBump = float3(noiseBump, noiseBump, 1.0);
    //float3 finalNormal_TS = lerp(originalNormal, bloodBump, bloodMask * 0.5);
    //finalNormal_TS = normalize(finalNormal_TS);
    //surface.N = normalize(mul(finalNormal_TS, TBN));

    float4 surfaceMap = texture_surfacemap.Sample(sampler_objectshader, input.uvsets.xy);
    surface.create(GetMaterial(), color, surfaceMap);
    

    float bloodRoughness = lerp(0.5, 0.1, wetness);
    surface.roughness = lerp(surface.roughness, bloodRoughness, bloodMask);
    surface.metalness = lerp(surface.metalness, 0.0, bloodMask);
    surface.emissiveColor = float4(0, 0, 0, 0);
    surface.update();
    
    surface.pixel = pixel;
    surface.screenUV = ScreenCoord;
    surface.layerMask = GetMaterial().layerMask;
    
    float3 ambient = GetAmbient(surface.N);
    Lighting lighting;
    lighting.create(0, 0, ambient, 0);

    //TiledLighting(surface, lighting);
    //PE: envMap was missing.
    float3 envAmbient = 0;
    TiledLighting(surface, lighting, envAmbient);
    lighting.indirect.diffuse += envAmbient;

    ApplyLighting(surface, lighting, color);
    ApplyFog(dist, g_xCamera_CamPos, surface.V, color);
    
    return max(0, color);
}


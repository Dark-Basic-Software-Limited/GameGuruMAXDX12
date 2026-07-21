// GGFrameCompat.hlsli - Compatibility shim mapping old DX11 FrameCB/CameraCB
// field names to new Wicked Engine DX12 accessor patterns.
//
// Include this header AFTER globals.hlsli (or PBR/globals.hlsli) in shaders
// that reference old g_xFrame_* or g_xCamera_* field names.
//
// The old engine had a monolithic FrameCB with all fields as direct struct
// members. The new engine splits these across multiple accessor functions:
//   - GetFrame()   -> ShaderFrame (time, options, etc.)
//   - GetScene()   -> ShaderScene (globalenvmap, instance/material buffers)
//   - GetWeather() -> ShaderWeather (fog, sky, sun, ambient, etc.)
//   - GetCamera()  -> ShaderCamera (matrices, resolution, culling tiles, etc.)
//
// Additionally, half3-packed fields (sun direction/color, ambient, horizon,
// zenith) are now unpacked via helper functions (GetSunDirection(), etc.)
// declared in the new globals.hlsli.
//
// CUSTOM GAMEGURU FIELDS:
//   The old FrameCB included GameGuru-specific fields that do NOT exist in the
//   new engine's FrameCB. These fields are already provided by the per-shader
//   custom constant buffers (TerrainCB at b2, TreeCB at b2, GrassCB at b2) or
//   by PBR/globals.hlsli helper functions. See Section 3 below for details.
//
// CUBEMAP RENDERING:
//   The old engine used CubemapRenderCB at b8 with xCubemapRenderCams[6].VP.
//   The new engine replaces this with CameraCB.cameras[camera_index], accessed
//   via GetCameraIndexed(camera_index).view_projection. Env probe VS shaders
//   must migrate from xCubemapRenderCams[cubeFaceID].VP to
//   GetCameraIndexed(cubeFaceID).view_projection. See Section 4 below.

#ifndef GG_FRAMECOMPAT_HLSLI
#define GG_FRAMECOMPAT_HLSLI

// ============================================================================
// 1. FrameCB field compatibility macros
//    Old: g_xFrame_FieldName  ->  New: GetFrame()/GetScene()/GetWeather()/GetCamera()
// ============================================================================

// Time and frame
#define g_xFrame_Time                     GetFrame().time
#define g_xFrame_TimePrev                 GetFrame().time_previous
#define g_xFrame_DeltaTime                GetFrame().delta_time
#define g_xFrame_FrameCount               GetFrame().frame_count
#define g_xFrame_Options                  GetFrame().options
#define g_xFrame_TemporalAASampleRotation GetFrame().temporalaa_samplerotation

// Resolution (moved to ShaderCamera in new engine)
#define g_xFrame_InternalResolution       GetCamera().internal_resolution
#define g_xFrame_InternalResolution_rcp   GetCamera().internal_resolution_rcp

// Fog (moved to ShaderWeather.fog in new engine)
// Old layout: float4(start, end, height_start, height_end)
// New layout: ShaderWeather.fog has .start, .density (not .end), .height_start, .height_end
// Reconstruct old end = start + 1/density
#define g_xFrame_Fog                      float4(GetWeather().fog.start, GetWeather().fog.start + 1.0 / max(GetWeather().fog.density, 0.0001), GetWeather().fog.height_start, GetWeather().fog.height_end)

// Sun/Sky (moved to ShaderWeather in new engine, with half3 packing)
// The new globals.hlsli provides GetSunDirection(), GetSunColor(), etc.
// that unpack from uint2 fields. These macros preserve old naming.
#define g_xFrame_SunDirection             float3(GetSunDirection())
#define g_xFrame_SunColor                 float3(GetSunColor())
#define g_xFrame_Ambient                  float4(float3(GetAmbientColor()), 1.0)
#define g_xFrame_Horizon                  float4(float3(GetHorizonColor()), 1.0)
#define g_xFrame_Zenith                   float4(float3(GetZenithColor()), 1.0)

// Sky settings
#define g_xFrame_SkyExposure              GetWeather().sky_exposure
#define g_xFrame_Stars                    GetWeather().stars

// Env probes (replaced by ShaderEntityIterator in new engine)
// Old: separate offset/count uint fields in FrameCB
// New: packed in ShaderEntityIterator, accessed via probes().first_item()/item_count()
#define g_xFrame_EnvProbeArrayOffset      probes().first_item()
#define g_xFrame_EnvProbeArrayCount       probes().item_count()
// NOTE: g_xFrame_EnvProbeMipCount no longer exists as a CB field.
// The new engine uses cubemap.GetDimensions() dynamically.
// Hardcoded to 8 (typical for 256x256 cubemaps) for backward compatibility.
#define g_xFrame_EnvProbeMipCount         8
#define g_xFrame_GlobalEnvProbeIndex      GetScene().globalprobe

// Lights (replaced by ShaderEntityIterator in new engine)
#define g_xFrame_LightArrayOffset         lights().first_item()
#define g_xFrame_LightArrayCount          lights().item_count()

// Entity culling tile count (moved to ShaderCamera in new engine)
#define g_xFrame_EntityCullingTileCount   GetCamera().entity_culling_tilecount

// Blue noise
#define g_xFrame_BlueNoisePhase           GetFrame().blue_noise_phase

// Gamma (no longer in new FrameCB - use constants)
#define g_xFrame_Gamma                    2.2
// StaticSkyGamma: old engine stored 0 (no sky), 1 (HDR), or LDR gamma value.
// New engine checks IsStaticSky() via GetScene().globalenvmap >= 0.
#define g_xFrame_StaticSkyGamma           (IsStaticSky() ? 1.0 : 0.0)

// ============================================================================
// 2. CameraCB field compatibility macros
//    Old: g_xCamera_FieldName  ->  New: GetCamera().field_name
// ============================================================================

#define g_xCamera_CamPos          GetCamera().position
#define g_xCamera_VP              GetCamera().view_projection
#define g_xCamera_View            GetCamera().view
#define g_xCamera_Proj            GetCamera().projection
#define g_xCamera_InvV            GetCamera().inverse_view
#define g_xCamera_InvP            GetCamera().inverse_projection
#define g_xCamera_InvVP           GetCamera().inverse_view_projection
#define g_xCamera_At              GetCamera().forward
#define g_xCamera_Up              GetCamera().up
#define g_xCamera_ZNearP          GetCamera().z_near
#define g_xCamera_ZFarP           GetCamera().z_far
#define g_xCamera_ZNearP_rcp      GetCamera().z_near_rcp
#define g_xCamera_ZFarP_rcp       GetCamera().z_far_rcp
#define g_xCamera_ZRange          GetCamera().z_range
#define g_xCamera_ZRange_rcp      GetCamera().z_range_rcp
#define g_xCamera_ClipPlane       GetCamera().clip_plane

// ============================================================================
// 3. Custom GameGuru FrameCB fields - NO NEW-ENGINE EQUIVALENT
//
//    These fields were added to the old FrameCB by GameGuru. They do NOT exist
//    in the new Wicked Engine FrameCB/ShaderWeather/ShaderScene structs.
//
//    RESOLUTION: These are already provided through the per-shader custom
//    constant buffers or through PBR/globals.hlsli helper functions:
//
//    Field                    Source in GG shaders
//    -----                    --------------------
//    g_xFrame_TreeWind        Old FrameCB at b0 (used in GGTreesConstants.hlsli)
//                             -> Must be added to TreeCB at b2, OR kept in a
//                                custom GGFrameCB extension
//
//    g_xFrame_TreeSubSurfaceScattering
//                             Old FrameCB at b0 (used in PBR/lightingHF.hlsli
//                             DirectionalLight for vegetation SSS)
//                             -> Must be in a shared CB (used by tree shaders)
//
//    g_xFrame_WaterColor      Old FrameCB at b0 (used in GGCommonFunctions.hlsli)
//    g_xFrame_WaterHeight     Old FrameCB at b0 (used in GGCommonFunctions.hlsli,
//                                                PBR/lightingHF.hlsli)
//    g_xFrame_WaterFogMin     Old FrameCB at b0 (used in GGCommonFunctions.hlsli)
//    g_xFrame_WaterFogMax     Old FrameCB at b0 (used in GGCommonFunctions.hlsli)
//    g_xFrame_WaterFogMinAmount Old FrameCB at b0 (used in GGCommonFunctions.hlsli)
//                             -> These water fields are used by ALL shaders via
//                                GGCommonFunctions.hlsli. Must be in a shared CB
//                                or added to the new FrameCB.
//
//    g_xFrame_FogColor        Old FrameCB at b0 (used via PBR/globals.hlsli GetFogColor())
//    g_xFrame_FogOpacity      Old FrameCB at b0 (used via PBR/globals.hlsli GetFogOpacity())
//                             -> Used by GetFogColor()/GetFogOpacity() in PBR/globals.hlsli.
//                                Must be available to all shaders.
//
//    g_xFrame_SunEnergy       Old FrameCB at b0 (used via PBR/globals.hlsli GetSunEnergy())
//                             -> Used by GetSunEnergy() in PBR/globals.hlsli.
//                                Must be available to all shaders.
//
//    g_xFrame_WetmapMul       Old FrameCB at b0 (NOT used in any GG terrain/veg shader)
//    g_xFrame_WetmapMulInstant Old FrameCB at b0 (NOT used in any GG terrain/veg shader)
//    g_xFrame_WetmapOffset    Old FrameCB at b0 (NOT used in any GG terrain/veg shader)
//                             -> Can be dropped unless object shaders need them.
//
//    RECOMMENDED C++ STRATEGY:
//    Option A: Add a GGCustomFrameCB at an unused register (e.g. b3 or b4) containing
//              all custom fields. Include it in all shaders via this header.
//    Option B: Extend the per-shader CBs (TerrainCB, TreeCB, GrassCB) to include
//              the water/fog/sun fields each shader needs. Requires duplication.
//    Option C: Patch the new engine's FrameCB struct to include GG custom fields.
//              Cleanest for shaders but couples to engine internals.
//
//    The chosen strategy must be implemented in C++ (setting the CB values) and
//    the macros below must be updated to point to the correct CB fields.
// ============================================================================

// GGCustomFrameCB at register(b4) provides these custom fields.
// Include GGCustomFrameCB.hlsli before this header (PBR/globals.hlsli does this).
#define g_xFrame_TreeWind                 ggCustomFrame.treeWind
#define g_xFrame_TreeSubSurfaceScattering ggCustomFrame.treeSubSurfaceScattering
#define g_xFrame_SunEnergy                ggCustomFrame.sunEnergy
#define g_xFrame_DeSaturate               ggCustomFrame.deSaturate
#define g_xFrame_WaterColor               ggCustomFrame.waterColor
#define g_xFrame_WaterHeight              ggCustomFrame.waterHeight
#define g_xFrame_WaterFogMin              ggCustomFrame.waterFogMin
#define g_xFrame_WaterFogMax              ggCustomFrame.waterFogMax
#define g_xFrame_WaterFogMinAmount        ggCustomFrame.waterFogMinAmount
#define g_xFrame_FogOpacity               ggCustomFrame.fogOpacity
#define g_xFrame_FogColor                 float4(ggCustomFrame.fogColor, 1.0)
#define g_xFrame_FogHeightSky             ggCustomFrame.fogHeightSky
#define g_xFrame_Cloudiness               ggCustomFrame.cloudiness
#define g_xFrame_CloudScale               ggCustomFrame.cloudScale
#define g_xFrame_CloudSpeed               ggCustomFrame.cloudSpeed

// ============================================================================
// 3b. Shadow system field stubs
//     The old engine had these as FrameCB members. The new engine uses a
//     completely different shadow atlas system. These provide hardcoded
//     defaults so that lightingHF.hlsli and GGTerrainPS.hlsl compile.
//     At runtime, the shadow system must be migrated to the new atlas-based
//     approach (see lightingHF.hlsli migration in Task #5).
// ============================================================================

// Shadow resolution and kernel sizes (hardcoded reasonable defaults)
#define g_xFrame_ShadowRes2D              1024.0
#define g_xFrame_ShadowKernel2D           (1.0 / 1024.0)
#define g_xFrame_ShadowResSpot2D          512.0
#define g_xFrame_ShadowKernelSpot2D       (1.0 / 512.0)
#define g_xFrame_ShadowResCube            256.0
#define g_xFrame_ShadowKernelCube         (1.0 / 256.0)
#define g_xFrame_ShadowCascadeCount       3

// Directional light index: in old engine this was a FrameCB field.
// In the new engine, directional lights are iterated via directional_lights().
// Use first directional light as default (index = first_item of directional_lights).
#define g_xFrame_DirectionalLightIndex    directional_lights().first_item()

// ============================================================================
// 3c. Voxel GI field stubs
//     The old engine had voxel radiance fields in FrameCB. The new engine
//     has a completely different VXGI system (ShaderVoxelGrid in ShaderScene).
//     These stubs provide reasonable defaults for compilation.
// ============================================================================

#define g_xFrame_VoxelRadianceDataRes          128
#define g_xFrame_VoxelRadianceDataRes_rcp      (1.0 / 128.0)
#define g_xFrame_VoxelRadianceDataSize         32.0
#define g_xFrame_VoxelRadianceDataSize_rcp     (1.0 / 32.0)
#define g_xFrame_VoxelRadianceDataCenter       float3(0, 0, 0)
#define g_xFrame_VoxelRadianceDataMIPs         7
#define g_xFrame_VoxelRadianceMaxDistance       0.0
#define g_xFrame_VoxelRadianceRayStepSize       1.0
#define g_xFrame_VoxelRadianceNumCones         16
#define g_xFrame_VoxelRadianceNumCones_rcp     (1.0 / 16.0)

// ============================================================================
// 3d. Atmosphere compatibility
//     The old engine had g_xFrame_Atmosphere as a struct in FrameCB.
//     The new engine has ShaderWeather.atmosphere (AtmosphereParameters).
// ============================================================================

#define g_xFrame_Atmosphere               GetWeather().atmosphere

// ============================================================================
// 4. CubemapRenderCB migration (b8 -> CameraCB cameras[] array)
//
//    Old engine: CubemapRenderCB at b8 with xCubemapRenderCams[6] array,
//                each containing a VP matrix and properties uint4.
//                Used by env probe VS shaders:
//                  - GGTerrainEnvProbeVS.hlsl
//                  - GGTreesHighEnvProbeVS.hlsl
//                  - GGTreeBranchesHighEnvProbeVS.hlsl
//
//    New engine: CubemapRenderCB does NOT exist. The new engine uses the
//                CameraCB.cameras[] array (16 ShaderCamera entries at b1).
//                For env probe rendering, the C++ side sets cameras[0..5]
//                to the 6 cube face view-projection matrices.
//                VS shaders use GetCameraIndexed(cubeFaceID).view_projection.
//
//    Migration for env probe VS shaders:
//      OLD: mul(xCubemapRenderCams[cubeFaceID].VP, pos)
//      NEW: mul(GetCameraIndexed(cubeFaceID).view_projection, pos)
//
//    The compatibility macro below provides a bridge:
// ============================================================================

// Bridge struct to emulate old CubemapRenderCam access pattern via new CameraCB
#ifndef __cplusplus
struct GG_CubemapRenderCamCompat
{
	float4x4 VP;
};

inline GG_CubemapRenderCamCompat GG_GetCubemapRenderCam(uint faceIndex)
{
	GG_CubemapRenderCamCompat cam;
	cam.VP = GetCameraIndexed(faceIndex).view_projection;
	return cam;
}
#endif // !__cplusplus

// Compatibility: xCubemapRenderCams[i].VP -> GetCameraIndexed(i).view_projection
// NOTE: This is a function-based approach. Shaders must change array syntax:
//   OLD: xCubemapRenderCams[cubeFaceID].VP
//   NEW: GG_GetCubemapRenderCam(cubeFaceID).VP
// Or simply use GetCameraIndexed(cubeFaceID).view_projection directly.

// ============================================================================
// 5. Resource access compatibility
//    Old: EntityArray[i] / MatrixArray[i]  ->  New: load_entity(i) / load_entitymatrix(i)
//    Old: STRUCTUREDBUFFER(EntityTiles,...) ->  New: load_entitytile(i)
// ============================================================================

// In the old engine, EntityArray and MatrixArray were StructuredBuffers at slots.
// In the new engine, entities are inline in FrameCB accessed via load_entity().
#define EntityArray_Load(index)   load_entity(index)
#define MatrixArray_Load(index)   load_entitymatrix(index)

// For shaders that declare STRUCTUREDBUFFER(EntityTiles, uint, slot),
// the new engine uses load_entitytile() for bindless structured buffer access.
#define EntityTiles_Load(index)   load_entitytile(index)

#endif // GG_FRAMECOMPAT_HLSLI

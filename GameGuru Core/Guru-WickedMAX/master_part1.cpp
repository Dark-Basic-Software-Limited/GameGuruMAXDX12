//
// MasterRenderer Functions
//
#include "GGAnimBridge.h"
#include "wiGraphicsDevice_DX12.h" // Phase 5: For DX12 ImGui rendering in Compose
#include "GGTerrain/GGTerrainWicked.h"
#include "wiProfiler.h"
#include "wiTerrain.h" // GGMAX 1.71: gg_svt_atlas_height (setup.ini svtatlasheight)
#include <atomic>

// GGMAX delta 1.29: engine-side 30fps animation throttle flags (defined in WickedEngine/wiScene.cpp).
// MasterRenderer::Update drives these per-frame from the editor "Lower Animation & LUA Speed" checkbox,
// before __super::Update runs the animation jobs. Replaces the removed GGAnimBridge pre-hook.
namespace wi { namespace scene {
	extern std::atomic<uint32_t> gg_anim30fps_enabled;
	extern std::atomic<uint32_t> gg_anim30fps_frame;
	extern std::atomic<uint32_t> gg_anim30fps_dist2;
	// GGMAX delta 1.35: frustum-visibility animation pause (defined in WickedEngine/wiScene.cpp)
	extern std::atomic<uint32_t> gg_anim_vis_pause_frames;
	extern std::atomic<uint32_t> gg_anim_vis_pause_neardist2;
} }
// GGMAX delta 1.30: engine-side apparent-size object cull threshold (defined in WickedEngine/wiRenderer.cpp).
// MasterRenderer::Update drives it per-frame from the editor "Apparent Size" slider (see below).
namespace wi { namespace renderer {
	extern std::atomic<uint32_t> gg_apparent_cull_bits;
} }

// Cached profiler text — updated in Compose() when GPU ranges are active.
// GetTextData() called during Update misses GPU sub-ranges because BeginFrame()
// clears in_use before Render creates GPU ranges. Capturing in Compose() gets all ranges.
static std::string g_cachedProfilerText;
std::string GGPerf_GetCachedProfilerText() { return g_cachedProfilerText; }

// GGMAX 1.67: main-camera poly count for the Performance HUD (latched per frame in the engine)
uint64_t GGPerf_GetPolyCount() { return wi::renderer::GG_GetMainCameraPolyCount(); }

// GGMAX 1.71: setup.ini `svtatlasheight` bridge. The terrain virtual-texture physical atlas is a
// fixed cost (16384 tall = a 768 MB tile pool) regardless of how much a level uses, and measured
// residency is only ~26% of it. Must be set before the atlas is created (first terrain update),
// which is why it comes from setup.ini rather than a runtime harness command. See VRAM_CENSUS.md.
void GGSetSVTAtlasHeight(int height)
{
	if (height >= 2048 && height <= 16384)
	{
		wi::terrain::gg_svt_atlas_height = (uint32_t)height;
	}
}

// GGMAX low-VRAM preset bridge (setup.ini `lowvram`, harness SET_LOWVRAM). One entry point so the
// preset's members stay in one place — the old-namespace setup.ini reader cannot declare engine
// namespace externs at block scope, and spraying them through it would rot.
//
// Members today:
//   * grass draw distance cap  (gg_lowvram, GGTerrainWicked.cpp) — content side
//   * lazy object PSOs         (wi::renderer::gg_pso_lazy_object) — floor side, −633 MB measured
//
// TIMING, measured not assumed: the PSO flag must be set before wi::renderer::LoadShaders builds
// the object pipelines, and setup.ini's main parse (FPSC_LoadSETUPINI) runs LATER than that — the
// first attempt wired it only there and it did nothing at all. The key is therefore also read in
// GetSetupIniEarly(), which main() calls before the engine starts. That is also why SET_LOWVRAM
// cannot enable this half: no harness command lands early enough.
// The grass cap has no such constraint; grass entities are built per chunk at spawn time.
void GGSetLowVRAM(int on)
{
	extern bool gg_lowvram;                 // GGTerrainWicked.cpp
	gg_lowvram = (on != 0);
	wi::renderer::gg_pso_lazy_object = (on != 0);
}
void GGSetLowVRAMGrassDist(float inches)
{
	extern float gg_lowvram_grass_dist;     // GGTerrainWicked.cpp
	if (inches > 0.0f) gg_lowvram_grass_dist = inches;
}
void GGSetLowVRAMGrassDensity(float scale01)
{
	extern float gg_lowvram_grass_density;  // GGTerrainWicked.cpp
	if (scale01 > 0.0f && scale01 <= 1.0f) gg_lowvram_grass_density = scale01;
}

// GGMAX 1.81: bridge for the Scene::Update caller tracer. Lives here because the harness is
// compiled outside the wi::scene namespace and a block-scope extern would mangle wrongly.
unsigned int GG_GetSceneUpdateCallsBridge(const void** ret, const void** scene,
	unsigned long long* frame, float* dt, unsigned int max_out)
{
	return wi::scene::GG_GetSceneUpdateCalls(ret, scene, frame, dt, max_out);
}

// GGMAX wall-gap tracer bridges for old-namespace files (main.cpp pump timing / master preamble)
unsigned long long GGPerf_TraceNowUs(void) { return wi::profiler::gg_trace_now_us(); }
void GGPerf_TraceMarkId(const char* prefix, unsigned int id) { wi::profiler::gg_trace_mark_id(prefix, id); }
void GGPerf_TraceMark(const char* name) { wi::profiler::gg_trace_mark(name); }
extern std::atomic<unsigned long long> gg_dbg_pump_dispatches, gg_dbg_pump_us; // wiProfiler.cpp, global namespace
void GGPerf_TracePumpAccum(unsigned long long us)
{
	gg_dbg_pump_dispatches.fetch_add(1, std::memory_order_relaxed);
	gg_dbg_pump_us.fetch_add(us, std::memory_order_relaxed);
}

// Phase 3: Forward declarations for GG custom draw functions (terrain/trees/grass)
extern "C" void GGTerrain_Draw_Prepass(const wi::primitive::Frustum*, wi::graphics::CommandList);
extern "C" void GGTerrain_Draw_Prepass_Reflections(const wi::primitive::Frustum*, wi::graphics::CommandList);
extern "C" void GGTerrain_Draw(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGTerrain_Draw_Transparent(const wi::primitive::Frustum*, wi::graphics::CommandList);
extern "C" void GGTerrain_Draw_ShadowMap(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGTerrain_Draw_EnvProbe(const wi::primitive::Sphere*, const wi::primitive::Frustum*, uint32_t, wi::graphics::CommandList);
extern "C" void GGTrees_Draw_Prepass(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGTrees_Draw(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGTrees_Draw_ShadowMap(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGTrees_Draw_EnvProbe(const wi::primitive::Sphere*, const wi::primitive::Frustum*, uint32_t, wi::graphics::CommandList);
extern "C" void GGGrass_Draw_Prepass(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGGrass_Draw(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGGrass_Draw_ShadowMap(const wi::primitive::Frustum*, int, wi::graphics::CommandList);
extern "C" void GGTerrain_Draw_Debug(wi::graphics::CommandList);
extern "C" void GGTerrain_Draw_Overlay(wi::graphics::CommandList);
extern "C" void GGTerrain_VirtualTexReadBack(const wi::graphics::Texture&, uint32_t, wi::graphics::CommandList);

void MasterRenderer::Load()
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	__super::Load();

	// switch OFF JOLT (for now)
	wi::physics::SetEnabled(false);

	// remove VSYNC cap
	wiEvent::SetVSync( false );

	// clear image management system early
	char pCurrentDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, pCurrentDir);
	WickedCall_InitImageManagement(pCurrentDir);

	// default wicked settings
	setSSREnabled ( false ); //PE: Not on by default.
	setReflectionsEnabled ( true );
	setFXAAEnabled ( false ); //PE: We already have MSAA 8 so.
	#ifdef POSTPROCESSRAIN
	setRainEnabled(false); //PE: test post process shader.
	setRainTextures("Files\\effectbank\\common\\rain_color.png", "Files\\effectbank\\common\\rain_normal.png");
	setRainOpacity(0.0);
	setRainScaleX(1.0);
	setRainScaleY(1.0);
	setRainRefreactionScale(0.01);
	#endif
	#ifdef POSTPROCESSSNOW
	setSnowLayers(15.0);
	setSnowDepth(0.5);
	setSnowWindiness(0.5);
	setSnowSpeed(0.15);
	setSnowOpacity(1.0);
	setSnowOffset(0.0);
	#endif
	setBloomEnabled ( true ); 
	setShadowsEnabled ( true );
	wiRenderer::SetTessellationEnabled(false); //PE: Tessellation dont work like this it has to be set per mesh, so have never worked.
	setLightShaftsEnabled ( true );
	setLightShaftsStrength ( 0.18f ); // PORT TRANSLATE 2026-07-29: DX11 exposure parity (see Wicked_Update_Visuals)
	setBloomThreshold ( 2.0f );
	//setBloomStrength( 1.0f );

	// only activated when in TEST LEVEL
	setEyeAdaptionEnabled( false );

	// disable wicked backlog, can draw behind imgui , can be seen sometimes.
	if (wiBackLog::isActive()) 
	{
		wiBackLog::Toggle();
	}

	// for best terrain rendering
	wiGraphics::SamplerDesc desc = wiRenderer::GetSampler ( SAMPLER_OBJECTSHADER )->GetDesc ( );
	desc.filter = wiGraphics::Filter::ANISOTROPIC;

	// create cloudy sky by default
	Scene weatherscene;
	g_weatherEntityID = CreateEntity();
	auto& weather = weatherscene.weathers.Create ( g_weatherEntityID );
	weather.ambient = XMFLOAT3(0.5f, 0.5f, 0.5f);
	weather.horizon = XMFLOAT3 ( 0.38f, 0.38f, 0.38f );
	weather.zenith = XMFLOAT3 ( 0.42f, 0.42f, 0.42f );
	
	//PE: We dont want any lightshaft from the sun. before we activate it.
	weather.volumetricCloudParameters.layerFirst.coverageAmount = 0.0f;
	weather.volumetricCloudParameters.layerFirst.windSpeed = 0.0f;
	weather.fogStart = 0;
	weather.fogDensity = 0;
	weather.SetRealisticSky( true );

	weather.SetVolumetricClouds( true );

	wiScene::GetScene ( ).Merge ( weatherscene );

	// create directional light from sun
	Entity entitySunLight = wiScene::GetScene ( ).Entity_CreateLight ( "sunLight", XMFLOAT3 ( 0, 3, 0 ), XMFLOAT3 ( 1, 1, 1 ), 4, 600 );
	g_entitySunLight = entitySunLight;
	LightComponent* lightSun = wiScene::GetScene ( ).lights.GetComponent ( entitySunLight );
	lightSun->SetCastShadow ( true );
	lightSun->SetVisualizerEnabled ( false );
	lightSun->direction = XMFLOAT3 ( 0.25, -0.5, 0.25 );
	lightSun->color = XMFLOAT3 ( 1.0, 1.0, 1.0 );
	lightSun->SetType ( wiScene::LightComponent::DIRECTIONAL );
	// Sun volumetrics OFF (2026-07-18): the volumetric raymarch scales with fog
	// density, which was 0 until the fog wiring landed — so this has never been
	// visible. With fog active it renders god-rays tinted by TRANSPARENT
	// shadow casters, and the editor's translucent red/green zone markers
	// painted giant coloured beams across the sky. DX11 shows no sun shafts.
	// Key 8 (perf toggles) re-enables for experiments; if god-rays are wanted
	// later, first stop editor markers casting transparent shadows.
	lightSun->SetVolumetricsEnabled( false );
	// Production DX11 cascade splits in world inches (old engine GGREDUCED block);
	// stock Wicked default {8,80,800} is ~20m here. Kept in sync by
	// WickedCall_SetShadowRange whenever the level's visuals apply.
	lightSun->cascade_distances = { 380.0f, 950.0f, 7500.0f, 30000.0f, 500000.0f };

	// Production DX11 parity: staggered cascade refresh (c0 every frame, then
	// /2 /3 /4 /9) — skipped cascades keep their atlas contents + frozen
	// matrices; a >64" camera move or any atlas/rect change forces a refresh.
	// Wicked delta 1.11; the DELAYED_SHADOWS harness command A/Bs it live.
	// Respect the project's Delayed Shadows setting (Graphics & Performance panel): at HIGHEST
	// quality g_bDelayedShadows=false -> every-frame cascades -> stable shadows (no cliff-shadow
	// flicker under camera movement). M-Visuals apply + the checkbox keep this in sync live.
	extern bool g_bDelayedShadows;
	wi::renderer::SetDelayedShadowCascadesEnabled( g_bDelayedShadows );
	// Shadow LOD override renders each object into the shadow at a per-cascade projected-size LOD
	// (Wicked default ON) — for the terrain this picks a DIFFERENT, threshold-oscillating LOD mesh
	// than the visible chunk, so the cast shadow flips between two terrain shapes as the cascade
	// shifts (the author's own warning: "can result in shadow mismatch"). Tie it to the Delayed
	// Shadows setting: OFF (highest quality) = shadows use the stable main-view LOD, no flicker.
	wi::renderer::SetShadowLODOverrideEnabled( g_bDelayedShadows );

	// LB: sun needs lens flare texture
	int iFlareCount = 3;
	lightSun->lensFlareRimTextures.resize(iFlareCount);
	lightSun->lensFlareNames.resize(iFlareCount);
	for (int iFlareChain = 0; iFlareChain < iFlareCount; iFlareChain++)
	{
		std::string fileName;
		if (iFlareChain == 0) fileName = "Files\\lensflares\\flare1.jpg";
		if (iFlareChain == 1) fileName = "Files\\lensflares\\flare2.jpg";
		if (iFlareChain == 2) fileName = "Files\\lensflares\\flare3.jpg";
		lightSun->lensFlareRimTextures[iFlareChain] = wiResourceManager::Load(fileName);
		lightSun->lensFlareNames[iFlareChain] = fileName;
	}

	// force window to maximised view
	HWND hWnd = GetActiveWindow();
	ShowWindow(hWnd, SW_MAXIMIZE);

	// Phase 3: Set up custom scene draw callbacks
	// Terrain only (trees/grass disabled — see GRASSISSUE.md for details)
	// When ggterrain_use_wicked_terrain is active, all callbacks return early
	customDraw_Prepass = [](const Frustum* frustum, CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw_Prepass(frustum, cmd);
	};
	customDraw_Prepass_Reflections = [](const Frustum* frustum, CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw_Prepass_Reflections(frustum, cmd);
	};
	customDraw_Opaque = [](const Frustum* frustum, int mode, CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw(frustum, mode, cmd);
	};
	customDraw_Transparent = [](const Frustum* frustum, CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw_Transparent(frustum, cmd);
	};
	wi::renderer::customDraw_ShadowMap = [](const Frustum* frustum, int cascade, CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw_ShadowMap(frustum, cascade, cmd);
	};
	wi::renderer::customDraw_EnvProbe = [](const wi::primitive::Sphere* culler, const Frustum* frusta, uint32_t frustum_count, CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw_EnvProbe(culler, frusta, frustum_count, cmd);
	};
	customDraw_Compose = [](CommandList cmd) {
		if (ggterrain_use_wicked_terrain) return;
		GGTerrain_Draw_Debug(cmd);
		GGTerrain_Draw_Overlay(cmd);
	};
	// TODO: Enable when the prepass has a second RT for terrain page readback (SV_TARGET1).
	// Currently the prepass only has rtPrimitiveID_render at SV_TARGET0, so terrain page IDs
	// written to SV_TARGET1 are discarded. Reading rtPrimitiveID_render would produce garbage.
	// The CPU-based progressive page seeding (in GGTerrain_Update) provides the fallback.
	//customDraw_AfterPrepass = [](const Texture& texReadBack, uint32_t sampleCount, CommandList cmd) {
	//	GGTerrain_VirtualTexReadBack(texReadBack, sampleCount, cmd);
	//};

}

void MasterRenderer::PreUpdate()
{
	__super::PreUpdate();
}

void MasterRenderer::Update(float dt)
{
	// otherwise continue
	if (m_bRenderingVR == false)
	{
#ifdef OPTICK_ENABLE
		OPTICK_EVENT("GuruLoopLogic");
#endif
		// regular update mode
		auto range = wiProfiler::BeginRangeCPU("Update - Logic (Total)");
		bool bFullyInitialised = GuruLoopLogic();
		wiProfiler::EndRange(range);

		// no further than logic while in splash show mode
		// DX12: Must NOT return early — __super::Update(dt) must always run so
		// RenderPath3D prepares render targets before Render()/Compose() are called.
		if (g_iShowSplashForFirstFewCycles <= 0 && bFullyInitialised == true )
		{
			// normal update
			wiScene::CameraComponent &camera = wiScene::GetCamera();

			// must be outside a render pass and only called once, even if VR renders twice
			CommandList cmd = wiGraphics::GetDevice()->BeginCommandList();
			auto range = wiProfiler::BeginRangeCPU("Update - Particles");
			gpup_update(dt, cmd);
			wiProfiler::EndRange(range);

			// terrain processing (if used)
			if (t.visuals.bEnableEmptyLevelMode == false)
			{
				extern int g_iDisableTerrainSystem;
				auto range3 = wiProfiler::BeginRangeCPU("Update - Terrain");
				extern bool bImGuiRenderTargetFocus;
				auto rangeT1 = wiProfiler::BeginRangeCPU("Terrain - GG Core");
				GGTerrain_Update(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd, bImGuiRenderTargetFocus);
				wiProfiler::EndRange(rangeT1);
				if (ggterrain_use_wicked_terrain)
				{
					ggterrain_draw_enabled = 0;  // suppress all old draw callbacks
					auto rangeT2 = wiProfiler::BeginRangeCPU("Terrain - Wicked Bridge");
					GGTerrainWicked_Update(camera);
					wiProfiler::EndRange(rangeT2);
				}
				else
				{
					ggterrain_draw_enabled = 1;
				}
				if (g_iDisableTerrainSystem == 0)
				{
					GGTrees_Update(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd, bImGuiRenderTargetFocus);
					auto rangeT3 = wiProfiler::BeginRangeCPU("Trees - FrustumCull");
					GGTrees_UpdateFrustumCulling(&camera);
					wiProfiler::EndRange(rangeT3);
					auto rangeT4 = wiProfiler::BeginRangeCPU("Grass - GG Update");
					GGGrass_Update(&camera, cmd, bImGuiRenderTargetFocus);
					wiProfiler::EndRange(rangeT4);
				}
				wiProfiler::EndRange(range3);
			}
			else
			{
				// still need for terrain globals to update local params (for editable_size reading)
				GGTerrain_Update_EmptyLevel(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd);
			}
			
#ifdef WICKEDPARTICLESYSTEM
			auto range4 = wiProfiler::BeginRangeCPU("Update - Emitters");
			WickedCall_UpdateEmitters();
			wiProfiler::EndRange(range4);
#endif

      // now just prepared IMGUI, but actual render called from Wicked hook
			auto range2 = wiProfiler::BeginRangeCPU("Update - Render");
			GuruLoopRender();
			wiProfiler::EndRange(range2);
		}
	}

	//Disable wicked backlog, can draw behind imgui , can be seen sometimes. Make sure it is never activated.
	if (wiBackLog::isActive()) wiBackLog::Toggle();

	// P6: cap delta time to 1/30s to prevent animation jumps after alt-tab or stalls
	if (dt > (1.0f / 30.0f)) dt = 1.0f / 30.0f;

	// animation bridge pre-hook (before scene->Update runs animations)
	////GGAnimBridge_PreUpdate(&wiScene::GetScene(), dt);   // removed 89873913 (too slow); throttle now lives in the engine
	// GGMAX delta 1.29: drive the engine 30fps animation throttle from the editor "Lower Animation & LUA
	// Speed" checkbox. Set BEFORE __super::Update (which runs the animation jobs). Parity counter flips each
	// frame for a deterministic every-other-frame skip. g_animThrottleFarDist = distance (scene units) beyond
	// which to throttle; 0 = throttle all eligible armatures ((b) behaviour), >0 = only distant ((c) gate).
	{
		extern bool bEnable30FpsAnimations;
		extern float g_animThrottleFarDist;
		wiScene::gg_anim30fps_enabled.store(bEnable30FpsAnimations ? 1u : 0u, std::memory_order_relaxed);
		static uint32_t s_ggAnimFrame = 0;
		wiScene::gg_anim30fps_frame.store(++s_ggAnimFrame, std::memory_order_relaxed);
		float fd = g_animThrottleFarDist;
		if (fd < 0.0f) fd = 0.0f;
		if (fd > 60000.0f) fd = 60000.0f;   // guard: dist^2 must fit uint32 (60000^2 = 3.6e9 < 4.29e9)
		wiScene::gg_anim30fps_dist2.store((uint32_t)(fd * fd), std::memory_order_relaxed);
	}

	// GGMAX delta 1.35: frustum-visibility animation pause — drive the engine knobs per frame.
	// Objects not passing the main-view cull for N frames stop animation EVALUATION (timers keep
	// advancing). Near guard protects just-off-frame characters (visible shadows).
	{
		extern int g_animVisPauseFrames;
		extern float g_animVisPauseNearDist;
		uint32_t vp = (g_animVisPauseFrames < 0) ? 0u : (uint32_t)g_animVisPauseFrames;
		wiScene::gg_anim_vis_pause_frames.store(vp, std::memory_order_relaxed);
		float nd = g_animVisPauseNearDist;
		if (nd < 0.0f) nd = 0.0f;
		if (nd > 60000.0f) nd = 60000.0f;   // same uint32 dist^2 guard as above
		wiScene::gg_anim_vis_pause_neardist2.store((uint32_t)(nd * nd), std::memory_order_relaxed);
	}

	// GGMAX delta 1.30: drive the engine apparent-size object cull from the editor "Apparent Size" slider.
	// maxApparentSize is the stored slider value (0.000002..0.0002, default 0.000008). Map the amount ABOVE
	// the default to a radius/distance tangent cutoff (at/below default => 0 = draw everything; slide right
	// to cull distant specks). g_apparentCullDirect >= 0 overrides for harness tuning. Stored as fixed-point
	// micro-units (tangent * 1e6) in the uint32 atomic the engine reads.
	{
		extern float maxApparentSize;
		extern float g_apparentCullK;
		extern float g_apparentCullDirect;
		float tangent;
		if (g_apparentCullDirect >= 0.0f)
		{
			tangent = g_apparentCullDirect;
		}
		else
		{
			const float over = maxApparentSize - 0.000008f;   // amount past the default
			tangent = (over > 0.0f) ? over * g_apparentCullK : 0.0f;
		}
		if (tangent < 0.0f) tangent = 0.0f;
		if (tangent > 0.5f) tangent = 0.5f;   // clamp: never cull objects filling more than ~half the view
		wiRenderer::gg_apparent_cull_bits.store((uint32_t)(tangent * 1000000.0f + 0.5f), std::memory_order_relaxed);
	}

	// GGMAX: "Laptop" delayed-shadow mode = twice as aggressive. When Delayed Shadows + Laptop are both
	// ticked, hold the far directional cascades for 4 frames instead of 2 (another ~halving of the
	// staggered-cascade shadow cost). Driven per-frame so panel toggles / level loads all stay in sync;
	// interval is only consumed when the cascade staggering is actually enabled.
	{
		extern bool g_bDelayedShadows;
		extern bool g_bDelayedShadowsLaptop;
		wi::renderer::SetDelayedShadowCascadeInterval((g_bDelayedShadows && g_bDelayedShadowsLaptop) ? 4 : 2);
	}

	// super update
	auto range2 = wiProfiler::BeginRangeCPU("Update - Wicked (Total)");
	__super::Update(dt);
	wiProfiler::EndRange(range2);
}

void MasterRenderer::PostUpdate()
{
	////GGAnimBridge_PostUpdate(&wiScene::GetScene());
	__super::PostUpdate();
}

void MasterRenderer::ResizeBuffers(void)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	if ( GetInternalResolution().x == 0 || GetInternalResolution().y == 0 ) return;


	//PE: Must be the same, currently its out of synch with internal resolution.
	//PE: Cant change if in VR that have another resolution.
	if (!m_bUsingVR)
	{
		// TODO: Set3DResolution removed, use resolutionScale instead
		//master.masterrenderer.Set3DResolution(master.masterrenderer.GetPhysicalWidth(), master.masterrenderer.GetPhysicalHeight(), false); //GGREDUCED
	}

	//PE: Resizebuffers change FOV.

	__super::ResizeBuffers();

	GraphicsDevice* device = wiGraphics::GetDevice();
	HRESULT hr;

	if (GetDepthStencil() != nullptr)
	{
		TextureDesc desc;
		// GGREDUCED DX12/FSR: these outline RTs are bound in renderpass_Outline together with the
		// internal-res main depth (*GetDepthStencil()). When FSR lowers resolutionScale<1.0 the depth is
		// smaller than physical, so sizing the RTs at physical res mismatches the pass dimensions (D3D12
		// DEVICE_DRAW_VIEW_DIMENSION_MISMATCH). Size to internal res instead; Postprocess_Outline later
		// samples + composites them full-screen, so a smaller source simply upscales.
		desc.width = GetInternalResolution().x;
		desc.height = GetInternalResolution().y;
		desc.sample_count = 1;
		desc.format = Format::R8_UNORM;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;

		//PE: Get the below error when using dx11 debug layer, so switched to resolving MSAA instead.
		//D3D11 ERROR: ID3D11DeviceContext::Draw: The Shader Resource View dimension declared in the shader code (TEXTURE2D)
		//             does not match the view type bound to slot 23 of the Pixel Shader unit (TEXTURE2DMS).
		//             This mismatch is invalid if the shader actually uses the view (e.g. it is not skipped due to shader code branching).
		//             [ EXECUTION ERROR #354: DEVICE_DRAW_VIEW_DIMENSION_MISMATCH]

		hr = device->CreateTexture(&desc, nullptr, &rt_Outline);
		hr = device->CreateTexture(&desc, nullptr, &rt_Outline_Red);
		hr = device->CreateTexture(&desc, nullptr, &rt_Outline_Blue);
		if (getMSAASampleCount() > 1)
		{
			desc.sample_count = getMSAASampleCount();
			hr = device->CreateTexture(&desc, nullptr, &rt_MSAAOutline);
			hr = device->CreateTexture(&desc, nullptr, &rt_MSAAOutline_Red);
			hr = device->CreateTexture(&desc, nullptr, &rt_MSAAOutline_Blue);
		}
		assert(SUCCEEDED(hr));
	}

	{
		RenderPassDesc desc;

		//PE: New wickedrepo.
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(rt_Outline, RenderPassAttachment::LoadOp::CLEAR));

		//PE: We now use the MSAA desc.sample_count so dont need to resolve it.
		if (getMSAASampleCount() > 1)
		{
			desc.attachments[0].texture = rt_MSAAOutline;
			desc.attachments.push_back(RenderPassAttachment::Resolve(rt_Outline));
		}

		//wiTextureHelper::getBlack(),

		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				*GetDepthStencil(),
				RenderPassAttachment::LoadOp::LOAD,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY
			)
		);

		hr = device->CreateRenderPass(&desc, &renderpass_Outline);
		assert(hr);

		desc.attachments[0].texture = rt_Outline_Red;
		if (getMSAASampleCount() > 1)
		{
			desc.attachments[0].texture = rt_MSAAOutline_Red;
			desc.attachments[1].texture = rt_Outline_Red;
		}
		hr = device->CreateRenderPass(&desc, &renderpass_Outline_Red);
		assert(hr);

		desc.attachments[0].texture = rt_Outline_Blue;
		if (getMSAASampleCount() > 1)
		{
			desc.attachments[0].texture = rt_MSAAOutline_Blue;
			desc.attachments[1].texture = rt_Outline_Blue;
	}
		hr = device->CreateRenderPass(&desc, &renderpass_Outline_Blue);
		assert(hr);

	}

	GGTerrain_WindowResized();
	extern bool bTriggerFovUpdate;
	bTriggerFovUpdate = true; //PE: restore FOV.
}


float fGetHighlightThickness(void);

void Wicked_Render_Opaque_Scene(CommandList cmd)
{
	extern bool g_bNo2DRender;
	extern bool BackBufferSnapShotMode;
	if (bImGuiInTestGame)
	{
		if (bActivateStandaloneOutline && master_renderer->GetDepthStencil() != nullptr) //&& !translator.selected.empty())
		{
			GraphicsDevice* device = wiGraphics::GetDevice();

			XMFLOAT4 area;
			bool ImGuiHook_GetScissorArea(float* pX1, float* pY1, float* pX2, float* pY2);
			if (ImGuiHook_GetScissorArea(&area.x, &area.y, &area.z, &area.w) == true)
				//device->SetScissorArea(cmd, area);

			wiRenderer::BindCommonResources(cmd);
			XMFLOAT4 col = XMFLOAT4(0.8f, 0.8f, 0.8f, 0.8f);;
			wiRenderer::Postprocess_Outline(rt_Outline, cmd, 0.1f, 0.8f, col);
			device->EventEnd(cmd);
			area = { 0, 0, (float)master.masterrenderer.GetPhysicalWidth(), (float)master.masterrenderer.GetPhysicalHeight() };
			//device->SetScissorArea(cmd, area);
		}
		return;
	}

	if (!bImGuiInTestGame && (!g_bNo2DRender || BackBufferSnapShotMode) )
	{
		if (master_renderer->GetDepthStencil() != nullptr) //&& !translator.selected.empty())
		{
			GraphicsDevice* device = wiGraphics::GetDevice();

			XMFLOAT4 area;
			float thickness = fGetHighlightThickness();
			bool ImGuiHook_GetScissorArea(float* pX1, float* pY1, float* pX2, float* pY2);
			if (ImGuiHook_GetScissorArea(&area.x, &area.y, &area.z, &area.w) == true)
				//device->SetScissorArea(cmd, area);

			wiRenderer::BindCommonResources(cmd);
			XMFLOAT4 col = selectionColor;
			col.w *= 0.65; //opacity;
			wiRenderer::Postprocess_Outline(rt_Outline, cmd, 0.1f, thickness, col);
			device->EventEnd(cmd);

			col = selectionColorRed;
			col.w *= 0.65; //opacity;
			wiRenderer::Postprocess_Outline(rt_Outline_Red, cmd, 0.1f, thickness, col);
			device->EventEnd(cmd);

			col = selectionColorBlue;
			col.w *= 0.65; //opacity;
			wiRenderer::Postprocess_Outline(rt_Outline_Blue, cmd, 0.1f, thickness, col);
			device->EventEnd(cmd);

			area = { 0, 0, (float)master.masterrenderer.GetPhysicalWidth(), (float)master.masterrenderer.GetPhysicalHeight() };
			//device->SetScissorArea(cmd, area);

		}
	}
}

void MasterRenderer::Compose(CommandList cmd) const
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	// Always suppress wiProfiler::DrawData() — it calls AllocateGPU() during Compose
	// which can return an invalid allocation, causing an access violation and GPU TDR.
	// Profiler timing data is still collected normally; use GetTextData() to read it safely.
	wi::profiler::DisableDrawForThisFrame();

	// Cache profiler text during Compose when all GPU ranges are active.
	// GetTextData() called during Update would miss GPU sub-ranges.
	if (wi::profiler::IsEnabled())
		g_cachedProfilerText = wi::profiler::GetTextData();

	__super::Compose(cmd);

	// DX12: Render splash screen AFTER normal compose (drawn on top with opaque blend).
	// Moved from Master::Update where it created a separate render pass that was
	// immediately cleared by Application::Run's own render pass.
	extern int g_iShowSplashForFirstFewCycles;
	extern bool bAreWeAEditor;
	if (g_iShowSplashForFirstFewCycles > 0)
	{
		if (master.g_pSplashTexture.IsValid())
		{
			wi::image::Params fx;
			fx.enableFullScreen();
			fx.blendFlag = wi::enums::BLENDMODE_OPAQUE;
			wi::image::Draw(&master.g_pSplashTexture, fx, cmd);
		}
		if (bAreWeAEditor)
		{
			wi::Resource tex = wi::resourcemanager::Load("Files\\editors\\uiv3\\MAX-Logo-Square.png");
			if (tex.IsValid())
			{
				wi::graphics::Texture logoTex = tex.GetTexture();
				if (logoTex.IsValid())
				{
					wi::image::Params logoFx;
					logoFx.disableFullScreen();
					logoFx.pos.x = master.canvas.GetLogicalWidth() * 0.5f;
					logoFx.pos.y = master.canvas.GetLogicalHeight() * 0.5f;
					logoFx.pivot = XMFLOAT2(0.5f, 0.5f);
					logoFx.siz.x = (float)logoTex.desc.width;
					logoFx.siz.y = (float)logoTex.desc.height;
					logoFx.blendFlag = wi::enums::BLENDMODE_ALPHA;
					wi::image::Draw(&logoTex, logoFx, cmd);
				}
			}
		}
		return; // skip ImGui during splash
	}
	// Phase 5: Render ImGui draw data using DX12 backend
	extern bool bImGuiInitDone;
	if (bImGuiInitDone)
	{
		extern bool ImGui_DX12_IsInitialized();
		if (ImGui_DX12_IsInitialized())
		{
			// static_cast: WickedEngine compiled with RTTI disabled (/GR-)
			auto* dx12Device = static_cast<wi::graphics::GraphicsDevice_DX12*>(wi::graphics::GetDevice());
			if (dx12Device)
			{
				ID3D12GraphicsCommandList* nativeCmdList = dx12Device->GetDX12GraphicsCommandList(cmd);
				if (nativeCmdList)
				{
					extern void ImGui_DX12_RenderBridge(ID3D12GraphicsCommandList* cmdList);
					ImGui_DX12_RenderBridge(nativeCmdList);
				}
			}
		}
	}
}

void MasterRenderer::Render() const
{
	__super::Render();
}

// moved into function so we can call it at the right time from within renderpath3D, just before 2D is rendered
std::vector<int> g_StandaloneObjectHighlightList;
void MasterRenderer::RenderOutlineHighlighers(CommandList cmd) const
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	if (m_bRenderingVR == false)
	{
		// regular update
		// PE: Make sure highlight and other editor functions is not redered in test game.
		void WickedCall_RenderEditorFunctions(void);
		//PE: Moved to function so we can delete any already setup highlights.
		//if (!bImGuiInTestGame) WickedCall_RenderEditorFunctions();
		extern bool bImGuiRenderTargetFocus;
		WickedCall_SetRenderTargetMouseFocus(bImGuiRenderTargetFocus);
		WickedCall_RenderEditorFunctions();

		if (!bImGuiInTestGame)
		{
			// Selection outline:
			if (GetDepthStencil() != nullptr)
			{
				GraphicsDevice* device = wiGraphics::GetDevice();
				CommandList cmd = device->BeginCommandList();

				device->EventBegin("GGMax - Selection Outline Mask", cmd);

				Viewport vp;
				vp.width = (float)rt_Outline.GetDesc().width;
				vp.height = (float)rt_Outline.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				wiImageParams fx;
				fx.enableFullScreen();
				fx.stencilComp = STENCILMODE::STENCILMODE_EQUAL;
				fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

				// Objects outline:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}

				// Objects outline Red:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline_Red, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT_RED;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}

				// Objects outline Blue:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline_Blue, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT_BLUE;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}
				device->EventEnd(cmd);
			}
		}
		else
		{
			if (!bActivateStandaloneOutline) return;

			//PE: Add objects here for standalone highlights/Outline.
			if (g_StandaloneObjectHighlightList.size() > 0)
			{
				for (int i = 0; i < (int)g_StandaloneObjectHighlightList.size(); i++)
				{
					int obj = g_StandaloneObjectHighlightList[i];
					if (obj > 0)
					{
						if (ObjectExist(obj))
						{
							void* GetObjectsInternalData(int iID);
							sObject* pObject = (sObject*)GetObjectsInternalData(obj);
							if (pObject)
							{
								//WickedCall_SetObjectHighlight(pObject, false);
								void WickedCall_DrawObjctBox(sObject * pObject, XMFLOAT4 color, bool bThickLine, bool ForceBox);
								WickedCall_DrawObjctBox(pObject, XMFLOAT4(0.8f, 0.8f, 0.8f, 0.8f), false, false);
							}
						}
					}
				}
			}


			if (GetDepthStencil() != nullptr)
			{
				GraphicsDevice* device = wiGraphics::GetDevice();
				CommandList cmd = device->BeginCommandList();

				device->EventBegin("GGMax - Selection Outline Mask", cmd);

				Viewport vp;
				vp.width = (float)rt_Outline.GetDesc().width;
				vp.height = (float)rt_Outline.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				wiImageParams fx;
				fx.enableFullScreen();
				fx.stencilComp = STENCILMODE::STENCILMODE_EQUAL;
				fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

				// Objects outline:
				{
					//device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
					device->RenderPassBegin(&renderpass_Outline, cmd);
					// Draw solid blocks of selected objects
					fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
					wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
					device->RenderPassEnd(cmd);
				}
				device->EventEnd(cmd);
			}
		}
	}
}

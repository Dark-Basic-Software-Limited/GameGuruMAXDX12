// =============================================================================================
// GGMAX 3.114 - FAR TERRAIN. See GGTerrainFar.h for what this is and why.
//
// SHAPE OF THE THING (modelled on GGTerrainBake.cpp)
//   main thread   GGTerrainFar_Update()              - coverage mask + layer constants, every frame
//   render thread GGTerrainFar_DrawPrepass / _Draw   - walk the legacy clipmap's current chunks,
//                                                      CPU-skip the ones wholly inside the live ring,
//                                                      frustum cull, one strip draw each
//   No resources are created here except shaders and PSOs (at startup); the five material colour
//   maps are the live terrain's own (resource-manager cache hits, held so they survive a bake);
//   the geometry is the legacy clipmap's, the constants go through BindDynamicConstantBuffer
//   (an upload allocation - legal inside a render pass, unlike UpdateBuffer).
// =============================================================================================

#include "GGTerrainFar.h"
#include "GGTerrainBake.h"

#include "WickedEngine.h"
#include "wiRenderer.h"
#include "wiScene.h"
#include "wiTerrain.h"
#include "wiProfiler.h"

#include "GGTerrain.h"

#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <climits>

using namespace wi::graphics;
using namespace wi::primitive;
using namespace wi::scene;

// ---------------------------------------------------------------------------------------------
// public state
// ---------------------------------------------------------------------------------------------
bool  gg_terrain_far = true;
bool  gg_terrain_far_trees = true;
float gg_terrain_far_sink = 1.0f;
int   gg_terrain_far_debug = 0;
bool  gg_terrain_far_reflect = true;

extern bool gg_no_terrain;
extern bool gg_terrain_bake;
int GGTerrainBake_GetChunkCoords( int* outXZ, int maxPairs );   // GGTerrainBake.cpp
namespace GGTerrain
{
	void GGCustomFrame_Bind( wi::graphics::CommandList cmd );
	float GGCustomFrame_GetWaterHeight();                           // GGTerrain_part0.cpp
	bool GGTerrainWicked_IsTerrainHidden();                          // GGTerrainWicked.cpp
}

namespace
{
	// ★ Must match cbuffer GGTerrainFarCB in GGTerrainFarCommon.hlsli byte for byte.
	struct FarCB
	{
		uint32_t mask[ 128 ];          // 64x64 bits
		int32_t  originX, originZ;
		float    chunkSize;
		float    segBase;
		float    sinkMul;
		uint32_t flags;
		float    texScale;
		float    pad0;
		float    layerStart[ 8 ];
		float    layerRcp[ 8 ];
		float    slope[ 4 ];
		uint32_t slots[ 8 ];
	};
	static_assert( sizeof( FarCB ) == 656, "FarCB must match GGTerrainFarCB (GGTerrainFarCommon.hlsli)" );

	FarCB g_cb = {};
	bool  g_active = false;
	uint64_t g_updateFrame = ~0ull;
	float g_reach = 0.0f;
	float g_chunkSize = 5280.0f;       // last live chunk stride (the bake keeps the same grid)
	int   g_coveredCells = 0;
	int   g_coverageOutside = 0;      // live chunks that did not fit the 64x64 mask (should be 0)
	int   g_coverageDupes = 0;        // terrain objects on an already-covered cell
	int   g_coverageObjects = 0;      // terrain objects that counted as ground
	int   g_coverageDistCulled = 0;   // terrain objects past their Object Detail draw distance
	const char* g_maskSource = "none";
	const char* g_inactiveWhy = "not updated yet";

	Shader        g_vs, g_ps, g_prepassPS;
	PipelineState g_psoOpaque, g_psoPrepass;            // the main view
	PipelineState g_psoOpaqueRefl, g_psoPrepassRefl;    // the planar reflection: winding FLIPPED
	// PipelineStateDesc stores POINTERS and the DX12 backend compiles lazily at bind - persistent
	// homes for the state, as in GGTerrainBake.cpp / the tree PSOs (2.96).
	RasterizerState   g_rsStore[ 4 ];
	DepthStencilState g_dssStore[ 4 ];
	BlendState        g_bsStore[ 4 ];
	InputLayout       g_ilStore[ 4 ];
	bool g_shadersReady = false;
	int  g_shaderAttempts = 0;

	// The five material colour maps (base, low, layer 1, high, slope) - the SAME Color.dds files
	// the live terrain's layer materials load (GGTerrainWicked.cpp SetupTerrainMaterial), by the
	// same path and flags, so the resource manager hands back the very textures the ring is using.
	// Held here so they outlive the Wicked terrain under a Terrain Bake (its material entities die
	// with it). The legacy texColorArray is NOT an option: it only exists once the 1.3 GB legacy
	// page atlas has been created. Re-resolved every frame from the render params (per-level).
	const int FAR_MATS = 5;
	wi::Resource g_matRes[ FAR_MATS ];
	int g_matIndex[ FAR_MATS ] = { -1, -1, -1, -1, -1 };
	const Texture* g_matTex[ FAR_MATS ] = {};        // what the render thread binds, set in Update
	int g_matSrgb[ FAR_MATS ] = { -1, -1, -1, -1, -1 };
	wi::vector<wi::Resource> g_matRetired;           // the set before last swap: in-flight frames (08-05 rule)
	int g_matLoads = 0;
	std::string g_exeDir;

	// draw statistics, last frame (main pass)
	int g_statDrawn = 0, g_statCulled = 0, g_statSkipped = 0, g_statListed = 0;
	int g_statReflDrawn = 0, g_statReflCulled = 0, g_statReflUnder = 0;   // the planar reflection's colour pass
	float g_waterY = -1.0e9f;                        // main thread copy for the render thread
	uint32_t g_levels = 0;
	float g_segBase = 0.0f;
	char g_report[ 2048 ] = "";

	// ★ The frame's drawable chunks, snapshotted on the MAIN thread at the end of Update and only READ
	// by the draws. The four hooks (main prepass, reflection prepass, reflection colour, main colour)
	// record on separate jobsystem workers at once - a shared scratch they each resized and filled
	// was a data race (review 3.114).
	std::vector<GGTerrain::GGFarChunkRef> g_chunks;           // sized once at Init, never reallocated
	int g_chunkCount = 0;
	std::vector<int> g_coordScratch;                         // main thread only
	uint64_t g_requestFrame = ~0ull;                         // GGTerrainFar_RequestUpdate (the terrain block ran)
	int g_inactiveFrames = 0;

	wi::terrain::Terrain* FindTerrain()
	{
		Scene& scene = wi::scene::GetScene();
		if ( scene.terrains.GetCount() == 0 ) return nullptr;
		return &scene.terrains[ 0 ];
	}

	void InitShaders()
	{
		// ★ Called at STARTUP (GameGuruMain.cpp, beside GGTerrainBake_Init): the shader path is
		// relative and the CWD moves to Max/Files later, after which a load can never succeed. The
		// Update() retry is a fallback only, so it is capped low (each failure logs a line).
		if ( g_shadersReady || g_shaderAttempts >= 4 ) return;
		g_shaderAttempts++;
		wi::renderer::LoadShader( ShaderStage::VS, g_vs,        "GGTerrainFarVS.cso" );
		wi::renderer::LoadShader( ShaderStage::PS, g_ps,        "GGTerrainFarPS.cso" );
		wi::renderer::LoadShader( ShaderStage::PS, g_prepassPS, "GGTerrainFarPrepassPS.cso" );
		if ( !g_vs.IsValid() || !g_ps.IsValid() || !g_prepassPS.IsValid() )
		{
			if ( g_shaderAttempts == 4 )
				wi::backlog::post( "GGMAX 3.114: Far Terrain shaders never loaded - check GGTerrainFar*.cso in the shaders folder.", wi::backlog::LogLevel::Error );
			return;
		}
		GraphicsDevice* device = GetDevice();

		RasterizerState rs;
		rs.fill_mode = FillMode::SOLID;
		// ★ BACK, with the legacy clipmap's own winding (GGTerrain_part0.cpp rastState: front = clockwise).
		// NOT none: wherever the planar reflection's rays run under LAND (pixels the water never
		// samples) they meet the far field's UNDERSIDE - the live chunks cull theirs, so with NONE the far
		// field alone filled those regions with lit, fogged, 5-texture pixels: 0.42 ms of reflection
		// (interleaved FPS A/B) for a thin strip of visible horizon. The reflection camera FLIPS the
		// winding (with the main winding its far field vanished - measured), hence a second PSO pair,
		// as the legacy code kept its own counter-clockwise rastState2.
		rs.cull_mode = CullMode::BACK;
		rs.front_counter_clockwise = false;
		rs.depth_clip_enable = true;

		DepthStencilState dss;
		dss.depth_enable = true;
		dss.depth_write_mask = DepthWriteMask::ALL;
		dss.depth_func = ComparisonFunc::GREATER_EQUAL;   // reverse-Z
		dss.stencil_enable = false;

		BlendState bs;
		bs.render_target[0].blend_enable = false;
		bs.independent_blend_enable = false;
		bs.alpha_to_coverage_enable = false;

		// the legacy clipmap's own vertex format (GGTerrain_part0.cpp TerrainVertex, 20 bytes)
		InputLayout il;
		il.elements = {
			{ "POSITION", 0, Format::R32G32B32_FLOAT, 0, 0,  InputClassification::PER_VERTEX_DATA },
			{ "INORMAL",  0, Format::R8G8B8A8_UNORM,  0, 12, InputClassification::PER_VERTEX_DATA },
			{ "ID",       0, Format::R32_UINT,        0, 16, InputClassification::PER_VERTEX_DATA },
		};

		int slot = 0;
		auto CreatePSO = [&]( PipelineState* out, const Shader* ps, DepthWriteMask writeMask, bool flipWinding )
		{
			g_rsStore[slot]  = rs;
			g_rsStore[slot].front_counter_clockwise = flipWinding;
			g_dssStore[slot] = dss;
			g_dssStore[slot].depth_write_mask = writeMask;
			g_bsStore[slot]  = bs;
			g_ilStore[slot]  = il;
			PipelineStateDesc desc = {};
			desc.vs = &g_vs;                 // ★ SAME compiled VS in both PSOs
			desc.ps = ps;
			desc.il = &g_ilStore[slot];
			desc.pt = PrimitiveTopology::TRIANGLESTRIP;
			desc.rs = &g_rsStore[slot];
			desc.dss = &g_dssStore[slot];
			desc.bs = &g_bsStore[slot];
			device->CreatePipelineState( &desc, out );
			slot++;
		};
		// prepass: depth + RT1 custom depth, RT0 (visibility buffer) masked - the bake's 3.75/3.77 setup
		bs.independent_blend_enable = true;
		bs.render_target[0].render_target_write_mask = ColorWrite::DISABLE;
		bs.render_target[1] = bs.render_target[0];
		bs.render_target[1].render_target_write_mask = ColorWrite::ENABLE_ALL;
		CreatePSO( &g_psoPrepass,     &g_prepassPS, DepthWriteMask::ALL, false );
		CreatePSO( &g_psoPrepassRefl, &g_prepassPS, DepthWriteMask::ALL, true );
		bs.independent_blend_enable = false;
		bs.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		CreatePSO( &g_psoOpaque,      &g_ps, DepthWriteMask::ZERO, false );
		CreatePSO( &g_psoOpaqueRefl,  &g_ps, DepthWriteMask::ZERO, true );
		g_shadersReady = true;
	}

	void MarkCovered( int cx, int cz )
	{
		const int i = cx - g_cb.originX, j = cz - g_cb.originZ;
		if ( i < 0 || j < 0 || i >= 64 || j >= 64 ) { g_coverageOutside++; return; }
		const uint32_t bit = (uint32_t)( j * 64 + i );
		if ( ( g_cb.mask[ bit >> 5 ] >> ( bit & 31 ) ) & 1u ) { g_coverageDupes++; return; }   // two objects, one cell
		g_cb.mask[ bit >> 5 ] |= 1u << ( bit & 31 );
		g_coveredCells++;
	}

	bool CellCovered( int cx, int cz )
	{
		const int i = cx - g_cb.originX, j = cz - g_cb.originZ;
		if ( i < 0 || j < 0 || i >= 64 || j >= 64 ) return false;
		const uint32_t bit = (uint32_t)( j * 64 + i );
		return ( g_cb.mask[ bit >> 5 ] >> ( bit & 31 ) ) & 1u;
	}

	// The CPU twin of FarInterior over a whole chunk: true when every cell under the chunk's XZ box,
	// plus a one-cell margin, is a live chunk - nothing of it could survive the pixel discard.
	bool ChunkHidden( const AABB& box )
	{
		const float cs = g_cb.chunkSize;
		const int x0 = (int)std::floor( box._min.x / cs + 0.5f ) - 1;
		const int x1 = (int)std::floor( box._max.x / cs + 0.5f ) + 1;
		const int z0 = (int)std::floor( box._min.z / cs + 0.5f ) - 1;
		const int z1 = (int)std::floor( box._max.z / cs + 0.5f ) + 1;
		if ( x0 < g_cb.originX || z0 < g_cb.originZ || x1 >= g_cb.originX + 64 || z1 >= g_cb.originZ + 64 ) return false;
		for ( int z = z0; z <= z1; z++ )
			for ( int x = x0; x <= x1; x++ )
				if ( !CellCovered( x, z ) ) return false;
		return true;
	}

	bool Fresh()
	{
		return g_active && g_shadersReady && g_updateFrame == GetDevice()->GetFrameCount();
	}

	// Main thread. Returns false when any of the five maps is unusable (the far field then stays off
	// rather than drawing black ground).
	bool ResolveMaterials( const int mats[ FAR_MATS ] )
	{
		bool changed = false;
		for ( int i = 0; i < FAR_MATS; i++ ) if ( mats[i] != g_matIndex[i] || !g_matRes[i].IsValid() ) changed = true;
		if ( changed )
		{
			if ( g_exeDir.empty() )
			{
				// ★ exactly as GGTerrainWicked_Init builds wickedTerrainExeDir - INCLUDING popping the trailing
				// separator. The resource manager keys on the raw string; with the separator left on, every
				// "cache hit" was a second copy of each Color.dds in VRAM (review 3.114).
				g_exeDir = wi::helper::GetDirectoryFromPath( wi::helper::GetExecutablePath() );
				if ( !g_exeDir.empty() && ( g_exeDir.back() == '/' || g_exeDir.back() == '\\' ) ) g_exeDir.pop_back();
			}
			wi::vector<wi::Resource> outgoing;
			for ( int i = 0; i < FAR_MATS; i++ ) if ( g_matRes[i].IsValid() ) outgoing.push_back( g_matRes[i] );
			for ( int i = 0; i < FAR_MATS; i++ )
			{
				// ★ byte-identical to SetupTerrainMaterial's path and BASECOLORMAP flags (streaming
				// disabled), so this is a cache hit on the live terrain's own texture
				char path[ 512 ];
				snprintf( path, sizeof( path ), "%s/Files/terraintextures/mat%d/Color.dds", g_exeDir.c_str(), mats[i] + 1 );
				g_matRes[i] = wi::resourcemanager::Load( path, wi::resourcemanager::Flags::IMPORT_BLOCK_COMPRESSED );
				g_matIndex[i] = mats[i];
				g_matLoads++;
			}
			g_matRetired = std::move( outgoing );    // the set from TWO swaps ago is released here
		}
		bool ok = true;
		for ( int i = 0; i < FAR_MATS; i++ )
		{
			if ( g_matRes[i].IsValid() && g_matRes[i].GetTexture().IsValid() )
			{
				g_matTex[i] = &g_matRes[i].GetTexture();
				g_matSrgb[i] = g_matRes[i].GetTextureSRGBSubresource();
			}
			else
			{
				g_matTex[i] = nullptr; g_matSrgb[i] = -1; ok = false;
			}
		}
		return ok;
	}

	// Main thread. The far field is not drawing: after a couple of idle frames let go of the colour maps
	// (under a bake ours are the only references - a per-level value must not outlive its use). The
	// render jobs only touch them on a Fresh() frame, and Wicked defers the GPU free past frames in flight.
	void ReleaseMaterialsWhenIdle()
	{
		g_inactiveFrames++;
		if ( g_inactiveFrames == 3 )
		{
			g_matRetired.clear();
			for ( int i = 0; i < FAR_MATS; i++ )
			{
				if ( g_matRes[i].IsValid() ) g_matRetired.push_back( g_matRes[i] );
				g_matRes[i] = wi::Resource(); g_matIndex[i] = -1; g_matTex[i] = nullptr; g_matSrgb[i] = -1;
			}
		}
		else if ( g_inactiveFrames == 6 ) g_matRetired.clear();
	}

	void DrawPass( const Frustum* frustum, CommandList cmd, PipelineState* pso, bool reflection, int statSlot )   // stats: 0 none, 1 main, 2 reflection
	{
		if ( !Fresh() ) return;
		for ( int i = 0; i < FAR_MATS; i++ ) if ( g_matTex[i] == nullptr ) return;

		const int n = g_chunkCount;   // the main thread's snapshot - read only here
		if ( n <= 0 ) return;

		GraphicsDevice* device = GetDevice();
		device->BindPipelineState( pso, cmd );
		GGTerrain::GGCustomFrame_Bind( cmd );                 // b4: fog / water constants for ApplyFogCustom
		device->BindDynamicConstantBuffer( g_cb, 5, cmd );    // b5: GGTerrainFarCB
		for ( int i = 0; i < FAR_MATS; i++ )
			device->BindResource( g_matTex[i], 51 + i, cmd, g_matSrgb[i] );   // t51..t55, sRGB view = linear albedo

		int drawn = 0, culled = 0, under = 0;
		const uint32_t strides[] = { 20 };
		for ( int k = 0; k < n; k++ )
		{
			const GGTerrain::GGFarChunkRef& c = g_chunks[ k ];
			if ( frustum != nullptr && !frustum->CheckBoxFast( c.aabb ) ) { culled++; continue; }
			// the planar reflection clips everything below the water plane (g_xCamera_ClipPlane), so a
			// chunk wholly under it is pure vertex work - and the mirrored camera sees the far field from
			// BELOW, where no near hill hides it, so nothing else culls it (3x the main pass, measured)
			if ( reflection && c.aabb._max.y <= g_waterY ) { under++; continue; }
			const GPUBuffer* vbs[] = { c.vb };
			device->BindVertexBuffers( vbs, 0, 1, strides, nullptr, cmd );
			device->BindIndexBuffer( c.ib, IndexBufferFormat::UINT16, 0, cmd );
			device->DrawIndexed( c.numIndices, 0, 0, cmd );
			drawn++;
		}
		if ( statSlot == 1 ) { g_statDrawn = drawn; g_statCulled = culled; }
		if ( statSlot == 2 ) { g_statReflDrawn = drawn; g_statReflCulled = culled; g_statReflUnder = under; }
	}
}

// ---------------------------------------------------------------------------------------------
// main thread
// ---------------------------------------------------------------------------------------------
void GGTerrainFar_Init()
{
	InitShaders();
	g_chunks.resize( 1024 );   // 9 levels x 64 = 576 at most; never resized again, so no render job sees a move
}

void GGTerrainFar_RequestUpdate()
{
	g_requestFrame = GetDevice()->GetFrameCount();
}

static bool GGTerrainFar_UpdateBody();

void GGTerrainFar_Update()
{
	g_active = false;
	g_chunkCount = 0;
	g_updateFrame = GetDevice()->GetFrameCount();
	g_reach = 0;
	if ( !GGTerrainFar_UpdateBody() ) ReleaseMaterialsWhenIdle();
	else g_inactiveFrames = 0;
}

// ★ Runs AFTER Scene::Update (master_part1.cpp, right after __super::Update), so the mask is built
// from the chunk set that is actually rendered this frame - Scene::Update's own Generation_Update
// merges and removes chunks (review 3.114). Returns true when the far field will draw.
static bool GGTerrainFar_UpdateBody()
{
	if ( g_requestFrame != g_updateFrame ) { g_inactiveWhy = "the terrain block did not run this frame (empty level / standalone saver)"; return false; }
	if ( !gg_terrain_far ) { g_inactiveWhy = "switched off (terrainfar / SET_TERRAINFAR 0)"; return false; }
	InitShaders();
	if ( !g_shadersReady ) { g_inactiveWhy = "shaders not loaded"; return false; }
	if ( GGTerrain::GGTerrainWicked_IsTerrainHidden() ) { g_inactiveWhy = "terrain hidden (View Options)"; return false; }

	// ---- the coverage mask: the chunks that ACTUALLY exist ----
	g_coordScratch.clear();
	wi::terrain::Terrain* terrain = FindTerrain();
	Scene& scene = wi::scene::GetScene();
	if ( terrain != nullptr && !gg_no_terrain )
	{
		g_chunkSize = (float)( wi::terrain::chunk_width - 1 ) * terrain->chunk_scale;
		// ★ From the MAIN scene's terrain objects, NOT terrain->chunks: the generator job inserts into that
		// map on a worker with no lock while we would be walking it (a robin-hood insert moves entries, a
		// rehash frees them). Main-scene objects only change on this thread - the generator builds into
		// its own scene and Scene::Update merges. Only wiTerrain sets FILTER_TERRAIN; a chunk sits at
		// c * chunkSize (wiTerrain.cpp chunk_data.position).
		const XMFLOAT3 eye = wi::scene::GetCamera().Eye;
		g_coverageDistCulled = 0;
		for ( size_t i = 0; i < scene.objects.GetCount(); i++ )
		{
			const ObjectComponent& obj = scene.objects[ i ];
			if ( ( obj.filterMask & wi::enums::FILTER_TERRAIN ) == 0 ) continue;
			if ( !obj.IsRenderable() ) continue;                       // a hidden chunk is a hole, not ground
			if ( scene.meshes.GetComponent( obj.meshID ) == nullptr ) continue;
			const TransformComponent* tr = scene.transforms.GetComponent( scene.objects.GetEntity( i ) );
			if ( tr == nullptr ) continue;
			const XMFLOAT3 pos = tr->GetPosition();
			// Object Detail Distance caps terrain chunks too (GGTerrainWicked.cpp), and the engine then dithers
			// them out past draw_distance: a chunk beyond its draw distance is NOT ground this frame - leave its
			// cell to the far field (it used to be sky) rather than discarding the far field over nothing.
			if ( obj.draw_distance < 1.0e30f )
			{
				const float dx = pos.x - eye.x, dz = pos.z - eye.z;
				if ( dx * dx + dz * dz > obj.draw_distance * obj.draw_distance ) { g_coverageDistCulled++; continue; }
			}
			g_coordScratch.push_back( (int)std::floor( pos.x / g_chunkSize + 0.5f ) );
			g_coordScratch.push_back( (int)std::floor( pos.z / g_chunkSize + 0.5f ) );
		}
		g_maskSource = "live chunks";
	}
	else if ( gg_no_terrain && gg_terrain_bake && GGTerrainBake_Ready() )
	{
		// a Terrain Bake: the baked chunk set, which does NOT follow the camera
		g_coordScratch.resize( 2 * 4096 );
		const int n = GGTerrainBake_GetChunkCoords( g_coordScratch.data(), 4096 );
		g_coordScratch.resize( 2 * (size_t)std::max( 0, n ) );
		g_maskSource = "baked chunks";
	}
	else
	{
		g_inactiveWhy = gg_no_terrain ? "terrain off without a bake (no ground to continue)" : "no terrain";
		g_maskSource = "none";
		return false;
	}
	if ( g_coordScratch.empty() ) { g_inactiveWhy = "no live chunks yet"; return false; }

	std::memset( g_cb.mask, 0, sizeof( g_cb.mask ) );
	int minX = INT_MAX, maxX = INT_MIN, minZ = INT_MAX, maxZ = INT_MIN;
	for ( size_t i = 0; i < g_coordScratch.size(); i += 2 )
	{
		minX = std::min( minX, g_coordScratch[i] ); maxX = std::max( maxX, g_coordScratch[i] );
		minZ = std::min( minZ, g_coordScratch[i + 1] ); maxZ = std::max( maxZ, g_coordScratch[i + 1] );
	}
	// centre of the covered set, not the camera - the baked set stays where it was baked
	g_cb.originX = ( minX + maxX ) / 2 - 32;
	g_cb.originZ = ( minZ + maxZ ) / 2 - 32;
	g_coveredCells = 0; g_coverageOutside = 0; g_coverageDupes = 0;
	g_coverageObjects = (int)( g_coordScratch.size() / 2 );
	for ( size_t i = 0; i < g_coordScratch.size(); i += 2 ) MarkCovered( g_coordScratch[i], g_coordScratch[i + 1] );

	// ---- the layer rule: GG's render params, read every frame (a per-level value - never cache it) ----
	const auto& rp = GGTerrain::ggterrain_global_render_params;
	for ( int i = 0; i < 8; i++ ) { g_cb.layerStart[i] = 0; g_cb.layerRcp[i] = 0; }
	for ( int i = 0; i < 5; i++ )
	{
		const float w = rp.layerEndHeight[i] - rp.layerStartHeight[i];
		g_cb.layerStart[i] = rp.layerStartHeight[i];
		g_cb.layerRcp[i] = ( w > 0.001f ) ? 1.0f / w : 0.0f;
	}
	for ( int i = 0; i < 2; i++ )
	{
		const float w = rp.slopeEnd[i] - rp.slopeStart[i];
		g_cb.slope[i * 2 + 0] = rp.slopeStart[i];
		g_cb.slope[i * 2 + 1] = ( w > 0.001f ) ? 1.0f / w : 0.0f;
	}
	// the live terrain's 5-slot mapping (GGTerrainWicked.cpp SetupWickedTerrainMaterials / FillChunkBlendmapGG)
	for ( int i = 0; i < 8; i++ ) g_cb.slots[i] = 0;
	g_cb.slots[0] = (uint32_t)( rp.baseLayerMaterial & 0xFF );
	g_cb.slots[1] = (uint32_t)( rp.layerMatIndex[0] & 0xFF );   // low
	g_cb.slots[2] = (uint32_t)( rp.layerMatIndex[1] & 0xFF );   // layer 1
	g_cb.slots[3] = (uint32_t)( rp.layerMatIndex[2] & 0xFF );   // high (layers 2..4)
	g_cb.slots[4] = (uint32_t)( rp.slopeMatIndex[0] & 0xFF );   // slope (both slopes)
	for ( int i = 0; i < 5; i++ ) if ( g_cb.slots[i] >= 32 ) g_cb.slots[i] = 0;
	{
		int mats[ FAR_MATS ];
		for ( int i = 0; i < FAR_MATS; i++ ) mats[i] = (int)g_cb.slots[i];
		if ( !ResolveMaterials( mats ) ) { g_inactiveWhy = "material colour maps not loaded"; return false; }
	}

	float reach = 0, segBase = 0; uint32_t levels = 0;
	GGTerrain::GGTerrain_GetFarDrawChunks( nullptr, 0, &reach, &segBase, &levels );
	if ( levels == 0 || segBase <= 0 ) { g_inactiveWhy = "legacy clipmap not valid"; return false; }

	g_waterY = GGTerrain::GGCustomFrame_GetWaterHeight();
	g_cb.chunkSize = g_chunkSize;
	g_cb.segBase = segBase;
	g_cb.sinkMul = gg_terrain_far_sink;
	g_cb.flags = ( (uint32_t)( gg_terrain_far_debug & 0xF ) ) << 8;
	g_cb.texScale = 32.0f / g_chunkSize;   // the live terrain's 32 repeats per chunk (gg_tile_share cap)
	g_reach = reach;
	g_levels = levels;
	g_segBase = segBase;

	// ---- the frame's chunk list: what the legacy draw would draw, minus whatever lies wholly inside the
	// ring (ChunkHidden). Built here, read by the render jobs. ----
	if ( g_chunks.size() < 1024 ) g_chunks.resize( 1024 );   // Init normally did this
	const int listed = GGTerrain::GGTerrain_GetFarDrawChunks( g_chunks.data(), (int)g_chunks.size(), &reach, &segBase, &levels );
	int kept = 0;
	for ( int k = 0; k < listed; k++ )
	{
		if ( ChunkHidden( g_chunks[ k ].aabb ) ) continue;
		g_chunks[ kept++ ] = g_chunks[ k ];
	}
	g_statListed = listed;
	g_statSkipped = listed - kept;
	g_chunkCount = kept;
	g_active = true;
	g_inactiveWhy = "";
	return true;
}

// setup.ini fogdx11curve (Common_part1.cpp has no engine headers) - the engine flag behind OPTION_BIT_GG_DX11_FOG_CURVE
void GGTerrainFar_SetDX11FogCurve( bool on )
{
	wi::renderer::gg_dx11_fog_curve = on;
}

float GGTerrainFar_GetReach()
{
	return ( g_active && gg_terrain_far ) ? g_reach : 0.0f;
}

// ---------------------------------------------------------------------------------------------
// render thread
// ---------------------------------------------------------------------------------------------
void GGTerrainFar_DrawPrepass( const Frustum* frustum, CommandList cmd, bool reflection )
{
	if ( !Fresh() ) return;
	if ( reflection && !gg_terrain_far_reflect ) return;
	auto range = wi::profiler::BeginRangeGPU( reflection ? "GGTerrainFar Prepass (Reflection)" : "GGTerrainFar Prepass", cmd );
	GetDevice()->EventBegin( "GGTerrainFar Prepass", cmd );
	DrawPass( frustum, cmd, reflection ? &g_psoPrepassRefl : &g_psoPrepass, reflection, 0 );
	GetDevice()->EventEnd( cmd );
	wi::profiler::EndRange( range );
}

void GGTerrainFar_Draw( const Frustum* frustum, int mode, CommandList cmd )
{
	if ( !Fresh() ) return;
	if ( mode != 0 && !gg_terrain_far_reflect ) return;
	auto range = wi::profiler::BeginRangeGPU( mode == 0 ? "GGTerrainFar Draw" : "GGTerrainFar Draw (Reflection)", cmd );
	GetDevice()->EventBegin( "GGTerrainFar Draw", cmd );
	DrawPass( frustum, cmd, mode != 0 ? &g_psoOpaqueRefl : &g_psoOpaque, mode != 0, mode == 0 ? 1 : 2 );
	GetDevice()->EventEnd( cmd );
	wi::profiler::EndRange( range );
}

// ---------------------------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------------------------
const char* GGTerrainFar_Report()
{
	int matsOk = 0;
	for ( int i = 0; i < FAR_MATS; i++ ) if ( g_matTex[i] != nullptr ) matsOk++;
	snprintf( g_report, sizeof( g_report ),
		"FAR TERRAIN (DX11 horizon)\n"
		"  switch            : %d   trees follow: %d   sink x%.2f   debug %d\n"
		"  active            : %d   %s\n"
		"  shaders           : %s after %d attempts\n"
		"  materials         : %d/5 colour maps (GG mat %d %d %d %d %d = base low layer1 high slope), %d loads\n"
		"  mask              : %s, %d cells covered from %d objects (%d on a shared cell, %d outside the 64x64 mask, %d past Object Detail distance), origin %d,%d, chunk %.0f\n"
		"  clipmap           : %u levels, LOD0 spacing %.1f, outermost half-width %.0f (reach)\n"
		"  last frame        : %d chunks listed, %d drawn, %d frustum-culled, %d hidden inside the ring\n"
		"  reflection        : %s, %d drawn, %d frustum-culled, %d wholly under the water plane (y %.0f)\n",
		gg_terrain_far ? 1 : 0, gg_terrain_far_trees ? 1 : 0, gg_terrain_far_sink, gg_terrain_far_debug,
		g_active ? 1 : 0, g_active ? "" : g_inactiveWhy,
		g_shadersReady ? "ok" : "NOT LOADED", g_shaderAttempts,
		matsOk, g_matIndex[0] + 1, g_matIndex[1] + 1, g_matIndex[2] + 1, g_matIndex[3] + 1, g_matIndex[4] + 1, g_matLoads,
		g_maskSource, g_coveredCells, g_coverageObjects, g_coverageDupes, g_coverageOutside, g_coverageDistCulled, g_cb.originX, g_cb.originZ, g_cb.chunkSize,
		g_levels, g_segBase, g_reach,
		g_statListed, g_statDrawn, g_statCulled, g_statSkipped,
		gg_terrain_far_reflect ? "on" : "OFF", g_statReflDrawn, g_statReflCulled, g_statReflUnder, g_waterY );
	g_report[ sizeof( g_report ) - 1 ] = 0;
	return g_report;
}

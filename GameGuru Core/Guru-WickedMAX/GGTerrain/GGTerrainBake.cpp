// =============================================================================================
// GGMAX 3.25 - TERRAIN BAKE. See GGTerrainBake.h for what this is and why.
//
// SHAPE OF THE THING
//   main thread   GGTerrainBake_Update()          - state machine; builds meshes and CREATES the
//                                                   output textures, then arms a dispatch
//   render thread GGTerrainBake_RecordPendingBakes() - RECORDS the compute dispatches only
//   render thread GGTerrainBake_DrawPrepass/_Draw    - per-chunk frustum cull + one draw call
//
//   The split between "create" and "record" is not stylistic. Creating textures and issuing a
//   copy from inside a customDraw_* callback - which runs on a job thread - access-violated
//   twice inside DescriptorBinder::flush on the far-tree work. Resource creation belongs on the
//   main thread; a render callback may only record into the command list it was handed, and the
//   ready flag is set at the END so a half-built set can never be drawn.
// =============================================================================================

#include "GGTerrainBake.h"

#include "WickedEngine.h"
#include "wiRenderer.h"
#include "wiScene.h"
#include "wiTerrain.h"
#include "wiProfiler.h"

#include "master.h"
#include "preprocessor-moreflags.h"
#include "gameguru.h"

#include "GGTerrain.h"
#include "GGTerrainWicked.h"

#include <vector>
#include <cstdio>

using namespace wi::graphics;
using namespace wi::primitive;
using namespace wi::scene;

// ---------------------------------------------------------------------------------------------
// public state
// ---------------------------------------------------------------------------------------------
bool gg_terrain_bake = false;
int  gg_terrain_bake_res = 256;
int  gg_terrain_bake_chunks = 0;
int  gg_terrain_bake_textures = 0;
int  gg_terrain_bake_drawcalls = 0;
int  gg_terrain_bake_culled = 0;
int  gg_terrain_bake_vram_kb = 0;

// the 2.94 Terrain Off teardown, which this switch reuses verbatim once the bake is ready
extern bool gg_no_terrain;
extern void GGSetNoTerrainLevel( int );
namespace GGTerrain { void GGCustomFrame_Bind( wi::graphics::CommandList cmd ); }

namespace
{
	// One vertex is 24 bytes. Normals are packed to R8G8B8A8_UNORM rather than kept as three
	// floats: a chunk is 67x67 = 4489 vertices, so the packing is worth 36 KB per chunk and a
	// terrain of a few hundred chunks is the whole reason this mode exists.
	struct BakeVertex
	{
		XMFLOAT3 pos;      // WORLD space - see the note in GGTerrainBakeVS.hlsl
		uint32_t normal;   // R8G8B8A8_UNORM
		XMFLOAT2 uv;       // chunk UV, indexes the baked texture directly
	};

	struct BakedChunk
	{
		GPUBuffer vb;
		Texture   tex;
		AABB      aabb;
		bool      dispatch_pending = false;
		bool      tex_ready = false;
		// the terrain chunk coordinate this came from, so a rebuild can be matched up
		int cx = 0, cz = 0;
	};

	std::vector<BakedChunk> g_chunks;
	GPUBuffer g_indexBuffer;          // shared: every chunk has identical grid topology
	uint32_t  g_indexCount = 0;

	Shader        g_vs, g_ps, g_prepassPS;
	PipelineState g_psoOpaque, g_psoPrepass;
	// ★ Wicked's DX12 backend compiles a PSO LAZILY at BIND time and PipelineStateDesc stores
	// POINTERS, so a PSO built from stack locals compiles from destroyed memory. These are the
	// persistent homes for the state each PSO was created with. Bitten on the tree PSOs (2.96).
	RasterizerState g_rsStore[2];
	DepthStencilState g_dssStore[2];
	BlendState g_bsStore[2];
	InputLayout g_ilStore[2];

	bool g_initialised = false;
	bool g_shadersReady = false;
	bool g_ready = false;            // meshes AND textures done; terrain may be torn down
	bool g_buildRequested = false;
	bool g_anyDispatchPending = false;
	char g_report[2048] = "";

	// GGTerrainWicked.cpp's GetWickedTerrain() is static to that translation unit, and the entity
	// handle it uses is a file-scope global there. Taking the first (and only) TerrainComponent in
	// the scene reaches the same object without widening anybody else's interface.
	wi::terrain::Terrain* FindTerrain()
	{
		Scene& scene = wi::scene::GetScene();
		if ( scene.terrains.GetCount() == 0 ) return nullptr;
		return &scene.terrains[0];
	}

	uint32_t PackNormal( const XMFLOAT3& n )
	{
		const uint32_t r = (uint32_t)std::max( 0.0f, std::min( 255.0f, (n.x * 0.5f + 0.5f) * 255.0f ) );
		const uint32_t g = (uint32_t)std::max( 0.0f, std::min( 255.0f, (n.y * 0.5f + 0.5f) * 255.0f ) );
		const uint32_t b = (uint32_t)std::max( 0.0f, std::min( 255.0f, (n.z * 0.5f + 0.5f) * 255.0f ) );
		return r | (g << 8) | (b << 16) | (255u << 24);
	}

	// Build the shared index buffer once: a plain (W-1)x(W-1) quad grid over the chunk's regular
	// vertex grid. This deliberately does NOT reuse Wicked's per-LOD index sets. Those exist to
	// stitch neighbouring chunks at different LODs, which needs the whole LOD selection system
	// that this mode is removing; a single full-detail grid keeps the baked ground exactly the
	// shape the player walks on (physics still reads the untouched CPU heightmap) and cannot
	// produce a seam or a hole between two chunks that disagree about their LOD.
	bool BuildSharedIndexBuffer()
	{
		if ( g_indexBuffer.IsValid() ) return true;
		const int W = wi::terrain::chunk_width;
		std::vector<uint16_t> indices;
		indices.reserve( (size_t)(W - 1) * (W - 1) * 6 );
		for ( int z = 0; z < W - 1; z++ )
		{
			for ( int x = 0; x < W - 1; x++ )
			{
				const uint16_t lowerLeft  = (uint16_t)( x + z * W );
				const uint16_t lowerRight = (uint16_t)( (x + 1) + z * W );
				const uint16_t topLeft    = (uint16_t)( x + (z + 1) * W );
				const uint16_t topRight   = (uint16_t)( (x + 1) + (z + 1) * W );
				indices.push_back( lowerLeft );  indices.push_back( topLeft );   indices.push_back( lowerRight );
				indices.push_back( topLeft );    indices.push_back( topRight );  indices.push_back( lowerRight );
			}
		}
		g_indexCount = (uint32_t)indices.size();
		GPUBufferDesc desc;
		desc.size = indices.size() * sizeof( uint16_t );
		desc.bind_flags = BindFlag::INDEX_BUFFER;
		desc.usage = Usage::DEFAULT;
		GraphicsDevice* device = GetDevice();
		if ( !device->CreateBuffer( &desc, indices.data(), &g_indexBuffer ) ) return false;
		device->SetName( &g_indexBuffer, "GGTerrainBake::indexBuffer" );
		return true;
	}
}

// ---------------------------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------------------------
void GGTerrainBake_Init()
{
	if ( g_initialised ) return;
	g_initialised = true;

	wi::renderer::LoadShader( ShaderStage::VS, g_vs,        "GGTerrainBakeVS.cso" );
	wi::renderer::LoadShader( ShaderStage::PS, g_ps,        "GGTerrainBakePS.cso" );
	wi::renderer::LoadShader( ShaderStage::PS, g_prepassPS, "GGTerrainBakePrepassPS.cso" );
	if ( !g_vs.IsValid() || !g_ps.IsValid() || !g_prepassPS.IsValid() )
	{
		wi::backlog::post( "GGMAX 3.25: Terrain Bake shaders failed to load - the switch will do nothing.", wi::backlog::LogLevel::Error );
		return;
	}

	GraphicsDevice* device = GetDevice();

	RasterizerState rs;
	rs.fill_mode = FillMode::SOLID;
	rs.cull_mode = CullMode::BACK;
	rs.front_counter_clockwise = true;
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

	InputLayout il;
	il.elements = {
		{ "POSITION", 0, Format::R32G32B32_FLOAT, 0, 0,  InputClassification::PER_VERTEX_DATA },
		{ "NORMAL",   0, Format::R8G8B8A8_UNORM,  0, 12, InputClassification::PER_VERTEX_DATA },
		{ "TEXCOORD", 0, Format::R32G32_FLOAT,    0, 16, InputClassification::PER_VERTEX_DATA },
	};

	int slot = 0;
	auto CreatePSO = [&]( PipelineState* out, const Shader* ps, DepthWriteMask writeMask, ComparisonFunc func )
	{
		g_rsStore[slot]  = rs;
		g_dssStore[slot] = dss;
		g_dssStore[slot].depth_write_mask = writeMask;
		g_dssStore[slot].depth_func = func;
		g_bsStore[slot]  = bs;
		g_ilStore[slot]  = il;
		PipelineStateDesc desc = {};
		desc.vs = &g_vs;                 // ★ SAME compiled VS in both PSOs - see the shader header
		desc.ps = ps;
		desc.il = &g_ilStore[slot];
		desc.pt = PrimitiveTopology::TRIANGLELIST;
		desc.rs = &g_rsStore[slot];
		desc.dss = &g_dssStore[slot];
		desc.bs = &g_bsStore[slot];
		device->CreatePipelineState( &desc, out );
		slot++;
	};
	// prepass lays the depth down; colour tests EQUAL against it and writes no depth, exactly
	// like the far-tree billboard pair.
	CreatePSO( &g_psoPrepass, &g_prepassPS, DepthWriteMask::ALL,  ComparisonFunc::GREATER_EQUAL );
	CreatePSO( &g_psoOpaque,  &g_ps,        DepthWriteMask::ZERO, ComparisonFunc::EQUAL );

	g_shadersReady = true;
}

// ---------------------------------------------------------------------------------------------
// teardown
// ---------------------------------------------------------------------------------------------
void GGTerrainBake_Clear()
{
	g_chunks.clear();
	g_ready = false;
	g_buildRequested = false;
	g_anyDispatchPending = false;
	gg_terrain_bake_chunks = 0;
	gg_terrain_bake_textures = 0;
	gg_terrain_bake_vram_kb = 0;
	gg_terrain_bake_drawcalls = 0;
	gg_terrain_bake_culled = 0;
}

bool GGTerrainBake_Ready()
{
	return g_ready;
}

// ---------------------------------------------------------------------------------------------
// main thread: build the meshes and create the textures
// ---------------------------------------------------------------------------------------------
namespace
{
	bool BuildFromTerrain()
	{
		wi::terrain::Terrain* terrain = FindTerrain();
		if ( terrain == nullptr ) return false;
		if ( terrain->chunks.empty() ) return false;
		if ( !BuildSharedIndexBuffer() ) return false;

		GraphicsDevice* device = GetDevice();
		Scene& scene = wi::scene::GetScene();
		const int W = wi::terrain::chunk_width;
		const uint32_t res = (uint32_t)std::max( 32, std::min( 2048, gg_terrain_bake_res ) );

		g_chunks.clear();
		g_chunks.reserve( terrain->chunks.size() );

		size_t vram = 0;
		std::vector<BakeVertex> verts;
		verts.resize( (size_t)W * W );

		for ( auto& pair : terrain->chunks )
		{
			const wi::terrain::Chunk& chunk = pair.first;
			wi::terrain::ChunkData& chunk_data = pair.second;
			if ( chunk_data.entity == wi::ecs::INVALID_ENTITY ) continue;
			// A chunk mid-regeneration holds the STALE pre-regeneration mesh in the main scene;
			// baking it would freeze the old shape into the replacement. Skip and let the next
			// bake pick it up - the switch can always be re-ticked.
			if ( chunk_data.merge_pending ) continue;

			ObjectComponent* object = scene.objects.GetComponent( chunk_data.entity );
			if ( object == nullptr ) continue;
			MeshComponent* mesh = scene.meshes.GetComponent( object->meshID );
			if ( mesh == nullptr ) continue;
			if ( mesh->vertex_positions.size() != (size_t)W * W ) continue;
			if ( mesh->vertex_normals.size()   != (size_t)W * W ) continue;
			if ( mesh->vertex_uvset_0.size()   != (size_t)W * W ) continue;

			// Chunk mesh vertices are chunk-LOCAL; fold the chunk world offset in here so the
			// draw path needs no per-chunk constant buffer at all.
			const XMFLOAT3 origin = chunk_data.position;
			XMFLOAT3 bmin( FLT_MAX, FLT_MAX, FLT_MAX ), bmax( -FLT_MAX, -FLT_MAX, -FLT_MAX );
			for ( size_t i = 0; i < verts.size(); i++ )
			{
				const XMFLOAT3& p = mesh->vertex_positions[i];
				BakeVertex& v = verts[i];
				v.pos = XMFLOAT3( p.x + origin.x, p.y + origin.y, p.z + origin.z );
				v.normal = PackNormal( mesh->vertex_normals[i] );
				v.uv = mesh->vertex_uvset_0[i];
				bmin.x = std::min( bmin.x, v.pos.x ); bmax.x = std::max( bmax.x, v.pos.x );
				bmin.y = std::min( bmin.y, v.pos.y ); bmax.y = std::max( bmax.y, v.pos.y );
				bmin.z = std::min( bmin.z, v.pos.z ); bmax.z = std::max( bmax.z, v.pos.z );
			}

			g_chunks.emplace_back();
			BakedChunk& out = g_chunks.back();
			out.cx = chunk.x; out.cz = chunk.z;
			out.aabb = AABB( bmin, bmax );

			GPUBufferDesc vbdesc;
			vbdesc.size = verts.size() * sizeof( BakeVertex );
			vbdesc.bind_flags = BindFlag::VERTEX_BUFFER;
			vbdesc.usage = Usage::DEFAULT;
			if ( !device->CreateBuffer( &vbdesc, verts.data(), &out.vb ) )
			{
				g_chunks.pop_back();
				continue;
			}
			device->SetName( &out.vb, "GGTerrainBake::chunkVB" );
			vram += vbdesc.size;

			// Create the texture HERE, on the main thread. The dispatch that fills it is
			// recorded later, on the render thread, into the frame's command list.
			if ( chunk_data.blendmap.IsValid() )
			{
				TextureDesc tdesc;
				tdesc.width = res;
				tdesc.height = res;
				tdesc.mip_levels = 1;
				tdesc.format = Format::R8G8B8A8_UNORM;
				tdesc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				tdesc.layout = ResourceState::SHADER_RESOURCE;
				if ( device->CreateTexture( &tdesc, nullptr, &out.tex ) )
				{
					device->SetName( &out.tex, "GGTerrainBake::chunkTex" );
					out.dispatch_pending = true;
					vram += (size_t)res * res * 4;
				}
			}
		}

		if ( g_chunks.empty() ) return false;

		g_anyDispatchPending = true;
		gg_terrain_bake_chunks = (int)g_chunks.size();
		gg_terrain_bake_vram_kb = (int)( vram / 1024 );
		return true;
	}
}

void GGTerrainBake_Update()
{
	if ( !g_initialised ) GGTerrainBake_Init();

	static bool s_prevSwitch = false;

	if ( gg_terrain_bake && !s_prevSwitch )
	{
		// ON edge: build now, from the terrain exactly as it currently stands.
		g_buildRequested = true;
	}
	if ( !gg_terrain_bake && s_prevSwitch )
	{
		// OFF edge: put the real terrain back FIRST, then drop the baked copy. Doing it in this
		// order means there is never a frame with neither - the visible failure mode would be a
		// one-frame hole in the world, which reads to a user as a crash.
		GGSetNoTerrainLevel( 0 );
		GGTerrainBake_Clear();
	}
	s_prevSwitch = gg_terrain_bake;

	if ( !gg_terrain_bake ) return;
	if ( !g_shadersReady ) return;

	if ( g_buildRequested )
	{
		g_buildRequested = false;
		if ( !BuildFromTerrain() )
		{
			// Nothing to bake (no terrain in the scene, or it has not generated yet). Leave the
			// real terrain alone rather than tearing it down for an empty replacement.
			g_ready = false;
			return;
		}
	}

	// Once every chunk has its object equivalent AND its texture has been filled, and not one
	// moment before, reuse the 2.94 Terrain Off teardown to remove the Wicked terrain.
	if ( !g_ready && !g_chunks.empty() && !g_anyDispatchPending )
	{
		int withTex = 0;
		for ( const BakedChunk& c : g_chunks ) if ( c.tex_ready ) withTex++;
		gg_terrain_bake_textures = withTex;
		g_ready = true;
		GGSetNoTerrainLevel( 1 );
	}
}

// ---------------------------------------------------------------------------------------------
// render thread: record the pending compute bakes
// ---------------------------------------------------------------------------------------------
void GGTerrainBake_RecordPendingBakes( CommandList cmd )
{
	if ( !g_anyDispatchPending ) return;
	wi::terrain::Terrain* terrain = FindTerrain();
	if ( terrain == nullptr ) { g_anyDispatchPending = false; return; }

	const uint32_t res = (uint32_t)std::max( 32, std::min( 2048, gg_terrain_bake_res ) );
	GetDevice()->EventBegin( "GGTerrainBake Bake", cmd );
	for ( BakedChunk& c : g_chunks )
	{
		if ( !c.dispatch_pending ) continue;
		c.dispatch_pending = false;
		auto it = terrain->chunks.find( wi::terrain::Chunk{ c.cx, c.cz } );
		if ( it == terrain->chunks.end() ) continue;
		if ( terrain->gg_BakeChunkBasecolor( it->second, c.tex, res, cmd ) )
			c.tex_ready = true;
	}
	GetDevice()->EventEnd( cmd );
	g_anyDispatchPending = false;
}

// ---------------------------------------------------------------------------------------------
// render thread: draw
// ---------------------------------------------------------------------------------------------
namespace
{
	void DrawInternal( const Frustum* frustum, CommandList cmd, PipelineState* pso, bool countStats )
	{
		if ( !g_ready || g_chunks.empty() ) return;
		GraphicsDevice* device = GetDevice();
		device->BindPipelineState( pso, cmd );
		GGTerrain::GGCustomFrame_Bind( cmd );
		device->BindIndexBuffer( &g_indexBuffer, IndexBufferFormat::UINT16, 0, cmd );

		int calls = 0, culled = 0;
		const uint32_t strides[] = { sizeof( BakeVertex ) };
		for ( BakedChunk& c : g_chunks )
		{
			if ( !c.vb.IsValid() || !c.tex_ready ) { continue; }
			if ( frustum != nullptr && !frustum->CheckBoxFast( c.aabb ) ) { culled++; continue; }
			device->BindResource( &c.tex, 50, cmd );
			const GPUBuffer* vbs[] = { &c.vb };
			device->BindVertexBuffers( vbs, 0, 1, strides, 0, cmd );
			device->DrawIndexed( g_indexCount, 0, 0, cmd );
			calls++;
		}
		if ( countStats ) { gg_terrain_bake_drawcalls = calls; gg_terrain_bake_culled = culled; }
	}
}

void GGTerrainBake_DrawPrepass( const Frustum* frustum, CommandList cmd )
{
	if ( !gg_terrain_bake || !g_shadersReady ) return;
	GetDevice()->EventBegin( "GGTerrainBake Prepass", cmd );
	DrawInternal( frustum, cmd, &g_psoPrepass, false );
	GetDevice()->EventEnd( cmd );
}

void GGTerrainBake_Draw( const Frustum* frustum, CommandList cmd )
{
	if ( !gg_terrain_bake || !g_shadersReady ) return;
	GetDevice()->EventBegin( "GGTerrainBake Draw", cmd );
	DrawInternal( frustum, cmd, &g_psoOpaque, true );
	GetDevice()->EventEnd( cmd );
}

// ---------------------------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------------------------
const char* GGTerrainBake_Report()
{
	const uint32_t res = (uint32_t)std::max( 32, std::min( 2048, gg_terrain_bake_res ) );
	int withTex = 0;
	for ( const BakedChunk& c : g_chunks ) if ( c.tex_ready ) withTex++;
	_snprintf( g_report, sizeof( g_report ),
		"TERRAIN BAKE\n"
		"  switch            : %d   (shaders %s, ready %d)\n"
		"  bake resolution   : %u x %u per chunk   (setup.ini terrainbakeres)\n"
		"  chunks baked      : %d   (meshes)\n"
		"  chunks textured   : %d   %s\n"
		"  video memory      : %d KB  (%.1f MB)\n"
		"  last frame        : %d draw calls, %d frustum-culled\n"
		"  wicked terrain    : %s\n",
		gg_terrain_bake ? 1 : 0, g_shadersReady ? "ok" : "FAILED", g_ready ? 1 : 0,
		res, res,
		(int)g_chunks.size(),
		withTex, ( withTex == (int)g_chunks.size() ) ? "" : "<-- MISMATCH, some chunks have no texture",
		gg_terrain_bake_vram_kb, gg_terrain_bake_vram_kb / 1024.0f,
		gg_terrain_bake_drawcalls, gg_terrain_bake_culled,
		gg_no_terrain ? "TORN DOWN (bake is what you are seeing)" : "still live" );
	g_report[sizeof( g_report ) - 1] = 0;
	return g_report;
}

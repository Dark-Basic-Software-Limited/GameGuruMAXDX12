// =============================================================================================
// GGMAX 3.25 - WATER BAKE.
//
// Replaces the 2.94 "Water Off" brutal off-switch. Water Off still does the expensive half of
// the job - with IsOceanEnabled() false the planar reflection Z-prepass and colour pass are
// never recorded, which measured 4.37 ms of a 10.06 ms GPU frame on TESTPRO1 - but it leaves a
// hole where the lake was. This draws a flat semi-transparent plane in its place, so the level
// still reads correctly from the shore while costing one quad.
//
// INSTANT IN BOTH DIRECTIONS, by construction. Lee measured "GPU Idle + unranged" jumping from
// 1.5 ms to 7 ms when water came back on and, crucially, STAYING at 7 ms for about seventy
// seconds after switching it off again. Nothing here can reproduce that shape: this plane owns
// four vertices and one pipeline state, both created once, and the on/off edge changes a single
// bool that is read at the top of the draw. There is no simulation to spin down, no resource to
// reallocate and nothing that decays over a minute. That is a property of the design, not a fix
// for the ocean's behaviour - if the 7 ms is still there with this switch, it belongs to the
// ocean teardown itself and wants measuring on that card with GGWaterBake_Report() in hand.
// =============================================================================================

#include "GGWaterBake.h"

#include "WickedEngine.h"
#include "wiRenderer.h"
#include "wiScene.h"

#include "master.h"
#include "preprocessor-moreflags.h"
#include "gameguru.h"

#include "GGTerrain.h"

#include <cstdio>

using namespace wi::graphics;
using namespace wi::primitive;

bool  gg_water_bake = false;
float gg_water_bake_alpha = 0.62f;   // setup.ini waterbakealpha, 0..1
int   gg_water_bake_drawn = 0;

extern bool gg_no_water;
extern void GGSetNoWaterLevel( int );
namespace GGTerrain { void GGCustomFrame_Bind( wi::graphics::CommandList cmd ); }

namespace
{
	struct WaterVertex
	{
		XMFLOAT3 pos;      // xz = local offset from the camera, y = absolute water height
		uint32_t color;    // R8G8B8A8_UNORM
	};

	// Half-size of the plane. The GG world is 200,000 units across, so this reaches past any
	// level's edge from anywhere inside it and the quad never has a visible boundary.
	const float WATER_HALF_SIZE = 250000.0f;

	GPUBuffer     g_vb, g_ib;
	Shader        g_vs, g_ps;
	PipelineState g_pso;
	RasterizerState   g_rs;
	DepthStencilState g_dss;
	BlendState        g_bs;
	InputLayout       g_il;

	bool  g_initialised = false;
	bool  g_shadersReady = false;
	bool  g_vbValid = false;
	float g_lastHeight = -99999.0f;
	uint32_t g_lastColor = 0;
	char g_report[1024] = "";

	uint32_t CurrentWaterColor()
	{
		// Same source the real ocean reads (M-GridEditB_part3.cpp), so ticking the switch does
		// not change the colour of the water, only how it is drawn.
		const float r = t.visuals.WaterRed_f;
		const float g = t.visuals.WaterGreen_f;
		const float b = t.visuals.WaterBlue_f;
		float a = gg_water_bake_alpha;
		if ( a < 0.05f ) a = 0.05f;
		if ( a > 1.0f )  a = 1.0f;
		const uint32_t ri = (uint32_t)std::max( 0.0f, std::min( 255.0f, r ) );
		const uint32_t gi = (uint32_t)std::max( 0.0f, std::min( 255.0f, g ) );
		const uint32_t bi = (uint32_t)std::max( 0.0f, std::min( 255.0f, b ) );
		const uint32_t ai = (uint32_t)std::max( 0.0f, std::min( 255.0f, a * 255.0f ) );
		return ri | (gi << 8) | (bi << 16) | (ai << 24);
	}
}

void GGWaterBake_Init()
{
	if ( g_initialised ) return;
	g_initialised = true;

	wi::renderer::LoadShader( ShaderStage::VS, g_vs, "GGWaterBakeVS.cso" );
	wi::renderer::LoadShader( ShaderStage::PS, g_ps, "GGWaterBakePS.cso" );
	if ( !g_vs.IsValid() || !g_ps.IsValid() )
	{
		wi::backlog::post( "GGMAX 3.25: Water Bake shaders failed to load - the switch will hide water without replacing it.", wi::backlog::LogLevel::Error );
		return;
	}

	GraphicsDevice* device = GetDevice();

	// Two triangles, and the index buffer never changes.
	const uint16_t indices[6] = { 0, 1, 2, 2, 1, 3 };
	GPUBufferDesc ibdesc;
	ibdesc.size = sizeof( indices );
	ibdesc.bind_flags = BindFlag::INDEX_BUFFER;
	ibdesc.usage = Usage::DEFAULT;
	if ( !device->CreateBuffer( &ibdesc, indices, &g_ib ) ) return;
	device->SetName( &g_ib, "GGWaterBake::ib" );

	g_rs.fill_mode = FillMode::SOLID;
	// NONE, not BACK: the player can stand under the surface and it must still be there.
	g_rs.cull_mode = CullMode::NONE;
	g_rs.front_counter_clockwise = true;
	g_rs.depth_clip_enable = true;

	g_dss.depth_enable = true;
	g_dss.depth_write_mask = DepthWriteMask::ZERO;   // transparent: test, never write
	g_dss.depth_func = ComparisonFunc::GREATER_EQUAL; // reverse-Z
	g_dss.stencil_enable = false;

	g_bs.render_target[0].blend_enable = true;
	g_bs.render_target[0].src_blend = Blend::SRC_ALPHA;
	g_bs.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
	g_bs.render_target[0].blend_op = BlendOp::ADD;
	g_bs.render_target[0].src_blend_alpha = Blend::ONE;
	g_bs.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
	g_bs.render_target[0].blend_op_alpha = BlendOp::ADD;
	g_bs.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
	g_bs.independent_blend_enable = false;
	g_bs.alpha_to_coverage_enable = false;

	g_il.elements = {
		{ "POSITION", 0, Format::R32G32B32_FLOAT, 0, 0,  InputClassification::PER_VERTEX_DATA },
		{ "COLOR",    0, Format::R8G8B8A8_UNORM,  0, 12, InputClassification::PER_VERTEX_DATA },
	};

	// ★ PipelineStateDesc stores POINTERS and Wicked's DX12 backend compiles at BIND time, so
	// every piece of state here is a file-scope object, never a stack local. Tree PSO lesson.
	PipelineStateDesc desc = {};
	desc.vs = &g_vs;
	desc.ps = &g_ps;
	desc.il = &g_il;
	desc.pt = PrimitiveTopology::TRIANGLELIST;
	desc.rs = &g_rs;
	desc.dss = &g_dss;
	desc.bs = &g_bs;
	device->CreatePipelineState( &desc, &g_pso );

	g_shadersReady = true;
}

void GGWaterBake_Update()
{
	if ( !g_initialised ) GGWaterBake_Init();

	static bool s_prev = false;
	if ( gg_water_bake != s_prev )
	{
		// The switch still drives the real Water Off machinery - that is where the saving is.
		GGSetNoWaterLevel( gg_water_bake ? 1 : 0 );
		s_prev = gg_water_bake;
	}
	if ( !gg_water_bake || !g_shadersReady ) return;

	// Rebuild the four vertices only when the water height or colour actually changed. A level
	// with static water therefore does no per-frame work at all here.
	const float height = (float)g.gdefaultwaterheight;
	const uint32_t color = CurrentWaterColor();
	if ( g_vbValid && height == g_lastHeight && color == g_lastColor ) return;

	WaterVertex v[4];
	v[0].pos = XMFLOAT3( -WATER_HALF_SIZE, height, -WATER_HALF_SIZE );
	v[1].pos = XMFLOAT3(  WATER_HALF_SIZE, height, -WATER_HALF_SIZE );
	v[2].pos = XMFLOAT3( -WATER_HALF_SIZE, height,  WATER_HALF_SIZE );
	v[3].pos = XMFLOAT3(  WATER_HALF_SIZE, height,  WATER_HALF_SIZE );
	for ( int i = 0; i < 4; i++ ) v[i].color = color;

	GraphicsDevice* device = GetDevice();
	GPUBufferDesc vbdesc;
	vbdesc.size = sizeof( v );
	vbdesc.bind_flags = BindFlag::VERTEX_BUFFER;
	vbdesc.usage = Usage::DEFAULT;
	GPUBuffer fresh;
	if ( !device->CreateBuffer( &vbdesc, v, &fresh ) ) return;
	device->SetName( &fresh, "GGWaterBake::vb" );
	g_vb = fresh;
	g_vbValid = true;
	g_lastHeight = height;
	g_lastColor = color;
}

void GGWaterBake_Draw( const Frustum* frustum, CommandList cmd )
{
	gg_water_bake_drawn = 0;
	if ( !gg_water_bake || !g_shadersReady || !g_vbValid ) return;
	// Nothing to stand in for if the level has no water in the first place.
	if ( t.visuals.bWaterEnable == false && t.game.set.ismapeditormode != 1 ) return;

	GraphicsDevice* device = GetDevice();
	device->EventBegin( "GGWaterBake Draw", cmd );
	device->BindPipelineState( &g_pso, cmd );
	GGTerrain::GGCustomFrame_Bind( cmd );
	const GPUBuffer* vbs[] = { &g_vb };
	const uint32_t strides[] = { sizeof( WaterVertex ) };
	device->BindVertexBuffers( vbs, 0, 1, strides, 0, cmd );
	device->BindIndexBuffer( &g_ib, IndexBufferFormat::UINT16, 0, cmd );
	device->DrawIndexed( 6, 0, 0, cmd );
	device->EventEnd( cmd );
	gg_water_bake_drawn = 1;
}

const char* GGWaterBake_Report()
{
	_snprintf( g_report, sizeof( g_report ),
		"WATER BAKE\n"
		"  switch          : %d   (shaders %s)\n"
		"  plane           : %s   height %.1f, alpha %.2f (setup.ini waterbakealpha)\n"
		"  drawn last frame: %d\n"
		"  ocean           : %s\n",
		gg_water_bake ? 1 : 0, g_shadersReady ? "ok" : "FAILED",
		g_vbValid ? "built" : "not built", g_lastHeight, gg_water_bake_alpha,
		gg_water_bake_drawn,
		gg_no_water ? "OFF (planar reflection pass not recorded)" : "on" );
	g_report[sizeof( g_report ) - 1] = 0;
	return g_report;
}

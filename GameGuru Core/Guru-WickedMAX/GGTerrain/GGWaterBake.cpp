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
// setup.ini waterbakealpha / SET_WATERBAKEALPHA. NEGATIVE (the default) means "use the A from
// the Water component's own Water Base Color picker", which is what Lee asked for; 0..1 overrides
// it. There is no sensible non-negative default here - picking one would silently ignore the
// alpha the level author set in the picker, which is exactly the bug this replaces.
float gg_water_bake_alpha = -1.0f;
// ★★★ GGMAX 3.35f: the distance opacity ramp ("fresnel"), Lee 2026-08-29.
//
// The baked plane draws one flat colour at one authored alpha, so you can see straight through it
// to the lake bed at ANY distance - including out to the horizon, which no real water does. Real
// water goes opaque at grazing angles because it reflects rather than transmits, and on a flat
// plane viewed from above, grazing angle and distance are the same thing.
//
// ★ So this is a true Schlick term on the plane normal, not a distance lerp - and that matters:
// a raw distance ramp has to be retuned every time the camera changes height, whereas an angular
// one is correct from a cliff top and from the shoreline with no tuning at all.
//
//     strength 0 = off, the plain semi-transparent plane 3.25 shipped
//     strength 1 = fully opaque by the horizon
//     power      = how early the ramp bites. Lower goes opaque nearer the camera; 5 is textbook
//                  Schlick and only really closes in the last few degrees, which is too subtle
//                  here, so the default is 3.
//
// ⚠ 2026-08-26 Lee asked for the Fresnel that was in this shader to be REMOVED - he wanted the
// ground below the water line to show through. This is not that lift coming back: that one raised
// alpha everywhere including underfoot, which is what hid the lake bed. This leaves the water by
// your feet at exactly the authored alpha and only closes it up in the distance, which is what he
// asked for on 08-29. Set waterbakefresnel=0 in setup.cfg for the 3.25 behaviour.
float gg_water_bake_fresnel = 0.85f;
float gg_water_bake_fresnel_power = 3.0f;
int   gg_water_bake_drawn = 0;
// GGMAX 3.25d: 1 = force the plane to opaque magenta. A solid-colour bisect, live-switchable
// because it rides in the vertex colour rather than the shader. See the note above.
int   gg_water_bake_debug = 0;
// How far to mix the authored colour toward the sky it would be reflecting, 0..1.
// ★ DEFAULT 0 = use the picked colour exactly. Lee's spec (2026-08-26): "the colour of the water
// plane color to be determined by the RGB and the Alpha selectable from the Water component
// menu". An automatic lift, however well-meant, means the picker no longer says what you get -
// he sets a colour and sees a different one. The dial stays for anyone who wants the old
// behaviour, but nothing applies it unasked. setup.ini waterbaketint / SET_WATERBAKETINT.
float gg_water_bake_tint = 0.0f;

extern bool gg_no_water;
extern void GGSetNoWaterLevel( int );
namespace GGTerrain { void GGCustomFrame_Bind( wi::graphics::CommandList cmd ); }

namespace
{
	struct WaterVertex
	{
		XMFLOAT3 pos;      // xz = local offset from the camera, y = absolute water height
		uint32_t color;    // R8G8B8A8_UNORM
		// ★ GGMAX 3.35f: (strength, power) for the distance opacity ramp. In the vertex for the
		// same reason the colour is: there is no spare constant-buffer slot in this pass, and it is
		// four vertices. Uniform across the quad; the shader wants them per-pixel.
		XMFLOAT2 fres;
	};

	// Half-size of the plane. The GG world is 200,000 units across, so this reaches past any
	// level's edge from anywhere inside it and the quad never has a visible boundary.
	const float WATER_HALF_SIZE = 250000.0f;

	GPUBuffer     g_vb, g_ib;
	Shader        g_vs, g_ps;
	PipelineState g_pso;
	PipelineState g_psoDepthAlways;      // debug twin, ComparisonFunc::ALWAYS
	DepthStencilState g_dssAlways;
	RasterizerState   g_rs;
	DepthStencilState g_dss;
	BlendState        g_bs;
	InputLayout       g_il;

	bool  g_initialised = false;
	bool  g_shadersReady = false;
	int   g_shaderAttempts = 0;
	bool  g_vbValid = false;
	float g_lastHeight = -99999.0f;
	uint32_t g_lastColor = 0;
	float g_lastFresStr = -1.0f;   // GGMAX 3.35f: part of the rebuild test, see UpdateVB
	float g_lastFresPow = -1.0f;
	uint32_t g_updates = 0;    // GGWaterBake_Update calls that got past the switch test
	uint32_t g_rebuilds = 0;   // vertex buffers actually created
	uint32_t g_draws = 0;      // draw calls actually issued
	char g_report[1024] = "";

	uint32_t CurrentWaterColor()
	{
		// ★★ THE AUTHORED WATER COLOUR IS AN ABSORPTION TINT, NOT A SURFACE COLOUR.
		//
		// The first version painted t.visuals.Water{Red,Green,Blue}_f straight onto the plane,
		// reasoning that this is the same source the real ocean reads so the colour would match.
		// It does not. MEASURED on Canyon Offensive those fields are **1.0, 12.0, 11.5 out of
		// 255** - very nearly black. The ocean looks like light blue-grey water because Wicked's
		// ocean shader uses that value to tint what it REFLECTS; the tint alone is what deep water
		// absorbs, which is almost everything. Used raw it produced a plane so dark against the
		// lake bed that it read as no plane at all, while every counter said it had been drawn.
		// (Caught by the magenta bisect: forcing opaque magenta showed the plane perfectly, which
		// cleared geometry, transform, camera CB, depth, blend and root signature in one frame and
		// left only the colour.)
		//
		// So the surface colour is approximated the way the ocean effectively arrives at one: the
		// authored tint mixed toward the sky it would be reflecting. Done HERE, on the CPU, once
		// per change - the pixel shader stays a flat colour lookup with no per-pixel sky work,
		// which is what Lee asked for. gg_water_bake_tint is the mix, so 0 gives the pure authored
		// value for anyone who wants exactly that.
		float r = t.visuals.WaterRed_f;
		float g = t.visuals.WaterGreen_f;
		float b = t.visuals.WaterBlue_f;
		{
			float k = gg_water_bake_tint;
			if ( k < 0.0f ) k = 0.0f;
			if ( k > 1.0f ) k = 1.0f;
			if ( k > 0.0f )
			{
				// Horizon colour is what a flat plane mostly reflects; fall back to a neutral
				// daylight blue if the scene has no weather yet (level still loading).
				XMFLOAT3 sky = XMFLOAT3( 0.45f, 0.60f, 0.75f );
				wi::scene::Scene& scene = wi::scene::GetScene();
				if ( scene.weathers.GetCount() > 0 )
				{
					const wi::scene::WeatherComponent& w = scene.weathers[0];
					sky = w.horizon;
				}
				r = r + ( sky.x * 255.0f - r ) * k;
				g = g + ( sky.y * 255.0f - g ) * k;
				b = b + ( sky.z * 255.0f - b ) * k;
			}
		}
		// ★ ALPHA FROM THE PICKER. t.visuals.WaterAlpha_f is the A of the same "Water Base Color"
		// swatch that supplies the RGB above (M-TerrainNew_part3.cpp writes all four), so the one
		// control the author actually looks at decides both the colour and how far through it
		// they can see. A negative gg_water_bake_alpha means "use it"; anything else overrides.
		float a;
		if ( gg_water_bake_alpha >= 0.0f )
		{
			a = gg_water_bake_alpha;                       // explicit override
		}
		else
		{
			a = t.visuals.WaterAlpha_f / 255.0f;
			// ⚠ A == 0 is "never set", not "deliberately invisible". The field was write-only
			// until 3.25e (never saved, reset to 0 every load), so every level made before this
			// carries a zero, and an author who genuinely wanted no water would turn water OFF
			// rather than set its surface to fully transparent. Honouring a literal 0 here would
			// ship a switch that draws nothing on almost every existing level - which is exactly
			// the report that led here. Fall back, and SAY SO in the report rather than quietly
			// substituting a number.
			if ( a <= 0.0f ) a = 0.5f;
		}
		if ( a < 0.0f ) a = 0.0f;
		if ( a > 1.0f ) a = 1.0f;
		if ( gg_water_bake_debug != 0 ) return 0xFFFF00FFu;   // opaque magenta (A,B,G,R packed)
		const uint32_t ri = (uint32_t)std::max( 0.0f, std::min( 255.0f, r ) );
		const uint32_t gi = (uint32_t)std::max( 0.0f, std::min( 255.0f, g ) );
		const uint32_t bi = (uint32_t)std::max( 0.0f, std::min( 255.0f, b ) );
		const uint32_t ai = (uint32_t)std::max( 0.0f, std::min( 255.0f, a * 255.0f ) );
		return ri | (gi << 8) | (bi << 16) | (ai << 24);
	}
}

void GGWaterBake_Init()
{
	// ★ RETRY, do not latch on entry - same trap as GGTerrainBake_Init, see the note there.
	if ( g_shadersReady ) return;
	g_initialised = true;
	if ( g_shaderAttempts > 600 ) return;
	g_shaderAttempts++;

	wi::renderer::LoadShader( ShaderStage::VS, g_vs, "GGWaterBakeVS.cso" );
	wi::renderer::LoadShader( ShaderStage::PS, g_ps, "GGWaterBakePS.cso" );
	if ( !g_vs.IsValid() || !g_ps.IsValid() )
	{
		if ( g_shaderAttempts == 600 )
			wi::backlog::post( "GGMAX 3.25: Water Bake shaders never loaded after 600 attempts - the switch will hide water without replacing it.", wi::backlog::LogLevel::Error );
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
		{ "TEXCOORD", 0, Format::R32G32_FLOAT,    0, 16, InputClassification::PER_VERTEX_DATA }, // GGMAX 3.35f fresnel
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

	// the debug twin - identical but with the depth test disabled. Its own DepthStencilState
	// object because PipelineStateDesc stores POINTERS and both PSOs must keep the state they
	// were created with.
	g_dssAlways = g_dss;
	g_dssAlways.depth_func = ComparisonFunc::ALWAYS;
	desc.dss = &g_dssAlways;
	device->CreatePipelineState( &desc, &g_psoDepthAlways );

	g_shadersReady = true;
}

void GGWaterBake_Update()
{
	if ( !g_initialised ) GGWaterBake_Init();

	static bool s_prev = false;
	if ( gg_water_bake != s_prev )
	{
		s_prev = gg_water_bake;
		// ★ ORDER IS THE WHOLE FIX. Write the flag the visuals apply READS, and only then apply.
		// The panel used to call GGApplyVisualsNow() itself, immediately, while gg_no_water still
		// held last frame's value - so ticking left the ocean ON (its ripples showing through the
		// flat plane) and unticking turned it OFF and never brought it back. Doing both here, in
		// this order, also covers the harness and level-load paths, which the panel never did.
		GGSetNoWaterLevel( gg_water_bake ? 1 : 0 );
		extern void GGApplyVisualsNow();
		GGApplyVisualsNow();
	}
	if ( !gg_water_bake ) return;
	if ( !g_shadersReady ) { GGWaterBake_Init(); return; }     // keep retrying while ticked

	g_updates++;
	// Rebuild the four vertices only when the water height or colour actually changed. A level
	// with static water therefore does no per-frame work at all here.
	const float height = (float)g.gdefaultwaterheight;
	const uint32_t color = CurrentWaterColor();
	// ⚠ GGMAX 3.35f: the fresnel knobs are part of the vertex data, so they MUST be part of the
	// rebuild test. Leaving them out is the classic cached-value trap - the knob would reply OK,
	// the report would show the new number, and nothing on screen would ever change because the
	// vertex buffer still held the old one.
	const float fstr = ( gg_water_bake_fresnel < 0.0f ) ? 0.0f : ( gg_water_bake_fresnel > 1.0f ? 1.0f : gg_water_bake_fresnel );
	const float fpow = ( gg_water_bake_fresnel_power < 0.1f ) ? 0.1f : ( gg_water_bake_fresnel_power > 16.0f ? 16.0f : gg_water_bake_fresnel_power );
	if ( g_vbValid && height == g_lastHeight && color == g_lastColor
		&& fstr == g_lastFresStr && fpow == g_lastFresPow ) return;

	WaterVertex v[4];
	v[0].pos = XMFLOAT3( -WATER_HALF_SIZE, height, -WATER_HALF_SIZE );
	v[1].pos = XMFLOAT3(  WATER_HALF_SIZE, height, -WATER_HALF_SIZE );
	v[2].pos = XMFLOAT3( -WATER_HALF_SIZE, height,  WATER_HALF_SIZE );
	v[3].pos = XMFLOAT3(  WATER_HALF_SIZE, height,  WATER_HALF_SIZE );
	for ( int i = 0; i < 4; i++ ) { v[i].color = color; v[i].fres = XMFLOAT2( fstr, fpow ); }

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
	g_rebuilds++;
	g_lastHeight = height;
	g_lastFresStr = fstr;
	g_lastFresPow = fpow;
	g_lastColor = color;
}

void GGWaterBake_Draw( const Frustum* frustum, CommandList cmd )
{
	gg_water_bake_drawn = 0;
	if ( !gg_water_bake || !g_shadersReady || !g_vbValid ) return;
	// Nothing to stand in for if the level has no water in the first place. ★ This MIRRORS the
	// ocean's own test in M-GridEditB_part3.cpp rather than approximating it - the editor branch
	// reads t.showeditorwater and the game branch reads visuals.bWaterEnable, and getting that
	// split wrong would either paint a plane across a level that has no water or leave a hole in
	// one that does. If that test ever changes, this changes with it.
	const bool bLevelHasWater = ( t.game.set.ismapeditormode == 1 ) ? ( t.showeditorwater != 0 )
	                                                               : ( t.visuals.bWaterEnable != 0 );
	if ( !bLevelHasWater ) return;

	GraphicsDevice* device = GetDevice();
	device->EventBegin( "GGWaterBake Draw", cmd );
	device->BindPipelineState( ( gg_water_bake_debug == 2 ) ? &g_psoDepthAlways : &g_pso, cmd );
	GGTerrain::GGCustomFrame_Bind( cmd );
	const GPUBuffer* vbs[] = { &g_vb };
	const uint32_t strides[] = { sizeof( WaterVertex ) };
	device->BindVertexBuffers( vbs, 0, 1, strides, 0, cmd );
	device->BindIndexBuffer( &g_ib, IndexBufferFormat::UINT16, 0, cmd );
	device->DrawIndexed( 6, 0, 0, cmd );
	device->EventEnd( cmd );
	gg_water_bake_drawn = 1;
	g_draws++;
}

const char* GGWaterBake_Report()
{
	_snprintf( g_report, sizeof( g_report ),
		"WATER BAKE\n"
		"  switch          : %d   (shaders %s after %d attempts)\n"
		"  plane           : %s   height %.1f, alpha %.2f (%s)%s\n"
		"  colour source   : Water Base Color RGBA = %.0f, %.0f, %.0f, %.0f (the picker in\n"
		"                    Terrain Tools > Water), sky mix %.0f%% -> packed 0x%08X\n"
		"  drawn last frame: %d   (updates %u, rebuilds %u, draws %u)\n"
		"  VERTEX BUFFER   : 0x%08X   <- what the GPU is actually drawing\n"
		"  ocean           : %s\n",
		gg_water_bake ? 1 : 0, g_shadersReady ? "ok" : "FAILED", g_shaderAttempts,
		g_vbValid ? "built" : "not built", g_lastHeight,
		( gg_water_bake_alpha < 0.0f ) ? ( t.visuals.WaterAlpha_f / 255.0f ) : gg_water_bake_alpha,
		( gg_water_bake_alpha >= 0.0f ) ? "overridden by waterbakealpha"
			: ( t.visuals.WaterAlpha_f > 0.0f ? "from the Water Base Color picker"
			                                  : "FALLBACK 0.5 - the picker's A is 0 (never set on this level)" ),
		( gg_water_bake_debug == 2 ) ? "  [DEBUG: magenta + depth test ALWAYS]"
			: ( gg_water_bake_debug ? "  [DEBUG: forced opaque magenta]" : "" ),
		t.visuals.WaterRed_f, t.visuals.WaterGreen_f, t.visuals.WaterBlue_f, t.visuals.WaterAlpha_f,
		gg_water_bake_tint * 100.0f, CurrentWaterColor(),
		gg_water_bake_drawn, g_updates, g_rebuilds, g_draws, g_lastColor,
		gg_no_water ? "OFF (planar reflection pass not recorded)" : "on" );
	g_report[sizeof( g_report ) - 1] = 0;
	return g_report;
}

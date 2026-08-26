#pragma once
// =============================================================================================
// GGMAX 3.25 - TERRAIN BAKE
//
// Replaces the 2.94 "Terrain Off" brutal off-switch. Terrain Off removed the rendered terrain
// and left a hole in the world; this bakes each generated terrain chunk into an ordinary mesh
// and an ordinary texture first, removes the Wicked terrain with the SAME teardown, and then
// draws the baked chunks from a custom pass that touches no ECS at all.
//
// The measurement that motivated it (Lee, 6-year-old AMD card, 2026-08-26): switching terrain
// off took a frame from 22 ms to 12 ms, so the Wicked terrain path costs 10 ms on that class of
// hardware. The far-tree billboard pass on the same card is close to free, which is what says
// the cost is the machinery around the terrain - virtual texture indirection, SVT feedback and
// page requests, and several hundred chunk entities going through Scene::Update - rather than
// the triangles. So: keep the triangles, throw away the machinery.
//
// Lifetime is RUNTIME, per session, nothing written to disk (Lee's call): the bake runs when the
// switch is ticked and always matches the terrain as it is right now, so sculpting cannot leave
// a stale baked copy behind and there is no invalidation logic to get wrong.
// =============================================================================================

#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"

// The switch itself. Panel tick box "Terrain Bake", setup.ini `terrainbake`, harness
// SET_TERRAINBAKESWITCH. Drives gg_no_terrain internally once the bake is ready.
extern bool gg_terrain_bake;
// Bake resolution per chunk, in pixels per axis. setup.ini `terrainbakeres`. 256 is the shipped
// default: a chunk spans 66 world units of terrain grid, so 256 gives roughly 4 texels per grid
// cell. Raising it multiplies video memory by the square, which the report below spells out.
extern int  gg_terrain_bake_res;

// Diagnostics, all read by the harness DUMP_TERRAINBAKE.
extern int  gg_terrain_bake_chunks;      // chunks with a built mesh
extern int  gg_terrain_bake_textures;    // chunks whose texture bake succeeded
extern int  gg_terrain_bake_drawcalls;   // draw calls issued last frame
extern int  gg_terrain_bake_culled;      // chunks frustum-culled last frame
extern int  gg_terrain_bake_vram_kb;     // total video memory held by the bake
extern int  gg_terrain_bake_notready;    // chunks not bakeable yet (bake is waiting, not broken)

void GGTerrainBake_Init();                                                   // main thread, once
void GGTerrainBake_Update();                                                 // main thread, per frame: drives the state machine
void GGTerrainBake_Clear();                                                  // main thread: release everything
bool GGTerrainBake_Ready();                                                  // true once the terrain may be torn down
void GGTerrainBake_RecordPendingBakes( wi::graphics::CommandList cmd );        // render thread: record the compute dispatches
void GGTerrainBake_Draw( const wi::primitive::Frustum* frustum, wi::graphics::CommandList cmd );
void GGTerrainBake_DrawPrepass( const wi::primitive::Frustum* frustum, wi::graphics::CommandList cmd );
const char* GGTerrainBake_Report();
